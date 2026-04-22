#include "leor/application.hpp"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <cstdio>
#include <cstdlib>

namespace leor {

namespace {

constexpr const char *kTag = "leor_app";
constexpr uint32_t kPowerOffMessageMs = 320;
constexpr uint32_t kOtaUiFrameMs = 33;
constexpr uint32_t kBleWindowMinMs = 20000;
constexpr uint32_t kBleWindowDefaultMs = 60000;

bool conflicts_with_display_i2c(int pin, const DisplayConfig &display) {
  return pin == display.sda_pin || pin == display.scl_pin;
}

void release_held_pin(int pin) {
  if (pin < 0) {
    return;
  }
  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  esp_rom_gpio_pad_select_gpio(gpio);
  gpio_hold_dis(gpio);
}

} // namespace

void Application::open_ble_window(uint32_t now_ms, bool start_advertising) {
  const bool should_start_advertising = !ble_window_open_;
  ble_window_open_ = true;
  ble_window_started_ms_ = now_ms;
  ble_window_duration_ms_ = std::max(kBleWindowMinMs, preferences_.getUInt("ble_win", kBleWindowDefaultMs));
  ble_window_deadline_ms_ = ble_window_started_ms_ + ble_window_duration_ms_;
  if (start_advertising && should_start_advertising) {
    ble_.start_advertising();
  }
}

void draw_ota_screen(DisplayBackend& display, int pct, const char* line1, const char* line2, uint32_t now_ms) {
  display.clear();
  display.set_font_small();

  bool is_error = (line1 != nullptr && std::string(line1).find("FAILED") != std::string::npos);

  if (is_error) {
    display.draw_frame(0, 0, 128, 64);
    display.fill_box(2, 2, 124, 11);
    display.set_color(0);
    const int tw = display.text_width("CRITICAL ERROR");
    display.draw_text((display.width() - tw) / 2, 11, "CRITICAL ERROR");
    display.set_color(1);
    display.set_font_medium();
    const char* msg = line2 ? line2 : "UNKNOWN";
    const int w2 = display.text_width(msg);
    display.draw_text((display.width() - w2) / 2, 40, msg);
  } else if (pct >= 0) {
    // 1. Header (Tightened)
    display.fill_box(0, 0, 128, 11);
    display.set_color(0);
    display.draw_text(4, 9, "RE-FLASHING SYSTEM...");
    display.set_color(1);

    // 2. Data Section (Grid layout)
    display.set_font_medium();
    char pct_buf[12];
    std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", pct);
    display.draw_text(4, 32, pct_buf);

    display.set_font_small();
    if (line2) {
      // Shorter label 'DATA:' and fixed-position values to prevent out-of-bounds
      display.draw_text(60, 22, "DATA:");
      display.draw_text(60, 33, line2);
    }

    // 3. Robust Progress Bar (Moved up slightly)
    display.draw_frame(2, 40, 124, 10);
    const int fill_w = (120 * pct) / 100;
    if (fill_w > 0) {
      display.fill_box(4, 42, fill_w, 6);
      display.set_color(0);
      for (int i = 4; i < 4 + fill_w; i += 5) {
        display.draw_vline(i, 42, 6);
      }
      display.set_color(1);
    }

    // 4. Footer (Moved to bottom pixel)
    if (pct == 100) {
      display.draw_text(30, 62, "[VERIFICATION OK]");
    } else {
      const int dots = (now_ms / 400) % 4;
      char dots_s[8] = "----";
      if (dots > 0) {
        for(int i=0; i<dots; ++i) dots_s[i] = '>';
        dots_s[dots] = '\0';
      }
      display.draw_text(4, 62, "STATUS: BUSY");
      display.draw_text(80, 62, dots_s);
    }
  }

  display.send_buffer();
}

Application::Application() = default;

esp_err_t Application::start() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    ESP_LOGE(kTag, "nvs init failed: %s", esp_err_to_name(err));
    return err;
  }

#if CONFIG_PM_ENABLE
  esp_pm_config_t pm_config = {
      .max_freq_mhz = 80, .min_freq_mhz = 40, .light_sleep_enable = true};
  ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
