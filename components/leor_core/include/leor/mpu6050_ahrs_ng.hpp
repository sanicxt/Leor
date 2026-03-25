#pragma once

#include <cstdint>

#include "driver/i2c_master.h"

namespace leor {

struct Mpu6050Data {
    int16_t rawAx = 0;
    int16_t rawAy = 0;
    int16_t rawAz = 0;
    int16_t rawGx = 0;
    int16_t rawGy = 0;
    int16_t rawGz = 0;
    int16_t rawTemp = 0;

    float tempC = 0.0f;
    float axG = 0.0f;
    float ayG = 0.0f;
    float azG = 0.0f;
    float gxDps = 0.0f;
    float gyDps = 0.0f;
    float gzDps = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
};

class Mpu6050AhrsNg {
  public:
    explicit Mpu6050AhrsNg(uint8_t addr = 0x68);
    ~Mpu6050AhrsNg();

    bool begin(int sda_pin, int scl_pin, uint32_t i2c_clock_hz = 400000, i2c_port_num_t i2c_port = I2C_NUM_0);
    bool update();

    const Mpu6050Data& data() const { return data_; }
    bool is_calibrated() const { return !calibrating_; }
    uint16_t calibration_progress() const { return cal_count_; }

    void set_filter_gains(float kp, float ki) { kp_ = kp; ki_ = ki; }
    void set_accel_cal(float off_x, float off_y, float off_z, float sc_x, float sc_y, float sc_z);
    void set_gyro_offsets(float off_x, float off_y, float off_z);

    void sleep();
    void wake();
    void low_power_accel_only(uint8_t wake_freq = 2);

  private:
    esp_err_t write_reg(uint8_t reg, uint8_t value);
    esp_err_t read_reg(uint8_t reg, uint8_t* out);
    bool read_sensors();
    void mahony_update(float ax, float ay, float az, float gx, float gy, float gz, float dt);
    void compute_euler();

    uint8_t addr_ = 0x68;
    bool owns_bus_ = false;
    i2c_master_bus_handle_t bus_ = nullptr;
    i2c_master_dev_handle_t dev_ = nullptr;
    i2c_port_num_t port_ = I2C_NUM_0;

    bool calibrating_ = true;
    uint16_t cal_count_ = 0;
    int64_t last_us_ = 0;
    int32_t gsum_[3] = {0, 0, 0};
    float g_off_[3] = {0.0f, 0.0f, 0.0f};
    float a_cal_[6] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    float q_[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    float kp_ = 30.0f;
    float ki_ = 0.0f;
    float ix_ = 0.0f;
    float iy_ = 0.0f;
    float iz_ = 0.0f;

    Mpu6050Data data_{};
};

}  // namespace leor
