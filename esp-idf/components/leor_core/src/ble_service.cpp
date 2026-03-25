#include "leor/ble_service.hpp"

#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/os_mbuf.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <algorithm>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void ble_store_config_init(void);

namespace leor {

namespace {

static const char* kTag = "leor_ble";
static BleService* s_service = nullptr;
static uint8_t s_own_addr_type = 0;
static uint16_t s_status_handle = 0;
static uint16_t s_gesture_handle = 0;
static uint16_t s_ota_control_handle = 0;
static uint16_t s_ota_data_handle = 0;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static std::string s_last_status = "ready";
static std::string s_last_gesture = "idle";

constexpr ble_uuid128_t kServiceUuid = BLE_UUID128_INIT(0x4b,0x91,0x31,0xc3,0xc9,0xc5,0xcc,0x8f,0x9e,0x45,0xb5,0x1f,0x01,0xc2,0xaf,0x4f);
constexpr ble_uuid128_t kCommandUuid = BLE_UUID128_INIT(0xa8,0x26,0x1b,0x36,0x07,0xea,0xf5,0xb7,0x88,0x46,0xe1,0x36,0x3e,0x48,0xb5,0xbe);
constexpr ble_uuid128_t kStatusUuid = BLE_UUID128_INIT(0x7e,0xe8,0x7b,0x5d,0x2e,0x7a,0x3d,0xbf,0x3a,0x41,0xf7,0xd8,0xe3,0xd5,0x95,0x1c);
constexpr ble_uuid128_t kGestureUuid = BLE_UUID128_INIT(0x3a,0x2f,0x1e,0x0d,0x9c,0x8b,0x7a,0x6f,0x5e,0x4d,0x3c,0x2b,0xa1,0xf0,0xe5,0xd1);
constexpr ble_uuid128_t kOtaServiceUuid = BLE_UUID128_INIT(0xd8,0xe6,0xfd,0x1d,0x4a,0x24,0xc6,0xb1,0x53,0x4c,0x4c,0x59,0x6d,0xd9,0xf1,0xd6);
constexpr ble_uuid128_t kOtaControlUuid = BLE_UUID128_INIT(0x30,0xd8,0xe3,0x3a,0x0e,0x27,0x22,0xb7,0xa4,0x46,0xc0,0x21,0xaa,0x71,0xd6,0x7a);
constexpr ble_uuid128_t kOtaDataUuid = BLE_UUID128_INIT(0xb0,0xa5,0xf8,0x45,0x8d,0xca,0x89,0x9b,0xd8,0x4c,0x40,0x1f,0x88,0x88,0x40,0x23);

void advertise();

int gap_event(struct ble_gap_event* event, void* arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                s_conn_handle = event->connect.conn_handle;
                if (s_service) {
                    s_service->on_connected(s_conn_handle);
                    s_service->notify_status("connected");
                }
            } else {
                advertise();
            }
            return 0;
        case BLE_GAP_EVENT_DISCONNECT:
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            if (s_service) s_service->on_disconnected();
            advertise();
            return 0;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            advertise();
            return 0;
        default:
            return 0;
    }
}