#endif

  std::srand(esp_timer_get_time() & 0xffffffff);
  ESP_ERROR_CHECK(preferences_.begin("leor"));

  config_.display.controller =
      preferences_.getString("disp_type", "ssd1306") == "sh1106"
          ? DisplayController::kSh1106
          : DisplayController::kSsd1306;
  config_.display.i2c_address =
      static_cast<uint8_t>(preferences_.getUInt("disp_addr", 0x3c));
  config_.touch_wake_pin =
      static_cast<uint8_t>(preferences_.getUInt("wake_pin", 0));
  config_.touch_active_level = 1;
  config_.touch_hold_ms = preferences_.getUInt("touch_ms", 3000);
  config_.pwr_ctrl_pin = static_cast<int>(preferences_.getUInt("pwr_pin", 1));

  if (conflicts_with_display_i2c(static_cast<int>(config_.touch_wake_pin),
                                 config_.display)) {
    ESP_LOGW(kTag,
             "wake pin %d conflicts with display I2C pins (SDA=%d, SCL=%d), "
             "reverting to 0",
             static_cast<int>(config_.touch_wake_pin), config_.display.sda_pin,
             config_.display.scl_pin);
    config_.touch_wake_pin = 0;
    preferences_.putUInt("wake_pin", 0);
  }

  if (conflicts_with_display_i2c(config_.pwr_ctrl_pin, config_.display)) {
    ESP_LOGW(kTag,
             "power control pin %d conflicts with display I2C pins (SDA=%d, "
             "SCL=%d), disabling power pin",
             config_.pwr_ctrl_pin, config_.display.sda_pin,
             config_.display.scl_pin);
    config_.pwr_ctrl_pin = -1;
  }

  gpio_deep_sleep_hold_dis();
  release_held_pin(config_.display.sda_pin);
  release_held_pin(config_.display.scl_pin);

  power_.init(config_.touch_wake_pin, config_.touch_active_level,
              config_.touch_hold_ms, config_.pwr_ctrl_pin, config_.led_pin);
  power_.set_i2c_pins(config_.display.sda_pin, config_.display.scl_pin);
  power_.arm(1000, 0);

  display_ = std::make_unique<U8g2DisplayBackend>();
  if (!display_->init(config_.display)) {
    ESP_LOGW(kTag,
             "display init failed, falling back to null backend (%s, SDA=%d, "
             "SCL=%d, addr=0x%02x)",
             config_.display.controller == DisplayController::kSsd1306
                 ? "ssd1306"
                 : "sh1106",
             config_.display.sda_pin, config_.display.scl_pin,
             config_.display.i2c_address);
    display_ = std::make_unique<NullDisplayBackend>();
    display_->init(config_.display);
  }

  eyes_ = std::make_unique<MochiEyesEngine>(*display_);
  eyes_->begin();
  eyes_->set_width(preferences_.getInt("ew", 36),
                   preferences_.getInt("ew", 36));
  eyes_->set_height(preferences_.getInt("eh", 36),
                    preferences_.getInt("eh", 36));
  eyes_->set_space_between(preferences_.getInt("es", 10));
  eyes_->set_border_radius(preferences_.getInt("er", 8),
                           preferences_.getInt("er", 8));
  eyes_->set_mouth_size(preferences_.getInt("mw", 20), 6);
  eyes_->setGazeSpeed(static_cast<float>(preferences_.getInt("gs", 6)));
  eyes_->setOpennessSpeed(static_cast<float>(preferences_.getInt("os", 12)));
  eyes_->setSquishSpeed(static_cast<float>(preferences_.getInt("ss", 10)));
  eyes_->set_breathing(preferences_.getBool("br_en", true),
                       preferences_.getFloat("br_int", 0.08f),
                       preferences_.getFloat("br_spd", 0.3f));

  gesture_.start(config_.gesture_dummy_enabled, config_.display.sda_pin,
                 config_.display.scl_pin, display_.get());
  gesture_.restore(preferences_.getBool("gm", true),
                   preferences_.getUInt("grt", 1500),
                   preferences_.getUInt("gcf", 70),
                   preferences_.getUInt("gcd", 1500),
                   preferences_.getString("ga", "happy,angry,curious,neutral,love"));
  gesture_.set_inverted(preferences_.getBool("ginv", false));
  gesture_.set_shake_threshold(preferences_.getFloat("gst", 200.0f));
  gesture_.set_pat_threshold(preferences_.getFloat("gpt", 0.32f));
  gesture_.set_swipe_threshold(preferences_.getFloat("gvt", 0.45f));
  gesture_.set_touch_threshold(preferences_.getFloat("gtt", 0.05f));

  shuffle_.restore(preferences_.getBool("shuf_en", true),
                   preferences_.getUInt("shuf_emin", 2000),
                   preferences_.getUInt("shuf_emax", 5000),
                   preferences_.getUInt("shuf_nmin", 2000),
                   preferences_.getUInt("shuf_nmax", 5000));
  clock_.restore(preferences_.getBool("clk_on", false),
                 preferences_.getBool("clk_24", true),
                 static_cast<int16_t>(preferences_.getInt("clk_tz", 0)),
                 preferences_.getULong64("clk_epoch", 0),
                 preferences_.getUInt("clk_sec", 0));
  was_clock_enabled_ = clock_.enabled();

  commands_ = std::make_unique<CommandRouter>(preferences_, config_.display,
                                              *display_, *eyes_, gesture_,
                                              shuffle_, clock_, power_, ble_);

  const std::string ble_name = preferences_.getString("ble_name", "Leor");
  ESP_ERROR_CHECK(ble_.start(ble_name, [this](const std::string &cmd) {
    const uint32_t now_ms =
        static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    return commands_->handle(cmd, now_ms);
  }));
  open_ble_window(static_cast<uint32_t>(esp_timer_get_time() / 1000ULL), false);
  display_->set_contrast(static_cast<uint8_t>(preferences_.getUInt("disp_con", 0x7f)));

  ESP_LOGI(kTag, "application started");
  return ESP_OK;
}

