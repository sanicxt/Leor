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

class GestureService {
  public:
    void start(bool dummy_enabled, int i2c_sda_pin = 10, int i2c_scl_pin = 7, DisplayBackend* display = nullptr);
    void restore(bool matching, uint32_t rt, uint32_t cf, uint32_t cd, const std::string& actions_csv);
    std::string poll(uint32_t now_ms, bool touch_active);
    void set_matching_enabled(bool enabled) { matching_enabled_ = enabled; }
    bool matching_enabled() const { return matching_enabled_; }
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
    // ------------------------

    void set_action(int index, const std::string& action);
    std::string action(int index) const;
    std::string list_json() const;
    std::string settings_json() const;

    float pitch() const { return mpu_.data().pitch; }
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
    uint32_t sampling_start_ms_ = 0;
    
    std::string classify();

    bool dummy_enabled_ = true;
    bool matching_enabled_ = true;
    uint32_t last_emit_ms_ = 0;
    uint32_t reaction_time_ms_ = 1500;
    uint32_t confidence_percent_ = 70;
    uint32_t cooldown_ms_ = 1500;

    float shake_threshold_ = 200.0f;
    float pat_threshold_ = 0.32f;
    float swipe_threshold_ = 0.45f;
    float touch_ratio_threshold_ = 0.05f;

    static constexpr int kLabelCount = 5;
    const char* labels_[kLabelCount] = {"pat", "shake", "swipe", "pickup", "pet"};
    std::string actions_[kLabelCount] = {"happy", "angry", "curious", "neutral", "love"};

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