int gatt_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    const ble_uuid_t* uuid = ctxt->chr->uuid;
    if (ble_uuid_cmp(uuid, &kCommandUuid.u) == 0 && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        std::string cmd;
        cmd.resize(OS_MBUF_PKTLEN(ctxt->om));
        os_mbuf_copydata(ctxt->om, 0, cmd.size(), cmd.data());
        if (s_service) {
            const std::string response = s_service->handle_command(cmd);
            if (!response.empty()) {
                s_service->notify_status(response);
            }
        }
        return 0;
    }

    if (ble_uuid_cmp(uuid, &kOtaControlUuid.u) == 0 && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        if (s_service && OS_MBUF_PKTLEN(ctxt->om) >= 1) {
            uint8_t opcode = 0;
            os_mbuf_copydata(ctxt->om, 0, 1, &opcode);
            const uint8_t ack = s_service->ota_handle_control(opcode);
            if (ack != OtaService::kCtrlNop) {
                struct os_mbuf* om = ble_hs_mbuf_from_flat(&ack, 1);
                ble_gatts_notify_custom(s_conn_handle, s_ota_control_handle, om);
            }
        }
        return 0;
    }

    if (ble_uuid_cmp(uuid, &kOtaDataUuid.u) == 0 && ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        if (s_service) {
            const size_t len = OS_MBUF_PKTLEN(ctxt->om);
            std::string payload;
            payload.resize(len);
            if (len > 0) {
                os_mbuf_copydata(ctxt->om, 0, len, payload.data());
            }
            const uint8_t code = s_service->ota_handle_data(reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
            if (code != OtaService::kCtrlNop) {
                struct os_mbuf* om = ble_hs_mbuf_from_flat(&code, 1);
                ble_gatts_notify_custom(s_conn_handle, s_ota_control_handle, om);
            }
        }
        return 0;
    }

    const std::string* payload = nullptr;
    if (ble_uuid_cmp(uuid, &kStatusUuid.u) == 0) {
        payload = &s_last_status;
    } else if (ble_uuid_cmp(uuid, &kGestureUuid.u) == 0) {
        payload = &s_last_gesture;
    }
    if (payload) {
        os_mbuf_append(ctxt->om, payload->data(), payload->size());
    }
    return 0;
}

ble_gatt_chr_def kGattCharacteristics[] = {
    {&kCommandUuid.u, gatt_access, nullptr, nullptr, BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP, 0, nullptr, nullptr},
    {&kStatusUuid.u, gatt_access, nullptr, nullptr, BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, 0, &s_status_handle, nullptr},
    {&kGestureUuid.u, gatt_access, nullptr, nullptr, BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, 0, &s_gesture_handle, nullptr},
    {nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, nullptr},
};

ble_gatt_chr_def kOtaCharacteristics[] = {
    {&kOtaControlUuid.u, gatt_access, nullptr, nullptr, BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY, 0, &s_ota_control_handle, nullptr},
    {&kOtaDataUuid.u, gatt_access, nullptr, nullptr, BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP, 0, &s_ota_data_handle, nullptr},
    {nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, nullptr},
};

const struct ble_gatt_svc_def kGattServices[] = {
    {BLE_GATT_SVC_TYPE_PRIMARY, &kServiceUuid.u, nullptr, kGattCharacteristics},
    {BLE_GATT_SVC_TYPE_PRIMARY, &kOtaServiceUuid.u, nullptr, kOtaCharacteristics},
    {0, nullptr, nullptr, nullptr},
};

void on_reset(int reason) {
    ESP_LOGW(kTag, "nimble reset reason=%d", reason);
}

void on_sync(void) {
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    advertise();
}

void host_task(void* param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void advertise() {
    struct ble_hs_adv_fields adv_fields = {};
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.uuids128 = const_cast<ble_uuid128_t*>(&kServiceUuid);
    adv_fields.num_uuids128 = 1;
    adv_fields.uuids128_is_complete = 1;
    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGW(kTag, "adv fields set failed rc=%d", rc);
    }

    struct ble_hs_adv_fields rsp_fields = {};
    const char* name = ble_svc_gap_device_name();
    rsp_fields.name = reinterpret_cast<uint8_t*>(const_cast<char*>(name));
    rsp_fields.name_len = std::strlen(name);
    rsp_fields.name_is_complete = 1;
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        // Fallback for long names: send a shortened scan response name.
        rsp_fields.name_len = std::min<size_t>(rsp_fields.name_len, 20U);
        rsp_fields.name_is_complete = 0;
        rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
        if (rc != 0) {
            ESP_LOGW(kTag, "adv scan response set failed rc=%d", rc);
        }
    }

    struct ble_gap_adv_params adv = {};
    adv.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(s_own_addr_type, nullptr, BLE_HS_FOREVER, &adv, gap_event, nullptr);
    if (rc != 0) {
        ESP_LOGW(kTag, "adv start failed rc=%d", rc);
    }
}

}  // namespace

