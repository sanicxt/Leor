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
constexpr uint32_t kTouchDetectWindowMs = 5000;

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
    display.draw_text((display.width() - tw) / 2, 9, "CRITICAL ERROR");
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

enum class BootStage { kDisplay, kGyro, kTouch, kBuzzer, kPower, kWifi, kDone };

void draw_boot_screen(DisplayBackend& disp, BootStage stage, const HardwareStatus& hw,
                      int gyro_pct, bool touch_waiting, bool wifi_waiting, bool wifi_ok,
                      uint32_t now_ms) {
  (void)now_ms;
  disp.clear();

  const int stage_count = static_cast<int>(BootStage::kDone);
  const int overall_pct = ((static_cast<int>(stage) + 1) * 100) / stage_count;

  disp.set_font_small();
  disp.draw_text(2, 10, "LEOR");
  char pct_s[24];
  std::snprintf(pct_s, sizeof(pct_s), "%d%%", overall_pct);
  const int pw = disp.text_width(pct_s);
  disp.draw_text(disp.width() - 2 - pw, 10, pct_s);

  disp.draw_frame(2, 14, 124, 10);
  const int fill_w = (120 * overall_pct) / 100;
  if (fill_w > 0) disp.fill_box(4, 16, fill_w, 6);

  const char* label = "";
  switch (stage) {
    case BootStage::kDisplay: label = "DISPLAY"; break;
    case BootStage::kGyro:    label = "GYRO";    break;
    case BootStage::kTouch:   label = "TOUCH";   break;
    case BootStage::kBuzzer:  label = "BUZZER";  break;
    case BootStage::kPower:   label = "POWER";   break;
    case BootStage::kWifi:    label = "WIFI";    break;
    default: break;
  }
  disp.set_font_medium();
  const int lw = disp.text_width(label);
  disp.draw_text((disp.width() - lw) / 2, 40, label);
  disp.set_font_small();

  if (touch_waiting && stage == BootStage::kGyro) {
    std::snprintf(pct_s, sizeof(pct_s), "CAL %d%%  PRESS", gyro_pct);
  } else if (touch_waiting) {
    std::snprintf(pct_s, sizeof(pct_s), "PRESS TOUCH");
  } else if (stage == BootStage::kGyro) {
    if (hw.gyro == HwState::kProbeFailed) {
      std::snprintf(pct_s, sizeof(pct_s), "FAIL");
    } else if (hw.gyro == HwState::kAbsent) {
      std::snprintf(pct_s, sizeof(pct_s), "ABSENT");
    } else {
      std::snprintf(pct_s, sizeof(pct_s), "CAL %d%%", gyro_pct);
    }
  } else if (stage == BootStage::kDisplay) {
    std::snprintf(pct_s, sizeof(pct_s), "%s", hw.display == HwState::kPresent ? "OK" : "FAIL");
  } else if (stage == BootStage::kBuzzer) {
    std::snprintf(pct_s, sizeof(pct_s), "%s", hw.buzzer == HwState::kPresent ? "OK" : "ABSENT");
  } else if (stage == BootStage::kPower) {
    std::snprintf(pct_s, sizeof(pct_s), "%s", hw.power == HwState::kPresent ? "OK" : "ABSENT");
  } else if (stage == BootStage::kTouch) {
    std::snprintf(pct_s, sizeof(pct_s), "%s", hw.touch == HwState::kPresent ? "OK" : "ABSENT");
  } else if (stage == BootStage::kWifi) {
    std::snprintf(pct_s, sizeof(pct_s), "%s", wifi_waiting ? "SYNC..." : (wifi_ok ? "SYNCED" : "FAIL"));
  }
  const int hint_w = disp.text_width(pct_s);
  disp.draw_text((disp.width() - hint_w) / 2, 58, pct_s);

  disp.send_buffer();
}

