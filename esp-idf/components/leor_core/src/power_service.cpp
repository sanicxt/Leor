#include "leor/power_service.hpp"

#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace leor {

namespace {

void configure_output_pin(gpio_num_t pin, int level) {
  esp_rom_gpio_pad_select_gpio(pin);
  gpio_hold_dis(pin);
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = (1ULL << pin);
  cfg.mode = GPIO_MODE_OUTPUT;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);
  gpio_set_level(pin, level);
  gpio_hold_en(pin);
}

void release_hold(gpio_num_t pin) {
  gpio_hold_dis(pin);
  esp_rom_gpio_pad_select_gpio(pin);
}

void configure_touch_inactive_level(gpio_num_t pin, uint8_t active_level) {
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = (1ULL << pin);
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = active_level == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  cfg.pull_down_en =
      active_level == 1 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&cfg);
}

} // namespace

void PowerService::init(uint8_t touch_pin, uint8_t active_level,
                        uint32_t hold_ms, int pwr_ctrl_pin, int led_pin) {
  touch_pin_ = touch_pin;
  active_level_ = active_level;
  pwr_ctrl_pin_ = pwr_ctrl_pin;
  led_pin_ = led_pin;
  set_hold_ms(hold_ms);

  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(static_cast<gpio_num_t>(touch_pin_));

  if (pwr_ctrl_pin_ >= 0) {
    configure_output_pin(static_cast<gpio_num_t>(pwr_ctrl_pin_), 0);
  }

  if (led_pin_ >= 0) {
    configure_output_pin(static_cast<gpio_num_t>(led_pin_), 1);
  }

  vTaskDelay(pdMS_TO_TICKS(20));

  configure_touch_inactive_level(static_cast<gpio_num_t>(touch_pin_),
                                 active_level_);
  last_state_ = pressed();
}

void PowerService::set_sleep_prepare_callback(SleepPrepareCallback callback) {
  sleep_prepare_callback_ = std::move(callback);
}

void PowerService::arm(uint32_t delay_ms, uint32_t now_ms) {
  enable_at_ms_ = now_ms + delay_ms;
  press_start_ms_ = 0;
  last_state_ = pressed();
}

void PowerService::set_hold_ms(uint32_t value_ms) {
  if (value_ms < 1000U)
    value_ms = 1000U;
  if (value_ms > 15000U)
    value_ms = 15000U;
  hold_ms_ = value_ms;
}

bool PowerService::pressed() const {
  return gpio_get_level(static_cast<gpio_num_t>(touch_pin_)) ==
         static_cast<int>(active_level_);
}

ButtonEvent PowerService::poll(uint32_t now_ms) {
  if (now_ms < enable_at_ms_) {
    return ButtonEvent::kNone;
  }
  const bool current = pressed();
  ButtonEvent event = ButtonEvent::kNone;

  if (!last_state_ && current) {
    press_start_ms_ = now_ms;
  }
  
  if (last_state_ && !current) {
    if (press_start_ms_ != 0) {
      uint32_t duration = now_ms - press_start_ms_;
      if (duration >= 50 && duration < hold_ms_) { // 50ms denounce
        event = ButtonEvent::kShortPress;
      }
    }
    press_start_ms_ = 0;
  }

  if (current && press_start_ms_ != 0) {
    if (now_ms - press_start_ms_ >= hold_ms_) {
      event = ButtonEvent::kLongPress;
      press_start_ms_ = 0; // Consume
    }
  }

  last_state_ = current;
  return event;
}

void PowerService::do_sleep() {
  if (sleep_prepare_callback_) {
    sleep_prepare_callback_();
  }
  if (pwr_ctrl_pin_ >= 0) {
    gpio_num_t pin = static_cast<gpio_num_t>(pwr_ctrl_pin_);
    release_hold(pin);
    gpio_set_level(pin, 1);
    gpio_hold_en(pin);
  }
  if (led_pin_ >= 0) {
    gpio_num_t pin = static_cast<gpio_num_t>(led_pin_);
    release_hold(pin);
    gpio_set_level(pin, 1);
    gpio_hold_en(pin);
  }
  const gpio_num_t touch_gpio = static_cast<gpio_num_t>(touch_pin_);
  configure_touch_inactive_level(touch_gpio, active_level_);
  gpio_hold_en(touch_gpio);

  const esp_sleep_gpio_wake_up_mode_t wake_mode =
      active_level_ == 0 ? ESP_GPIO_WAKEUP_GPIO_LOW
                         : ESP_GPIO_WAKEUP_GPIO_HIGH;
  esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown((1ULL << touch_pin_),
                                                      wake_mode);

  const uint32_t release_start_ms =
      static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
  while (pressed()) {
    const uint32_t loop_now =
        static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
    if (loop_now - release_start_ms > 5000U) {
      gpio_hold_dis(touch_gpio);
      if (pwr_ctrl_pin_ >= 0) {
        gpio_num_t pin = static_cast<gpio_num_t>(pwr_ctrl_pin_);
        release_hold(pin);
        gpio_set_level(pin, 0);
        gpio_hold_en(pin);
      }
      if (led_pin_ >= 0) {
        gpio_num_t pin = static_cast<gpio_num_t>(led_pin_);
        release_hold(pin);
        gpio_set_level(pin, 1);
        gpio_hold_en(pin);
      }
      press_start_ms_ = 0;
      return;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelay(pdMS_TO_TICKS(50));

  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
}

} // namespace leor
