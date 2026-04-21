#include "leor/gesture_service.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

}  // namespace

namespace leor {

void GestureService::start(bool dummy_enabled, int i2c_sda_pin, int i2c_scl_pin, DisplayBackend* display) {
    dummy_enabled_ = dummy_enabled;
    last_emit_ms_ = 0;
    i2c_sda_pin_ = i2c_sda_pin;
    i2c_scl_pin_ = i2c_scl_pin;

    if (!dummy_enabled_) {
        mpu_available_ = init_mpu(i2c_sda_pin_, i2c_scl_pin_, display);
        mpu_calibrated_ = mpu_available_;
    }
}

void GestureService::restore(bool matching, uint32_t rt, uint32_t cf, uint32_t cd, const std::string& actions_csv) {
    reaction_time_ms_ = rt;
    confidence_percent_ = cf;
    cooldown_ms_ = cd;

    if (!actions_csv.empty()) {
        std::size_t start = 0;
        std::size_t end = actions_csv.find(',');
        int i = 0;
        while (i < kLabelCount) {
            if (end != std::string::npos) {
                actions_[i] = actions_csv.substr(start, end - start);
                start = end + 1;
                end = actions_csv.find(',', start);
            } else {
                actions_[i] = actions_csv.substr(start);
                break;
            }
            i++;
        }
    }
    
    // Set matching state last to handle MPU sleep/wake correctly
    set_matching_enabled(matching);
}

void GestureService::set_matching_enabled(bool enabled) {
    if (matching_enabled_ == enabled) return;
    
    matching_enabled_ = enabled;
    if (mpu_available_) {
        // Only wake if we are NOT suspended
        if (enabled && !suspended_) {
            ESP_LOGI("leor_gest", "Gestures enabled: Waking MPU");
            mpu_.wake();
        } else if (!enabled) {
            ESP_LOGI("leor_gest", "Gestures disabled: Sleeping MPU");
            mpu_.sleep();
            state_ = State::kReady;
        }
    }
}

void GestureService::set_suspended(bool suspended) {
    if (suspended_ == suspended) return;
    suspended_ = suspended;

    if (mpu_available_) {
        // If we are suspending, always sleep
        if (suspended) {
            ESP_LOGI("leor_gest", "System suspended: Sleeping MPU");
            mpu_.sleep();
            state_ = State::kReady;
        } 
        // If we are resuming, only wake if user enabled gestures
        else if (matching_enabled_) {
            ESP_LOGI("leor_gest", "System resumed: Waking MPU");
            mpu_.wake();
        }
    }
}

std::string GestureService::poll(uint32_t now_ms, bool touch_active) {
    if (!matching_enabled_ || suspended_ || !mpu_available_ || !mpu_calibrated_) {
        return "";
    }

    if (now_ms - last_mpu_read_ms_ < 20) {
        return "";
    }
    last_mpu_read_ms_ = now_ms;
    if (!read_mpu_sample()) {
        return "";
    }

    const auto& d = mpu_.data();
    
    // Baselines
    az_lp_ = az_lp_ * 0.95f + d.azG * 0.05f; 
    ax_lp_ = ax_lp_ * 0.95f + d.axG * 0.05f;
    ay_lp_ = ay_lp_ * 0.95f + d.ayG * 0.05f;

    const bool currently_tilted = (std::abs(d.pitch) > 30.0f || std::abs(d.roll) > 30.0f);
    const float gyro_mag = std::sqrt(d.gxDps * d.gxDps + d.gyDps * d.gyDps + d.gzDps * d.gzDps);
    const float az_delta = d.azG - az_lp_;
    const float axy_delta = std::max(std::abs(d.axG - ax_lp_), std::abs(d.ayG - ay_lp_));

    // --- State Machine ---

    // 1. PET Override (Priority)
    if (touch_active) {
        if (!was_touching_) touch_start_ms_ = now_ms;
        if (now_ms - touch_start_ms_ > 1500 && state_ != State::kActive) {
            state_ = State::kActive;
            state_start_ms_ = now_ms;
            ESP_LOGI("leor_gest", "LIFE: PET DETECTED -> Entering ACTIVE");
            return actions_[4]; // love
        }
    }

    switch (state_) {
        case State::kReady: {
            bool impulse = (gyro_mag > (shake_threshold_ * 0.75f) || std::abs(az_delta) > (pat_threshold_ * 0.8f) || axy_delta > (swipe_threshold_ * 0.8f) || (currently_tilted != was_tilted_));
            if (impulse) {
                state_ = State::kSampling;
                state_start_ms_ = now_ms;
                window_stats_.reset();
                ESP_LOGI("leor_gest", "LIFE: READY -> SAMPLING (800ms)");
            }
            break;
        }

        case State::kSampling: {
            window_stats_.total_samples++;
            if (touch_active) window_stats_.touch_samples++;
            if (gyro_mag > window_stats_.max_gyro) window_stats_.max_gyro = gyro_mag;
            if (az_delta > window_stats_.max_az_delta) window_stats_.max_az_delta = az_delta;
            if (axy_delta > window_stats_.max_axy_delta) window_stats_.max_axy_delta = axy_delta;
            if (currently_tilted && !was_tilted_) window_stats_.tilt_triggered = true;

            if (now_ms - state_start_ms_ >= 800) {
                std::string result = classify();
                if (!result.empty()) {
                    state_ = State::kActive;
                    state_start_ms_ = now_ms;
                    ESP_LOGI("leor_gest", "LIFE: SAMPLING -> ACTIVE (Duration: %ums)", (unsigned)reaction_time_ms_);
                    return result;
                } else {
                    state_ = State::kReady;
                    ESP_LOGI("leor_gest", "LIFE: SAMPLING -> READY (No match)");
                }
            }
            break;
        }

        case State::kActive: {
            // Wait for Expression Duration (rt)
            if (now_ms - state_start_ms_ >= reaction_time_ms_) {
                state_ = State::kCooldown;
                state_start_ms_ = now_ms;
                ESP_LOGI("leor_gest", "LIFE: ACTIVE -> COOLDOWN (Lockout: %ums)", (unsigned)cooldown_ms_);
                return "neutral"; // Return to neutral automatically
            }
            break;
        }

        case State::kCooldown: {
            // Wait for Cooldown (cd)
            if (now_ms - state_start_ms_ >= cooldown_ms_) {
                state_ = State::kReady;
                ESP_LOGI("leor_gest", "LIFE: COOLDOWN -> READY");
            }
            break;
        }
    }

    was_touching_ = touch_active;
    was_tilted_ = currently_tilted;
    return "";
}

std::string GestureService::classify() {
    float touch_ratio = (float)window_stats_.touch_samples / (float)window_stats_.total_samples;
    
    ESP_LOGI("leor_gest", "ALGO EVAL: G=%.1f, AZ=%.2f, AXY=%.2f, T=%.2f, Tilt=%d", 
             window_stats_.max_gyro, window_stats_.max_az_delta, 
             window_stats_.max_axy_delta, touch_ratio, window_stats_.tilt_triggered);

    // Rule 1: Shake (Violent energy)
    if (window_stats_.max_gyro > shake_threshold_) {
        return actions_[1]; // shake
    }

    // Rule 2: Pat (Vertical impulse + ANY touch contact)
    if (window_stats_.max_az_delta > pat_threshold_ && touch_ratio > touch_ratio_threshold_) {
        return actions_[0]; // pat
    }

    // Rule 3: Pickup (Priority 3, after Pat)
    if (window_stats_.tilt_triggered) {
        return actions_[3]; // pickup
    }

    // Rule 4: Nudge/Swipe (Horizontal impulse)
    if (window_stats_.max_axy_delta > swipe_threshold_) {
        return actions_[2]; // swipe
    }

    ESP_LOGI("leor_gest", "ALGO: DISCARDED (Below Confidence)");
    return "";
}

bool GestureService::init_mpu(int i2c_sda_pin, int i2c_scl_pin, DisplayBackend* display) {
    if (!mpu_.begin(i2c_sda_pin, i2c_scl_pin, 400000, I2C_NUM_0)) {
        return false;
    }

    int guard = 0;
    int last_drawn_pct = -1;
    while (!mpu_.is_calibrated() && guard < 5000) {
        const bool updated = mpu_.update();
        if (display != nullptr) {
            const int pct = (static_cast<int>(mpu_.calibration_progress()) * 100) / 500;
            if (last_drawn_pct < 0 || pct >= last_drawn_pct + 4 || pct == 100) {
                draw_calibration(display, pct, false);
                last_drawn_pct = pct;
            }
        }
        if (!updated) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        ++guard;
    }

    if (!mpu_.is_calibrated()) {
        ESP_LOGE("leor_gest", "MPU calibration timed out! check I2C / sensor orientation");
        return false;
    }

    if (display != nullptr) {
        draw_calibration(display, 100, true);
        vTaskDelay(pdMS_TO_TICKS(250));
        display->clear();
        display->send_buffer();
    }

    return true;
}

bool GestureService::read_mpu_sample() {
    if (!mpu_.update()) {
        return false;
    }
    const auto& d = mpu_.data();
    gx_dps_ = d.gxDps;
    gy_dps_ = d.gyDps;
    gz_dps_ = d.gzDps;
    ax_g_ = d.axG;
    ay_g_ = d.ayG;
    az_g_ = d.azG;
    return true;
}

void GestureService::draw_calibration(DisplayBackend* display, int percent, bool done) {
    if (display == nullptr) {
        return;
    }
    display->clear();
    display->set_font_small();
    if (done) {
        display->draw_text(18, 30, "Calibration Done!");
    } else {
        display->draw_text(18, 22, "Calibrating IMU");
        display->draw_frame(14, 35, 100, 6);
        const int fill = (96 * std::max(0, std::min(100, percent))) / 100;
        display->fill_box(16, 37, fill, 2);
    }
    display->send_buffer();
}

void GestureService::set_action(int index, const std::string& action) {
    if (index >= 0 && index < kLabelCount) {
        actions_[index] = action;
    }
}

std::string GestureService::action(int index) const {
    return (index >= 0 && index < kLabelCount) ? actions_[index] : std::string();
}

std::string GestureService::list_json() const {
    char buf[768];
    std::snprintf(buf, sizeof(buf),
                  "[{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"}]",
                  labels_[0], actions_[0].c_str(), labels_[1], actions_[1].c_str(), labels_[2], actions_[2].c_str(), labels_[3], actions_[3].c_str(), labels_[4], actions_[4].c_str());
    return buf;
}

std::string GestureService::settings_json() const {
    char buf[1024];
    std::snprintf(buf, sizeof(buf),
                  "{\"gm\":%d,\"rt\":%u,\"cf\":%u,\"cd\":%u,\"gst\":%.1f,\"gpt\":%.2f,\"gvt\":%.2f,\"gtt\":%.2f}",
                  matching_enabled_ ? 1 : 0,
                  static_cast<unsigned>(reaction_time_ms_),
                  static_cast<unsigned>(confidence_percent_),
                  static_cast<unsigned>(cooldown_ms_),
                  shake_threshold_,
                  pat_threshold_,
                  swipe_threshold_,
                  touch_ratio_threshold_);
    return buf;
}

}  // namespace leor
