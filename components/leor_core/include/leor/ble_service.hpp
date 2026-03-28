#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>

#include "esp_err.h"
#include "leor/ota_service.hpp"

namespace leor {

class BleService {
  public:
    using CommandHandler = std::function<std::string(const std::string&)>;

    esp_err_t start(const std::string& device_name, CommandHandler handler);
    void stop();
    void start_advertising();
    void poll();
    void notify_status(const std::string& status);
    void notify_gesture(const std::string& gesture);
    std::string handle_command(const std::string& command) const;
    uint8_t ota_handle_control(uint8_t opcode);
    uint8_t ota_handle_data(const uint8_t* data, size_t len);
    bool ota_has_pending_notify() const;
    uint8_t ota_pending_notify_code() const;
    void ota_consume_pending_notify();
    void on_connected(uint16_t conn_handle);
    void on_disconnected();
    bool connected() const { return connected_; }
    bool low_power_mode() const { return low_power_mode_; }
    void set_low_power_mode(bool enabled);
    OtaService& ota() { return ota_; }
    const OtaService& ota() const { return ota_; }

  private:
    CommandHandler command_handler_;
    OtaService ota_{};
    bool connected_ = false;
    bool low_power_mode_ = false;
};

}  // namespace leor
