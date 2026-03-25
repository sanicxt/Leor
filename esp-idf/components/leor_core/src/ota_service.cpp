#include "leor/ota_service.hpp"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_system.h"

namespace leor {

namespace {

static const char* kTag = "leor_ota";
static constexpr uint32_t kCreditBatch = 32;

}  // namespace

void OtaService::reset() {
    if (ota_handle_ != 0) {
        esp_ota_abort(ota_handle_);
    }
    in_progress_ = false;
    control_notify_pending_ = false;
    control_notify_code_ = kCtrlNop;
    reboot_pending_ = false;
    packet_size_ = 0;
    packets_rx_ = 0;
    bytes_rx_ = 0;
    reboot_at_us_ = 0;
    ota_partition_ = nullptr;
    ota_handle_ = 0;
}

esp_err_t OtaService::set_packet_size(uint16_t packet_size) {
    packet_size_ = packet_size;
    return ESP_OK;
}

uint8_t OtaService::handle_control_write(uint8_t opcode) {
    if (opcode == kCtrlRequest) {
        if (in_progress_) {
            reset();
        }
        ota_partition_ = esp_ota_get_next_update_partition(nullptr);
        if (ota_partition_ == nullptr) {
            ESP_LOGW(kTag, "no OTA partition available");
            return kCtrlRequestNak;
        }
        const esp_err_t err = esp_ota_begin(ota_partition_, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle_);
        if (err != ESP_OK) {
            ESP_LOGW(kTag, "esp_ota_begin failed: %s", esp_err_to_name(err));
            reset();
            return kCtrlRequestNak;
        }
        in_progress_ = true;
        packets_rx_ = 0;
        bytes_rx_ = 0;
        ESP_LOGI(kTag, "OTA started, pkt=%u", static_cast<unsigned>(packet_size_));
        return kCtrlRequestAck;
    }

    if (opcode == kCtrlDone) {
        if (!in_progress_) {
            return kCtrlDoneNak;
        }

        const esp_err_t end_err = esp_ota_end(ota_handle_);
        ota_handle_ = 0;
        if (end_err != ESP_OK) {
            ESP_LOGW(kTag, "esp_ota_end failed: %s", esp_err_to_name(end_err));
            reset();
            return kCtrlDoneNak;
        }

        const esp_err_t boot_err = esp_ota_set_boot_partition(ota_partition_);
        if (boot_err != ESP_OK) {
            ESP_LOGW(kTag, "set_boot_partition failed: %s", esp_err_to_name(boot_err));
            reset();
            return kCtrlDoneNak;
        }

        in_progress_ = false;
        reboot_pending_ = true;
        reboot_at_us_ = esp_timer_get_time() + 1000000ULL;
        ESP_LOGI(kTag, "OTA complete, rebooting soon");
        return kCtrlDoneAck;
    }

    return kCtrlNop;
}

uint8_t OtaService::handle_data_write(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) {
        return kCtrlNop;
    }

    if (!in_progress_ && len == 2) {
        packet_size_ = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
        return kCtrlNop;
    }

    if (!in_progress_) {
        return kCtrlDoneNak;
    }

    if (packets_rx_ == 0 && data[0] != 0xE9) {
        ESP_LOGW(kTag, "invalid image header byte: 0x%02x", data[0]);
        reset();
        return kCtrlDoneNak;
    }

    const esp_err_t err = esp_ota_write(ota_handle_, data, len);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "esp_ota_write failed: %s", esp_err_to_name(err));
        reset();
        return kCtrlDoneNak;
    }

    packets_rx_++;
    bytes_rx_ += static_cast<uint32_t>(len);

    if ((packets_rx_ % kCreditBatch) == 0) {
        control_notify_pending_ = true;
        control_notify_code_ = kCtrlCredit;
    }

    return kCtrlNop;
}

void OtaService::poll() {
    if (reboot_pending_ && esp_timer_get_time() >= static_cast<int64_t>(reboot_at_us_)) {
        esp_restart();
    }
}

}  // namespace leor
