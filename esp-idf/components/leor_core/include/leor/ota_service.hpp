#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

namespace leor {

class OtaService {
  public:
    static constexpr uint8_t kCtrlNop = 0x00;
    static constexpr uint8_t kCtrlRequest = 0x01;
    static constexpr uint8_t kCtrlRequestAck = 0x02;
    static constexpr uint8_t kCtrlRequestNak = 0x03;
    static constexpr uint8_t kCtrlDone = 0x04;
    static constexpr uint8_t kCtrlDoneAck = 0x05;
    static constexpr uint8_t kCtrlDoneNak = 0x06;
    static constexpr uint8_t kCtrlCredit = 0x07;

    bool in_progress() const { return in_progress_; }
    bool control_notify_pending() const { return control_notify_pending_; }
    uint8_t control_notify_code() const { return control_notify_code_; }
    void consume_control_notify() { control_notify_pending_ = false; }
    bool reboot_pending() const { return reboot_pending_; }

    void reset();
    esp_err_t set_packet_size(uint16_t packet_size);
    uint8_t handle_control_write(uint8_t opcode);
    uint8_t handle_data_write(const uint8_t* data, size_t len);
    void poll();

  private:
    bool in_progress_ = false;
    bool control_notify_pending_ = false;
    uint8_t control_notify_code_ = kCtrlNop;
    bool reboot_pending_ = false;
    uint16_t packet_size_ = 0;
    uint32_t packets_rx_ = 0;
    uint32_t bytes_rx_ = 0;
    uint64_t reboot_at_us_ = 0;
    const esp_partition_t* ota_partition_ = nullptr;
    esp_ota_handle_t ota_handle_ = 0;
};

}  // namespace leor
