#pragma once

#include <cstdint>
#include <functional>

namespace leor {

enum class ButtonEvent { kNone, kShortPress, kLongPress };

class PowerService {
public:
  using SleepPrepareCallback = std::function<void()>;

  void init(uint8_t touch_pin, uint8_t active_level, uint32_t hold_ms,
            int pwr_ctrl_pin = 1, int led_pin = -1);
  void arm(uint32_t delay_ms, uint32_t now_ms);
  ButtonEvent poll(uint32_t now_ms);
  void do_sleep();
  uint32_t hold_ms() const { return hold_ms_; }
  void set_hold_ms(uint32_t value_ms);
  void set_sleep_prepare_callback(SleepPrepareCallback callback);
  void set_i2c_pins(int sda_pin, int scl_pin);

private:
  bool pressed() const;

  uint8_t touch_pin_ = 0;
  uint8_t active_level_ = 1;
  int pwr_ctrl_pin_ = 1;
  int led_pin_ = 8;
  uint32_t hold_ms_ = 3000;
  uint32_t press_start_ms_ = 0;
  bool last_state_ = false;
  uint32_t enable_at_ms_ = 0;
  SleepPrepareCallback sleep_prepare_callback_;
  int i2c_sda_pin_ = -1;
  int i2c_scl_pin_ = -1;
};

} // namespace leor
