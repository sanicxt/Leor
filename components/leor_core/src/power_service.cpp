#include "leor/power_service.hpp"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_rom_gpio.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* kTag = "leor_power";

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

// Drive a GPIO output LOW, disable all internal pulls, and hold it.
// Used for I2C SDA/SCL before cutting VCC_PERIPH to prevent leakage
// through the I2C pull-up resistors back through ESP32's ESD diodes.
void drive_low_and_hold(int pin) {
  if (pin < 0) return;
  const gpio_num_t g = static_cast<gpio_num_t>(pin);
  esp_rom_gpio_pad_select_gpio(g);     // wrest pad from I2C peripheral
  gpio_hold_dis(g);
  gpio_set_direction(g, GPIO_MODE_OUTPUT);
  gpio_set_level(g, 0);
  gpio_pullup_dis(g);
  gpio_pulldown_dis(g);
  gpio_hold_en(g);
}

// Atomically transition a GPIO to output HIGH and hold it.
// No float gap: pad select → hold_dis → direction → level → hold_en.
void drive_high_and_hold(int pin) {
  if (pin < 0) return;
  const gpio_num_t g = static_cast<gpio_num_t>(pin);
  esp_rom_gpio_pad_select_gpio(g);
  gpio_hold_dis(g);
  gpio_set_direction(g, GPIO_MODE_OUTPUT);
  gpio_set_level(g, 1);
  gpio_hold_en(g);
}

// Atomically transition a GPIO to output LOW and hold it (for power-on).
void drive_low_output_and_hold(int pin) {
  if (pin < 0) return;
  const gpio_num_t g = static_cast<gpio_num_t>(pin);
  esp_rom_gpio_pad_select_gpio(g);
  gpio_hold_dis(g);
  gpio_set_direction(g, GPIO_MODE_OUTPUT);
  gpio_set_level(g, 0);
  gpio_hold_en(g);
}

} // namespace

void PowerService::init(uint8_t touch_pin, uint8_t active_level,
                        uint32_t hold_ms, int pwr_ctrl_pin, int led_pin) {
  touch_pin_ = touch_pin;
  active_level_ = active_level;
  pwr_ctrl_pin_ = pwr_ctrl_pin;
  led_pin_ = led_pin;
  set_hold_ms(hold_ms);

  // Release deep-sleep latch FIRST — gpio_hold_dis() is a no-op while
  // the deep-sleep hold is still active.
  gpio_deep_sleep_hold_dis();

  gpio_hold_dis(static_cast<gpio_num_t>(touch_pin_));

  if (pwr_ctrl_pin_ >= 0) {
    gpio_hold_dis(static_cast<gpio_num_t>(pwr_ctrl_pin_));
    configure_output_pin(static_cast<gpio_num_t>(pwr_ctrl_pin_), 0); // PNP ON
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

void PowerService::set_i2c_pins(int sda_pin, int scl_pin) {
  i2c_sda_pin_ = sda_pin;
  i2c_scl_pin_ = scl_pin;
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
      if (duration >= 50 && duration < hold_ms_) { // 50ms debounce
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
  // ================================================================
  // ACQUIRE A PM LOCK TO PREVENT TICKLESS IDLE (LIGHT SLEEP)
  // ================================================================
  // When CONFIG_PM_ENABLE + light_sleep_enable=true, FreeRTOS idle
  // enters light sleep during every vTaskDelay(). If we configure the
  // GPIO wakeup source before the wait loop, tickless idle wakes
  // instantly (touch is still held!) and corrupts the deep-sleep flow.
  //
  // Solution: take a CPU-speed PM lock that prevents light sleep
  // for the entire do_sleep() duration.
  // ================================================================
#if CONFIG_PM_ENABLE
  esp_pm_lock_handle_t pm_lock = nullptr;
  if (esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "do_sleep", &pm_lock) == ESP_OK) {
    esp_pm_lock_acquire(pm_lock);
  }
#endif

  // --- Step 1: Application-level sleep preparation callback ---
  if (sleep_prepare_callback_) {
    sleep_prepare_callback_();
  }

  // --- Step 2: Pull I2C lines LOW before cutting VCC_PERIPH ---
  // The on-board pull-up resistors sit on the switched VCC_PERIPH rail.
  // If SDA/SCL float HIGH while the PNP is off, current leaks through
  // ESP32's ESD diodes back into VCC_PERIPH (~2.7V), keeping the
  // OLED/IMU partially alive. Driving them LOW + hold breaks that path.
  drive_low_and_hold(i2c_sda_pin_);
  drive_low_and_hold(i2c_scl_pin_);

  // --- Step 3: Cut VCC_PERIPH (PNP OFF = GPIO HIGH) ---
  if (pwr_ctrl_pin_ >= 0) {
    drive_high_and_hold(pwr_ctrl_pin_);
  }

  // --- Step 4: LED off ---
  if (led_pin_ >= 0) {
    drive_high_and_hold(led_pin_);
  }

  // --- Step 5: Configure touch pin for wakeup ---
  const gpio_num_t touch_gpio = static_cast<gpio_num_t>(touch_pin_);
  configure_touch_inactive_level(touch_gpio, active_level_);
  gpio_hold_en(touch_gpio);

  // --- Step 6: Wait for user to release the button ---
  const uint32_t release_start_ms =
      static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
  while (pressed()) {
    const uint32_t loop_now =
        static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
    if (loop_now - release_start_ms > 5000U) {
      // User held too long — abort sleep, power everything back on
      ESP_LOGW(kTag, "sleep aborted: button held >5s");
      gpio_hold_dis(touch_gpio);
      if (pwr_ctrl_pin_ >= 0) {
        drive_low_output_and_hold(pwr_ctrl_pin_); // PNP ON
      }
      if (led_pin_ >= 0) {
        drive_high_and_hold(led_pin_);
      }
      press_start_ms_ = 0;
#if CONFIG_PM_ENABLE
      if (pm_lock) {
        esp_pm_lock_release(pm_lock);
        esp_pm_lock_delete(pm_lock);
      }
#endif
      return;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelay(pdMS_TO_TICKS(50));

  // --- Step 7: Configure GPIO wakeup (AFTER button released) ---
  const esp_sleep_gpio_wake_up_mode_t wake_mode =
      active_level_ == 0 ? ESP_GPIO_WAKEUP_GPIO_LOW
                         : ESP_GPIO_WAKEUP_GPIO_HIGH;
  esp_err_t err = esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown(
      (1ULL << touch_pin_), wake_mode);
  if (err != ESP_OK) {
    ESP_LOGE(kTag, "gpio wakeup config failed: %s", esp_err_to_name(err));
  }

  // --- Step 8: Enter deep sleep ---
  ESP_LOGI(kTag, "entering deep sleep...");
  gpio_deep_sleep_hold_en();

#if CONFIG_PM_ENABLE
  // Fully disable light sleep before entering deep sleep.
  // The PM lock alone isn't sufficient — we must also reconfigure PM
  // to prevent the FreeRTOS idle task from entering light sleep
  // during the brief window before esp_deep_sleep_start() executes.
  // Do NOT release the PM lock — esp_deep_sleep_start() never returns.
  esp_pm_config_t pm_off = {
      .max_freq_mhz = 80,
      .min_freq_mhz = 40,
      .light_sleep_enable = false,
  };
  esp_pm_configure(&pm_off);
#endif

  esp_deep_sleep_start();
  // Never returns.
}

} // namespace leor
