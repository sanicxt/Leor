#include "leor/ota_service.hpp"

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_system.h"

namespace leor {

namespace {

static const char* kTag = "leor_ota";
static constexpr uint32_t kCreditBatch = 32;
static constexpr uint8_t kMetaMagic0 = 0x4c; // 'L'
static constexpr uint8_t kMetaMagic1 = 0x52; // 'R'

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
    expected_size_ = 0;
    transfer_size_hint_ = 0;
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
            set_error("Begin error");
            return kCtrlRequestNak;
        }
        in_progress_ = true;
        packets_rx_ = 0;
        bytes_rx_ = 0;
        expected_size_ = ota_partition_->size;
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
            set_error(end_err == ESP_ERR_OTA_VALIDATE_FAILED ? "Bad image!" : "End error");
            return kCtrlDoneNak;
        }

        const esp_err_t boot_err = esp_ota_set_boot_partition(ota_partition_);
        if (boot_err != ESP_OK) {
            ESP_LOGW(kTag, "set_boot_partition failed: %s", esp_err_to_name(boot_err));
            set_error("Boot set error");
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

    if (!in_progress_ && len == 8 && data[0] == kMetaMagic0 && data[1] == kMetaMagic1) {
        packet_size_ = static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);
        transfer_size_hint_ = static_cast<uint32_t>(data[4]) |
                              (static_cast<uint32_t>(data[5]) << 8) |
                              (static_cast<uint32_t>(data[6]) << 16) |
                              (static_cast<uint32_t>(data[7]) << 24);
        return kCtrlNop;
    }

    if (!in_progress_) {
        return kCtrlDoneNak;
    }

    if (packets_rx_ == 0 && (data[0] != 0xE9 || len < 16)) {
        ESP_LOGW(kTag, "invalid image header: 0x%02x, len=%zu", data[0], len);
        set_error("Invalid ESP32 bin!");
        return kCtrlDoneNak;
    }

    if (ota_partition_ != nullptr && bytes_rx_ + len > ota_partition_->size) {
        ESP_LOGW(kTag, "OTA data exceeds partition size! rx=%u, len=%zu, max=%u",
                 (unsigned)bytes_rx_, len, (unsigned)ota_partition_->size);
        set_error("File too large!");
        return kCtrlDoneNak;
    }

    const esp_err_t err = esp_ota_write(ota_handle_, data, len);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "esp_ota_write failed: %s", esp_err_to_name(err));
        set_error("Write error!");
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

int OtaService::progress_percent() const {
    if (bytes_rx_ == 0 || !in_progress_) {
        return 0;
    }
    uint32_t estimated_total = transfer_size_hint_ > 0 ? transfer_size_hint_ : expected_size_;
    if (estimated_total == 0) {
        return 0;
    }
    int pct = static_cast<int>((static_cast<uint64_t>(bytes_rx_) * 100) / estimated_total);
    if (pct > 99 && in_progress_) {
        pct = 99;
    }
    return pct;
}

bool OtaService::error_pending() const {
    return show_error_until_us_ > 0 && esp_timer_get_time() < show_error_until_us_;
}

void OtaService::set_error(const char* msg) {
    error_message_ = msg;
    show_error_until_us_ = esp_timer_get_time() + 3000000ULL;
    reset();
}

}  // namespace leor