void Application::tick() {
  uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  ble_.poll();
  const bool ota_active = ble_.ota().in_progress() || ble_.ota().reboot_pending() || ble_.ota().error_pending();

  if (ble_window_open_) {
    const uint32_t desired_duration_ms = std::max(kBleWindowMinMs, preferences_.getUInt("ble_win", kBleWindowDefaultMs));
    if (desired_duration_ms != ble_window_duration_ms_) {
      ble_window_duration_ms_ = desired_duration_ms;
      ble_window_deadline_ms_ = ble_window_started_ms_ + ble_window_duration_ms_;
    }
  }

  if (ble_window_open_ && !ota_active && now_ms >= ble_window_deadline_ms_) {
    ble_.stop(false);
    ble_window_open_ = false;
  }

  // --- OTA Priority Bypass ---
  // If an OTA update is active or finished and waiting to reboot, we suspend
  // all normal rendering and logic (IMU, gestures, splines) to speed up BLE transfer.
  if (ota_active) {
    if (display_) {
      if (ble_.ota().error_pending()) {
        draw_ota_screen(*display_, 0, "OTA FAILED", ble_.ota().error_message() ? ble_.ota().error_message() : "Unknown", now_ms);
      } else {
        int pct = ble_.ota().progress_percent();
        if (ble_.ota().reboot_pending()) {
          pct = 100;
          draw_ota_screen(*display_, pct, "OTA SUCCESS", "Rebooting...", now_ms);
        } else {
          char msg[48];
          const uint32_t kb_done = ble_.ota().bytes_received() / 1024U;
          if (ble_.ota().progress_known()) {
            const uint32_t kb_total = ble_.ota().expected_size() / 1024U;
            std::snprintf(msg, sizeof(msg), "%lu/%lu KB",
                          static_cast<unsigned long>(kb_done),
                          static_cast<unsigned long>(kb_total));
          } else {
            std::snprintf(msg, sizeof(msg), "%lu KB",
                          static_cast<unsigned long>(kb_done));
          }
          draw_ota_screen(*display_, pct, nullptr, msg, now_ms);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(kOtaUiFrameMs));
    return;
  }
  // ---------------------------

  ButtonEvent btn = power_.poll(now_ms);
  if (btn == ButtonEvent::kShortPress) {
    if (now_ms - last_short_press_ms_ < kDoubleTapThresholdMs) {
      // Double tap detected
      if (menu_.is_open()) {
        menu_.close();
      }
      last_short_press_ms_ = 0; // Reset
    } else {
      last_short_press_ms_ = now_ms;
      open_ble_window(now_ms, true);
      if (menu_.is_open()) {
        menu_.on_short_press(now_ms);
      }
    }
  } else if (btn == ButtonEvent::kLongPress) {
    menu_.on_long_press(now_ms);
    last_short_press_ms_ = 0;
  }

  if (menu_.is_open() && (now_ms - menu_.last_activity_ms() > MenuService::kTimeoutMs)) {
    menu_.close();
  }

  MenuAction action = menu_.consume_action();
  switch (action) {
  case MenuAction::kToggleMode: {
    bool new_state = !clock_.enabled();
    clock_.set_enabled(new_state);
    preferences_.putBool("clk_on", new_state);
    break;
  }
  case MenuAction::kPowerOff:
    if (display_ && eyes_) {
      bool was_shuffle = shuffle_.enabled();
      if (was_shuffle) shuffle_.set_enabled(false);

      eyes_->triggerSleep();
      uint32_t start_ms = now_ms;
      while (!eyes_->is_sleep_done()) {
        uint32_t loop_ms =
            static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        display_->clear();
        eyes_->update(loop_ms);
        vTaskDelay(pdMS_TO_TICKS(16));
        if (loop_ms - start_ms > 4000)
          break; // failsafe
      }
      display_->prepare_sleep();
      ble_.stop();
      power_.do_sleep();
      
      // If do_sleep returns, the sleep was aborted by user holding button too long!
      if (was_shuffle) shuffle_.set_enabled(true);
      eyes_->reset_emotions();
      return;
    } else if (display_) {
      display_->clear();
      display_->send_buffer();
      display_->prepare_sleep();
      ble_.stop();
      power_.do_sleep();
    }
    return;
  default:
    break;
  }
  const std::string gesture_cmd = gesture_.poll(now_ms, power_.is_pressed());
  if (!gesture_cmd.empty() && !clock_.enabled() && !menu_.is_open()) {
    commands_->handle(gesture_cmd, now_ms);
  }

  // --- Tilt Compensation (Passive) ---
  if (!clock_.enabled() && !menu_.is_open() && eyes_) {
    if (gesture_.matching_enabled() && !gesture_.suspended()) {
      float p = gesture_.pitch();
      float r = gesture_.roll();
      if (std::abs(p) > 10.0f || std::abs(r) > 10.0f) {
          // Simple mapping: 45 deg tilt = 0.5 gaze offset
          float gx = std::clamp(-r / 45.0f, -0.6f, 0.6f);
          float gy = std::clamp(-p / 45.0f, -0.6f, 0.6f);
          eyes_->set_gaze_manual(gx, gy);
      } else {
          eyes_->set_gaze_manual(0.0f, 0.0f); 
      }
    } else {
      // Force neutral if gestures are disabled/suspended
      eyes_->set_gaze_manual(0.0f, 0.0f);
    }
  }
  // -----------------------------------

  const char *shuffle_cmd = nullptr;
  if (!clock_.enabled() && shuffle_.should_emit(now_ms, false, false, &shuffle_cmd) &&
      shuffle_cmd != nullptr) {
    commands_->handle(shuffle_cmd, now_ms, false);
  }

  const bool is_clock_enabled = clock_.enabled();
  const bool is_suspended = is_clock_enabled || menu_.is_open();
  gesture_.set_suspended(is_suspended);

  if (is_clock_enabled != was_clock_enabled_) {
    display_->clear();
    display_->send_buffer();
    was_clock_enabled_ = is_clock_enabled;
  }

  if (menu_.is_open() != was_menu_open_) {
    if (menu_.is_open()) {
      power_.set_hold_ms(1000);
    } else {
      power_.set_hold_ms(config_.touch_hold_ms);
    }
    display_->clear();
    display_->send_buffer();
    was_menu_open_ = menu_.is_open();
  }

  if (menu_.is_open()) {
    display_->clear();
    menu_.draw(*display_, is_clock_enabled, now_ms);
    display_->send_buffer();
  } else {
    if (is_clock_enabled) {
      clock_.draw(*display_, ble_.connected());
    } else {
      eyes_->update(now_ms);
    }
  }
}

} // namespace leor
