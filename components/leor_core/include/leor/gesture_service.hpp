#pragma once

#include <cstdint>
#include <string>

#include "leor/display_backend.hpp"
#include "leor/mpu6050_ahrs_ng.hpp"

namespace leor {

enum class GestureEvent : uint8_t {
    kNone,
    kPat,
    kShake,
    kSwipe,
    kPickup,
};

enum class CalibrationPhase : uint8_t {
    kIdle,
    kWait,
    kCapturing,
    kComplete,
};

class GestureService {
  public:
    void start(bool dummy_enabled, int i2c_sda_pin = 10, int i2c_scl_pin = 7, DisplayBackend* display = nullptr);
    void restore(bool matching, uint32_t rt, uint32_t cf, uint32_t cd, const std::string& actions_csv);
    std::string poll(uint32_t now_ms, bool touch_active);
    void set_matching_enabled(bool enabled);
    bool matching_enabled() const { return matching_enabled_; }
    void set_suspended(bool suspended);
    bool suspended() const { return suspended_; }
    void set_reaction_time(uint32_t value) { reaction_time_ms_ = value; }
    uint32_t reaction_time_ms() const { return reaction_time_ms_; }
    void set_confidence(uint32_t value) { confidence_percent_ = value; }
    uint32_t confidence_percent() const { return confidence_percent_; }
    void set_cooldown(uint32_t value) { cooldown_ms_ = value; }
    uint32_t cooldown_ms() const { return cooldown_ms_; }

    // --- Threshold Tuning ---
    void set_shake_threshold(float val) { shake_threshold_ = val; }
    float shake_threshold() const { return shake_threshold_; }
    void set_pat_threshold(float val) { pat_threshold_ = val; }
    float pat_threshold() const { return pat_threshold_; }
    void set_swipe_threshold(float val) { swipe_threshold_ = val; }
    float swipe_threshold() const { return swipe_threshold_; }
    void set_touch_threshold(float val) { touch_ratio_threshold_ = val; }
    float touch_threshold() const { return touch_ratio_threshold_; }
    void set_pickup_tilt_deg(float val) { pickup_tilt_deg_ = val; }
    float pickup_tilt_deg() const { return pickup_tilt_deg_; }
    void set_inverted(bool inv) { inverted_ = inv; }
    bool inverted() const { return inverted_; }
    // ------------------------

    // --- Per-Gesture Calibration ---
    bool calibrating() const { return calib_.phase != CalibrationPhase::kIdle; }
    CalibrationPhase calibration_phase() const { return calib_.phase; }
    int calibration_gesture_index() const { return calib_.gesture_index; }
    float calibration_peak() const { return calib_.peak_value; }
    float calibration_new_threshold() const { return calib_.new_threshold; }
    uint32_t calibration_progress_ms() const { return calib_.capture_ms; }

    void start_calibration(int gesture_index, uint32_t now_ms);
    void abort_calibration();
    /// Returns non-empty JSON string when calibration completes or aborts
    std::string calibration_tick(uint32_t now_ms, bool touch_active);
    std::string calibration_status_json() const;
    // ---------------------------

    void set_action(int index, const std::string& action);
    std::string action(int index) const;
    std::string list_json() const;
    std::string settings_json() const;

    float pitch() const { return inverted_ ? -mpu_.data().pitch : mpu_.data().pitch; }
    float roll() const { return mpu_.data().roll; }

  private:
    bool init_mpu(int i2c_sda_pin, int i2c_scl_pin, DisplayBackend* display);
    bool read_mpu_sample();
    void draw_calibration(DisplayBackend* display, int percent, bool done);

    float az_lp_ = 1.0f;
    float ax_lp_ = 0.0f;
    float ay_lp_ = 0.0f;
    uint32_t touch_start_ms_ = 0;
    bool was_touching_ = false;
    bool was_tilted_ = false;

    struct GestureFeatures {
        float max_gyro = 0.0f;
        float max_az_delta = 0.0f;
        float max_axy_delta = 0.0f;
        int touch_samples = 0;
        int total_samples = 0;
        bool tilt_triggered = false;
        void reset() { *this = GestureFeatures(); }
    };

    GestureFeatures window_stats_{};
    enum class State { kReady, kSampling, kActive, kCooldown };
    State state_ = State::kReady;
    uint32_t state_start_ms_ = 0;

    struct CalibrationState {
        CalibrationPhase phase = CalibrationPhase::kIdle;
        uint32_t phase_start_ms = 0;
        uint32_t calib_start_ms = 0;
        uint32_t capture_ms = 0;
        int gesture_index = -1;
        float peak_value = 0.0f;
        float new_threshold = 0.0f;
        int sample_count = 0;

        static constexpr uint32_t kWaitMs = 500;
        static constexpr uint32_t kCaptureMs = 3000;
        static constexpr uint32_t kTotalTimeoutMs = 15000;
        static constexpr float kThresholdRatio = 0.70f;

        void reset() {
            phase = CalibrationPhase::kIdle;
            phase_start_ms = 0;
            calib_start_ms = 0;
            capture_ms = 0;
            gesture_index = -1;
            peak_value = 0.0f;
            new_threshold = 0.0f;
            sample_count = 0;
        }
    };
    CalibrationState calib_{};

    std::string classify();

    bool dummy_enabled_ = true;
    bool matching_enabled_ = true;
    bool suspended_ = false;
    bool inverted_ = false;
    uint32_t last_emit_ms_ = 0;
    uint32_t reaction_time_ms_ = 1500;
    uint32_t confidence_percent_ = 70;
    uint32_t cooldown_ms_ = 1500;

    float shake_threshold_ = 200.0f;
    float pat_threshold_ = 0.32f;
    float swipe_threshold_ = 0.45f;
    float touch_ratio_threshold_ = 0.05f;
    float pickup_tilt_deg_ = 30.0f;

    static constexpr int kLabelCount = 4;
    const char* labels_[kLabelCount] = {"pat", "shake", "swipe", "pickup"};
    std::string actions_[kLabelCount] = {"happy", "angry", "curious", "neutral"};

    bool mpu_available_ = false;
    bool mpu_calibrated_ = false;
    int i2c_sda_pin_ = 10;
    int i2c_scl_pin_ = 7;
    float gyro_off_x_ = 0.0f;
    float gyro_off_y_ = 0.0f;
    float gyro_off_z_ = 0.0f;
    float gx_dps_ = 0.0f;
    float gy_dps_ = 0.0f;
    float gz_dps_ = 0.0f;
    float ax_g_ = 0.0f;
    float ay_g_ = 0.0f;
    float az_g_ = 0.0f;
    uint32_t last_mpu_read_ms_ = 0;
    Mpu6050AhrsNg mpu_{};
};

}  // namespace leor