constexpr size_t kTouchCandidatesCount = 10;
constexpr uint8_t kTouchCandidates[kTouchCandidatesCount] = {0, 1, 3, 4, 5, 6, 7, 10, 20, 21};

bool pin_in_use(int pin, const DisplayConfig& display, int buzzer_pin,
                int pwr_ctrl_pin) {
  if (pin == display.sda_pin || pin == display.scl_pin) return true;
  if (pin == buzzer_pin) return true;
  if (pin == pwr_ctrl_pin) return true;
  return false;
}

// poll() is invoked from inside the gyro calibration loop so one 5s window
// runs both concurrently. Pull resistor is set opposite to active_level so
// unwired pins read inactive; a pin needs kDebounceSamples consecutive active
// reads to be accepted (prevents floating-pin false detects).
struct TouchProbe {
  // 255 = not detected yet. GPIO 0 is a valid touch pin, so it cannot be
  // the 'no detection' sentinel.
  static constexpr uint8_t kNotDetected = 255;
  uint8_t active_level = 1;
  bool configured[kTouchCandidatesCount] = {};
  uint8_t stable[kTouchCandidatesCount] = {};
  uint8_t detected_pin = kNotDetected;

  void setup(const DisplayConfig& display, int buzzer_pin, int pwr_ctrl_pin,
             uint8_t level) {
    active_level = level;
    for (size_t i = 0; i < kTouchCandidatesCount; ++i) {
      const int pin = kTouchCandidates[i];
      if (pin_in_use(pin, display, buzzer_pin, pwr_ctrl_pin)) continue;
      gpio_config_t io = {};
      io.pin_bit_mask = (1ULL << pin);
      io.mode = GPIO_MODE_INPUT;
      io.intr_type = GPIO_INTR_DISABLE;
      if (active_level == 1) {
        io.pull_up_en = GPIO_PULLUP_DISABLE;
        io.pull_down_en = GPIO_PULLDOWN_ENABLE;
      } else {
        io.pull_up_en = GPIO_PULLUP_ENABLE;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
      }
      if (gpio_config(&io) == ESP_OK) configured[i] = true;
    }
  }

