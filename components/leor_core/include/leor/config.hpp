#pragma once

#include <cstdint>

namespace leor {

enum class DisplayController : uint8_t {
    kSh1106,
    kSsd1306,
};

struct DisplayConfig {
    DisplayController controller = DisplayController::kSh1106;
    uint8_t i2c_address = 0x3c;
    int width = 128;
    int height = 64;
    int i2c_port = 0;
    int sda_pin = 10;
    int scl_pin = 7;
};

struct RuntimeConfig {
    DisplayConfig display{};
    uint32_t frame_interval_ms = 16;
    bool gesture_dummy_enabled = false;
    uint8_t touch_wake_pin = 0;
    uint8_t touch_active_level = 1;
    uint32_t touch_hold_ms = 3000;
    int pwr_ctrl_pin = 1;
    int led_pin = -1;
};

}  // namespace leor