esp_err_t BleService::start(const std::string& device_name, CommandHandler handler) {
    command_handler_ = std::move(handler);
    s_service = this;
    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(kGattServices);
    ble_gatts_add_svcs(kGattServices);
    ble_svc_gap_device_name_set(device_name.c_str());
    ble_store_config_init();
    nimble_port_freertos_init(host_task);
    return ESP_OK;
}

void BleService::poll() {
    ota_.poll();
    if (connected_ && ota_.control_notify_pending()) {
        const uint8_t code = ota_.control_notify_code();
        ota_.consume_control_notify();
        if (s_ota_control_handle != 0) {
            struct os_mbuf* om = ble_hs_mbuf_from_flat(&code, 1);
            ble_gatts_notify_custom(s_conn_handle, s_ota_control_handle, om);
        }
    }
}

void BleService::notify_status(const std::string& status) {
    s_last_status = status;
    connected_ = s_conn_handle != BLE_HS_CONN_HANDLE_NONE;
    if (!connected_) {
        return;
    }

    const uint16_t mtu = ble_att_mtu(s_conn_handle);
    const size_t max_payload = mtu > 3 ? static_cast<size_t>(mtu - 3) : 20U;
    const size_t chunk_size = std::max<size_t>(20U, max_payload);

    if (status.size() <= chunk_size) {
        struct os_mbuf* om = ble_hs_mbuf_from_flat(status.data(), status.size());
        ble_gatts_notify_custom(s_conn_handle, s_status_handle, om);
        return;
    }

    for (size_t offset = 0; offset < status.size(); offset += chunk_size) {
        const size_t len = std::min(chunk_size, status.size() - offset);
        struct os_mbuf* om = ble_hs_mbuf_from_flat(status.data() + offset, len);
        if (ble_gatts_notify_custom(s_conn_handle, s_status_handle, om) != 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void BleService::notify_gesture(const std::string& gesture) {
    s_last_gesture = gesture;
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        struct os_mbuf* om = ble_hs_mbuf_from_flat(gesture.data(), gesture.size());
        ble_gatts_notify_custom(s_conn_handle, s_gesture_handle, om);
    }
}

void BleService::set_low_power_mode(bool enabled) {
    low_power_mode_ = enabled;
}

void BleService::on_connected(uint16_t conn_handle) {
    connected_ = conn_handle != BLE_HS_CONN_HANDLE_NONE;
    struct ble_gap_upd_params params = {};
    if (low_power_mode_) {
        params.itvl_min = 80;
        params.itvl_max = 160;
        params.latency = 0;
        params.supervision_timeout = 400;
    } else {
        params.itvl_min = 24;
        params.itvl_max = 48;
        params.latency = 0;
        params.supervision_timeout = 180;
    }
    ble_gap_update_params(conn_handle, &params);
}

void BleService::on_disconnected() {
    connected_ = false;
}

uint8_t BleService::ota_handle_control(uint8_t opcode) {
    return ota_.handle_control_write(opcode);
}

uint8_t BleService::ota_handle_data(const uint8_t* data, size_t len) {
    return ota_.handle_data_write(data, len);
}

bool BleService::ota_has_pending_notify() const {
    return ota_.control_notify_pending();
}

uint8_t BleService::ota_pending_notify_code() const {
    return ota_.control_notify_code();
}

void BleService::ota_consume_pending_notify() {
    ota_.consume_control_notify();
}

std::string BleService::handle_command(const std::string& command) const {
    return command_handler_ ? command_handler_(command) : std::string();
}

}  // namespace leor
