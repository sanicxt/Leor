#include "leor/gesture_service.hpp"

#include <algorithm>
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

GestureEvent GestureService::poll(uint32_t now_ms) {
    if (!dummy_enabled_) {
        if (!mpu_available_ || !mpu_calibrated_) {
            return GestureEvent::kNone;
        }
        if (now_ms - last_mpu_read_ms_ < 20) {
            return GestureEvent::kNone;
        }
        last_mpu_read_ms_ = now_ms;
        read_mpu_sample();
        return GestureEvent::kNone;
    }

    if (last_emit_ms_ == 0) {
        last_emit_ms_ = now_ms;
        return GestureEvent::kNone;
    }

    return GestureEvent::kNone;
}

bool GestureService::init_mpu(int i2c_sda_pin, int i2c_scl_pin, DisplayBackend* display) {
    if (!mpu_.begin(i2c_sda_pin, i2c_scl_pin, 400000, I2C_NUM_0)) {
        return false;
    }

    int guard = 0;
    int last_drawn_pct = -1;
    while (!mpu_.is_calibrated() && guard < 1200) {
        const bool updated = mpu_.update();
        if (display != nullptr) {
            const int pct = (static_cast<int>(mpu_.calibration_progress()) * 100) / 500;
            if (last_drawn_pct < 0 || pct >= last_drawn_pct + 4 || pct == 100) {
                draw_calibration(display, pct, false);
                last_drawn_pct = pct;
            }
        }
        if (!updated) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        ++guard;
    }

    if (!mpu_.is_calibrated()) {
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
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "[{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"},{\"n\":\"%s\",\"a\":\"%s\"}]",
                  labels_[0], actions_[0].c_str(), labels_[1], actions_[1].c_str(), labels_[2], actions_[2].c_str(), labels_[3], actions_[3].c_str());
    return buf;
}

std::string GestureService::settings_json() const {
    char buf[768];
    std::snprintf(buf, sizeof(buf),
                  "{\"gm\":%d,\"rt\":%u,\"cf\":%u,\"cd\":%u,\"map\":%s}",
                  matching_enabled_ ? 1 : 0,
                  static_cast<unsigned>(reaction_time_ms_),
                  static_cast<unsigned>(confidence_percent_),
                  static_cast<unsigned>(cooldown_ms_),
                  list_json().c_str());
    return buf;
}

}  // namespace leor