  uint8_t poll() {
    if (detected_pin != kNotDetected) return detected_pin;
    constexpr uint32_t kDebounceSamples = 5;
    for (size_t i = 0; i < kTouchCandidatesCount; ++i) {
      if (!configured[i]) continue;
      const int pin = kTouchCandidates[i];
      if (gpio_get_level(static_cast<gpio_num_t>(pin)) ==
          static_cast<int>(active_level)) {
        if (++stable[i] >= kDebounceSamples) {
          detected_pin = static_cast<uint8_t>(pin);
          return detected_pin;
        }
      } else {
        stable[i] = 0;
      }
    }
    return kNotDetected;
  }
};

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
      static_cast<uint8_t>(preferences_.getUInt("wake_pin", 255));
  config_.touch_active_level = 1;
  config_.touch_hold_ms = preferences_.getUInt("touch_ms", 3000);
  config_.pwr_ctrl_pin = static_cast<int>(preferences_.getUInt("pwr_pin", 1));

  const std::string touch_mode = preferences_.getString("touch_mode", "detect");
  const std::string buzzer_mode = preferences_.getString("buzzer_mode", "off");
  const std::string wifi_mode = preferences_.getString("wifi_mode", "off");

  if (touch_mode == "off" || touch_mode == "detect") {
    config_.touch_wake_pin = 255;
  }

  if (buzzer_mode == "off") {
    config_.buzzer_pin = -1;
  } else {
    config_.buzzer_pin =
        static_cast<int>(preferences_.getUInt("buz_pin", config_.buzzer_pin));
  }

  if (config_.touch_wake_pin != 255 &&
      conflicts_with_display_i2c(static_cast<int>(config_.touch_wake_pin),
                                 config_.display)) {
    ESP_LOGW(kTag,
             "wake pin %d conflicts with display I2C pins (SDA=%d, SCL=%d), "
             "reverting to disabled",
             static_cast<int>(config_.touch_wake_pin), config_.display.sda_pin,
             config_.display.scl_pin);
    config_.touch_wake_pin = 255;
    preferences_.putUInt("wake_pin", 255);
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
  if (config_.buzzer_pin >= 0) release_held_pin(config_.buzzer_pin);

  power_.init(config_.touch_wake_pin, config_.touch_active_level,
              config_.touch_hold_ms, config_.pwr_ctrl_pin, config_.led_pin);
  power_.set_i2c_pins(config_.display.sda_pin, config_.display.scl_pin);
  power_.arm(1000, 0);

  hw_.touch = power_.touch_enabled() ? HwState::kPresent : HwState::kAbsent;
  hw_.power = power_.power_control_enabled() ? HwState::kPresent : HwState::kAbsent;

  display_ = std::make_unique<U8g2DisplayBackend>();
  const bool display_ok = display_->init(config_.display);
  if (!display_ok) {
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
  hw_.display = display_ok ? HwState::kPresent : HwState::kProbeFailed;
  const uint32_t boot_now = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  if (display_ok) draw_boot_screen(*display_, BootStage::kDisplay, hw_, 0, false, false, false, boot_now);
  if (display_ok) vTaskDelay(pdMS_TO_TICKS(150));

  // Gyro calibration and touch auto-detection run concurrently in one 5s
  // window. The touch_probe callback is polled each IMU calibration iteration;
  // a display=nullptr suppresses init_mpu's internal screen so the LEOR boot
  // screen stays visible. Running them together avoids a separate 10s touch
  // window and the earlier race where MPU I2C traffic was misread as a press.
  TouchProbe touch_probe{};
  const bool want_touch_detect = (touch_mode == "detect");
  if (want_touch_detect) {
    touch_probe.setup(config_.display, config_.buzzer_pin,
                      config_.pwr_ctrl_pin, config_.touch_active_level);
  }

  const uint32_t touch_window_start_ms =
      static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  if (display_ok)
    draw_boot_screen(*display_, BootStage::kGyro, hw_, 0, want_touch_detect,
                     false, false, touch_window_start_ms);

  std::function<uint8_t(int)> probe_cb;
  if (want_touch_detect) {
    probe_cb = [this, &touch_probe, display_ok, last = 0](int pct) mutable -> uint8_t {
      if (display_ok && (pct >= last + 4 || pct >= 100)) {
        last = pct;
        draw_boot_screen(*display_, BootStage::kGyro, hw_, pct, true, false, false,
                         static_cast<uint32_t>(esp_timer_get_time() / 1000ULL));
      }
      return touch_probe.poll();
    };
  }
  gesture_.start(config_.gesture_dummy_enabled, config_.display.sda_pin,
                 config_.display.scl_pin, nullptr, probe_cb);
  if (gesture_.mpu_available()) {
    hw_.gyro = HwState::kPresent;
  } else if (gesture_.mpu_whoami_ok()) {
    hw_.gyro = HwState::kProbeFailed;
  } else {
    hw_.gyro = HwState::kAbsent;
  }
  gesture_.restore(preferences_.getBool("gm", true),
                    preferences_.getUInt("grt", 1500),
                    preferences_.getUInt("gcf", 70),
                    preferences_.getUInt("gcd", 1500),
                    preferences_.getString("ga", "happy,angry,curious,neutral"));
  gesture_.set_inverted(preferences_.getBool("ginv", false));
  gesture_.set_shake_threshold(preferences_.getFloat("gst", 200.0f));
  gesture_.set_pat_threshold(preferences_.getFloat("gpt", 0.32f));
  gesture_.set_swipe_threshold(preferences_.getFloat("gvt", 0.45f));
  gesture_.set_touch_threshold(preferences_.getFloat("gtt", 0.05f));
  gesture_.set_pickup_tilt_deg(preferences_.getFloat("gtd", 30.0f));

  if (want_touch_detect && touch_probe.detected_pin == TouchProbe::kNotDetected) {
    const uint32_t window_deadline_ms =
        static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) + kTouchDetectWindowMs;
    uint32_t last_draw_ms = 0;
    while (touch_probe.detected_pin == TouchProbe::kNotDetected) {
      const uint32_t now_ms =
          static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
      if (now_ms >= window_deadline_ms) break;
      if (display_ok && now_ms - last_draw_ms >= 100) {
        last_draw_ms = now_ms;
        draw_boot_screen(*display_, BootStage::kTouch, hw_, 100, true, false, false, now_ms);
      }
      touch_probe.poll();
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }

  if (want_touch_detect) {
    const uint8_t detected = touch_probe.detected_pin;
    if (detected != TouchProbe::kNotDetected) {
      config_.touch_wake_pin = detected;
      preferences_.putUInt("wake_pin", detected);
      ESP_LOGW(kTag, "auto-detected touch pin %u", detected);
      power_.init(detected, config_.touch_active_level, config_.touch_hold_ms,
                  config_.pwr_ctrl_pin, config_.led_pin);
      power_.set_i2c_pins(config_.display.sda_pin, config_.display.scl_pin);
      power_.arm(1000, 0);
      hw_.touch = HwState::kPresent;
    }
  }

  if (display_ok)
    draw_boot_screen(*display_, BootStage::kGyro, hw_, 100, false, false, false,
                     static_cast<uint32_t>(esp_timer_get_time() / 1000ULL));
  if (display_ok) vTaskDelay(pdMS_TO_TICKS(200));
  if (display_ok)
    draw_boot_screen(*display_, BootStage::kTouch, hw_, 100, false, false, false,
                     static_cast<uint32_t>(esp_timer_get_time() / 1000ULL));
  if (display_ok) vTaskDelay(pdMS_TO_TICKS(200));

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

  if (conflicts_with_display_i2c(config_.buzzer_pin, config_.display)) {
    ESP_LOGW(kTag, "buzzer pin %d conflicts with I2C, disabling buzzer", config_.buzzer_pin);
    config_.buzzer_pin = -1;
  }
  buzzer_.init(config_.buzzer_pin);
  hw_.buzzer = buzzer_.initialized() ? HwState::kPresent : HwState::kAbsent;
  buzzer_.play_power_on();
  if (display_ok)
    draw_boot_screen(*display_, BootStage::kBuzzer, hw_, 100, false, false, false,
                     static_cast<uint32_t>(esp_timer_get_time() / 1000ULL));
  if (display_ok) vTaskDelay(pdMS_TO_TICKS(200));

  if (display_ok)
    draw_boot_screen(*display_, BootStage::kPower, hw_, 100, false, false, false,
                     static_cast<uint32_t>(esp_timer_get_time() / 1000ULL));
  if (display_ok) vTaskDelay(pdMS_TO_TICKS(200));

  wifi_.init(preferences_);
  wifi_.set_time_callback([this](uint64_t epoch_ms) {
    clock_.set_from_epoch_ms(epoch_ms, static_cast<int16_t>(preferences_.getInt("clk_tz", 0)));
  });

  if (wifi_mode == "on" && wifi_.configured()) {
    const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    draw_boot_screen(*display_, BootStage::kWifi, hw_, 100, false, true, false, now_ms);
    const bool wifi_ok = wifi_.sync_blocking(20000);
    draw_boot_screen(*display_, BootStage::kWifi, hw_, 100, false, false, wifi_ok, now_ms);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

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
                                               shuffle_, clock_, power_, ble_,
                                               buzzer_, hw_, wifi_);
  commands_->set_notif_overlay(&notif_);
  commands_->set_buzzer_callback([this](bool on) {
      if (on) buzzer_.play_toggle_on();
      else buzzer_.play_toggle_off();
  });

  power_.set_sleep_prepare_callback([this]() {
      buzzer_.stop();
      buzzer_.prepare_sleep();
  });

  const std::string ble_name = preferences_.getString("ble_name", "Leor");
  ESP_ERROR_CHECK(ble_.start(ble_name, [this](const std::string &cmd) {
    const uint32_t now_ms =
        static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    return commands_->handle(cmd, now_ms);
  }));
  // Advertise at boot for the BLE window; touch re-opens it later.
  open_ble_window(static_cast<uint32_t>(esp_timer_get_time() / 1000ULL), true);
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

  if (hw_.touch == HwState::kAbsent) {
    if (!ble_.advertising_enabled()) {
      ble_window_open_ = true;
      ble_.start_advertising();
    }
  } else if (ble_window_open_ && !ota_active && !ble_.connected() &&
             now_ms >= ble_window_deadline_ms_) {
    ble_.stop(false);
    ble_window_open_ = false;
  }

  // Re-open the BLE window when a connection drops so the device stays
  // discoverable without needing a touch press.
  const bool ble_connected = ble_.connected();
  if (was_ble_connected_ && !ble_connected && !ota_active) {
    open_ble_window(now_ms, true);
  }
  was_ble_connected_ = ble_connected;

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

      buzzer_.play_power_off();
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

  // --- Notification Overlay Bypass ---
  if (notif_.active) {
    if (notif_.expired(now_ms)) {
      notif_.dismiss();
    } else {
      // Button dismisses msg and calendar, but NOT calls
      if (btn != ButtonEvent::kNone && notif_.type != NotificationType::kCall) {
        notif_.dismiss();
      }
      if (notif_.active) {
        display_->clear();
        draw_notification(*display_, notif_, now_ms);
        display_->send_buffer();
        return;
      }
    }
  }
  // -----------------------------------

  if (gesture_.calibrating()) {
    std::string cal_result = gesture_.calibration_tick(now_ms, power_.is_pressed());
    if (!cal_result.empty()) {
      // Persist calibrated thresholds
      int idx = gesture_.calibration_gesture_index();
      float new_thresh = gesture_.calibration_new_threshold();
      switch (idx) {
        case 0: preferences_.putFloat("gpt", new_thresh); break;
        case 1: preferences_.putFloat("gst", new_thresh); break;
        case 2: preferences_.putFloat("gvt", new_thresh); break;
        case 3: preferences_.putFloat("gtd", new_thresh); break;
      }
      ble_.notify_status(cal_result);
      gesture_.abort_calibration();
    }
    // Draw calibration screen on OLED
    if (gesture_.calibrating() && display_) {
      const char* gesture_name = "unknown";
      switch (gesture_.calibration_gesture_index()) {
        case 0: gesture_name = "Pat"; break;
        case 1: gesture_name = "Shake"; break;
        case 2: gesture_name = "Swipe"; break;
        case 3: gesture_name = "Pickup"; break;
      }
      const auto phase = gesture_.calibration_phase();
      const uint32_t capture_ms = gesture_.calibration_progress_ms();

      display_->clear();
      display_->set_font_small();

      // Title
      const char* title = "CALIBRATING";
      int tw = display_->text_width(title);
      display_->draw_text((display_->width() - tw) / 2, 10, title);

      // Gesture name
      tw = display_->text_width(gesture_name);
      display_->draw_text((display_->width() - tw) / 2, 18, gesture_name);

      // Phase indicator
      const char* phase_str = "";
      switch (phase) {
        case CalibrationPhase::kWait: phase_str = "Get ready..."; break;
        case CalibrationPhase::kCapturing:
          { char buf[32];
            std::snprintf(buf, sizeof(buf), "Capturing %lums", static_cast<unsigned long>(capture_ms));
            phase_str = buf;
            break; }
        case CalibrationPhase::kComplete: phase_str = "Complete!"; break;
        default: break;
      }
      tw = display_->text_width(phase_str);
      display_->draw_text((display_->width() - tw) / 2, 32, phase_str);

      // Progress bar
      int bar_x = 14, bar_y = 40, bar_w = 100, bar_h = 8;
      display_->draw_frame(bar_x, bar_y, bar_w, bar_h);
      int fill = 0;
      switch (phase) {
        case CalibrationPhase::kWait:
          fill = 10; break;
        case CalibrationPhase::kCapturing:
          fill = 10 + (90 * (int)capture_ms) / 3000;
          if (fill > 100) fill = 100;
          break;
        case CalibrationPhase::kComplete:
          fill = 100; break;
        default: break;
      }
      if (fill > 0) {
        display_->fill_rbox(bar_x + 2, bar_y + 2, ((bar_w - 4) * fill) / 100, bar_h - 4, 2);
      }

      // Peak value during capture
      if (phase == CalibrationPhase::kCapturing) {
        char peak_buf[32];
        std::snprintf(peak_buf, sizeof(peak_buf), "Peak:%.3f", 
                      (double)gesture_.calibration_peak());
        tw = display_->text_width(peak_buf);
        display_->draw_text((display_->width() - tw) / 2, 52, peak_buf);
      }

      display_->send_buffer();
    }
  } else {
    if (hw_.gyro == HwState::kPresent && !gesture_.mpu_available()) {
      hw_.gyro = HwState::kAbsent;
    }
    const std::string gesture_cmd = gesture_.poll(now_ms, power_.is_pressed());
    if (!gesture_cmd.empty() && !clock_.enabled() && !menu_.is_open()) {
      commands_->handle(gesture_cmd, now_ms);
    }
  }

  // --- Tilt Compensation (Passive) ---
  // Gaze is only written while actually tilted; writing (0,0) every tick
  // when flat would fight idle wandering and expression gaze positions.
  if (!clock_.enabled() && !menu_.is_open() && eyes_) {
    if (gesture_.matching_enabled() && !gesture_.suspended()) {
      float p = gesture_.pitch();
      float r = gesture_.roll();
      if (std::abs(p) > 10.0f || std::abs(r) > 10.0f) {
          // Simple mapping: 45 deg tilt = 0.5 gaze offset
          float gx = std::clamp(-r / 45.0f, -0.6f, 0.6f);
          float gy = std::clamp(-p / 45.0f, -0.6f, 0.6f);
          eyes_->set_gaze_manual(gx, gy);
          was_tilted_ = true;
      } else if (was_tilted_) {
          eyes_->set_gaze_manual(0.0f, 0.0f);
          was_tilted_ = false;
      }
    } else if (was_tilted_) {
      eyes_->set_gaze_manual(0.0f, 0.0f);
      was_tilted_ = false;
    }
  }
  // -----------------------------------

  const char *shuffle_cmd = nullptr;
  if (!clock_.enabled() && shuffle_.should_emit(now_ms, gesture_.reacting(), false, &shuffle_cmd) &&
      shuffle_cmd != nullptr) {
    commands_->handle(shuffle_cmd, now_ms, false);
  }

  const bool is_clock_enabled = clock_.enabled();
  const bool is_suspended = is_clock_enabled || menu_.is_open();
  gesture_.set_suspended(is_suspended);

  if (is_clock_enabled != was_clock_enabled_ && !gesture_.calibrating()) {
    display_->clear();
    display_->send_buffer();
    was_clock_enabled_ = is_clock_enabled;
  }

  if (menu_.is_open() != was_menu_open_ && !gesture_.calibrating()) {
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
  } else if (!gesture_.calibrating()) {
    if (is_clock_enabled) {
      clock_.draw(*display_, ble_.connected());
    } else {
      eyes_->update(now_ms);
    }
  }
}

} // namespace leor
