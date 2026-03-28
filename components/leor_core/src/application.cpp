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
constexpr uint32_t kBleWindowMs = 60000;

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
  ble_window_deadline_ms_ = now_ms + kBleWindowMs;
  if (start_advertising && should_start_advertising) {
    ble_.start_advertising();
  }
}

void draw_ota_screen(DisplayBackend& display, int pct, const char* line1, const char* line2, uint32_t now_ms) {
  const char* title = line1 != nullptr ? line1 : "OTA UPDATE";
  display.clear();
  display.set_font_small();

  const int title_w = display.text_width(title);
  display.draw_text((display.width() - title_w) / 2, 10, title);
  display.draw_hline(0, 12, display.width());

  if (line2 != nullptr) {
    const int sub_w = display.text_width(line2);
    display.draw_text((display.width() - sub_w) / 2, 22, line2);
  }

  if (pct >= 0) {
    constexpr int kBarX = 4;
    constexpr int kBarY = 28;
    constexpr int kBarW = 120;
    constexpr int kBarH = 14;
    display.draw_frame(kBarX, kBarY, kBarW, kBarH);
    const int fill = ((kBarW - 4) * pct) / 100;
    if (fill > 0) {
      display.fill_rbox(kBarX + 2, kBarY + 2, fill, kBarH - 4, 2);
    }

    char pct_buf[12];
    std::snprintf(pct_buf, sizeof(pct_buf), "%d%%", pct);
    const int pct_w = display.text_width(pct_buf);
    display.set_color(pct > 40 ? 0 : 1);
    display.draw_text((display.width() - pct_w) / 2, kBarY + 11, pct_buf);
    display.set_color(1);
  }

  if (pct >= 0 && pct < 100) {
    char dots[5] = "";
    const int dot_count = static_cast<int>((now_ms / 400U) % 4U);
    for (int i = 0; i < dot_count; ++i) {
      dots[i] = '.';
    }
    dots[dot_count] = '\0';
    display.draw_text(4, 50, "Flashing");
    if (dot_count > 0) {
      display.draw_text(58, 50, dots);
    }
  } else if (pct == 100) {
    display.draw_text(36, 50, "Rebooting...");
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
      .max_freq_mhz = 80, .min_freq_mhz = 40, .light_sleep_enable = false};
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
  eyes_->set_breathing(preferences_.getBool("br_en", true),
                       preferences_.getFloat("br_int", 0.08f),
                       preferences_.getFloat("br_spd", 0.3f));

  gesture_.start(config_.gesture_dummy_enabled, config_.display.sda_pin,
                 config_.display.scl_pin, display_.get());
  shuffle_.restore(preferences_.getBool("shuf_en", true),
                   preferences_.getUInt("shuf_emin", 2000),
                   preferences_.getUInt("shuf_emax", 5000),
                   preferences_.getUInt("shuf_nmin", 2000),
                   preferences_.getUInt("shuf_nmax", 5000));
  clock_.restore(preferences_.getBool("clk_on", false),
                 preferences_.getBool("clk_24", true),
                 preferences_.getUInt("clk_sec", 0),
                 static_cast<int16_t>(preferences_.getInt("clk_tz", 0)),
                 preferences_.getULong64("clk_epoch", 0),
                 static_cast<uint32_t>(esp_timer_get_time() / 1000ULL));
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
  ble_.set_low_power_mode(preferences_.getBool("ble_lp", false));
  open_ble_window(static_cast<uint32_t>(esp_timer_get_time() / 1000ULL), false);
  display_->set_contrast(0x7f);

  ESP_LOGI(kTag, "application started");
  return ESP_OK;
}

void Application::tick() {
  uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  ble_.poll();

  if (ble_window_open_ && now_ms >= ble_window_deadline_ms_) {
    ble_.stop();
    ble_window_open_ = false;
  }

  // --- OTA Priority Bypass ---
  // If an OTA update is active or finished and waiting to reboot, we suspend
  // all normal rendering and logic (IMU, gestures, splines) to speed up BLE transfer.
  if (ble_.ota().in_progress() || ble_.ota().reboot_pending() || ble_.ota().error_pending()) {
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
          const uint32_t pkts = ble_.ota().packets_received();
          if (ble_.ota().progress_known()) {
            const uint32_t kb_total = ble_.ota().expected_size() / 1024U;
            std::snprintf(msg, sizeof(msg), "%lu / %lu KB (%lu)",
                          static_cast<unsigned long>(kb_done),
                          static_cast<unsigned long>(kb_total),
                          static_cast<unsigned long>(pkts));
          } else {
            std::snprintf(msg, sizeof(msg), "%lu KB (%lu pkts)",
                          static_cast<unsigned long>(kb_done),
                          static_cast<unsigned long>(pkts));
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
    open_ble_window(now_ms, true);
    menu_.on_short_press(now_ms);
  } else if (btn == ButtonEvent::kLongPress) {
    menu_.on_long_press(now_ms);
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
  const GestureEvent event = gesture_.poll(now_ms);

  switch (event) {
  case GestureEvent::kPat:
    eyes_->set_mood(HAPPY);
    break;
  case GestureEvent::kShake:
    eyes_->set_mood(ANGRY);
    break;
  case GestureEvent::kSwipe:
    eyes_->set_position(POS_E);
    break;
  case GestureEvent::kPickup:
    eyes_->set_mood(DEFAULT);
    eyes_->set_position(POS_N);
    break;
  case GestureEvent::kNone:
  default:
    break;
  }

  const char *shuffle_cmd = nullptr;
  if (!clock_.enabled() && shuffle_.should_emit(now_ms, false, false, &shuffle_cmd) &&
      shuffle_cmd != nullptr) {
    commands_->handle(shuffle_cmd, now_ms, false);
  }

  const bool is_clock_enabled = clock_.enabled();
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
      clock_.draw(*display_, now_ms, ble_.connected());
    } else {
      eyes_->update(now_ms);
    }
  }
}

} // namespace leor
