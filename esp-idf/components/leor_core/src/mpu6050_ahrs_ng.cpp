#include "leor/mpu6050_ahrs_ng.hpp"

#include <cmath>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace leor {

namespace {

static const char* kTag = "mpu6050_ng";

constexpr float kPi = 3.14159265358979323846f;
constexpr float kGscale = (250.0f / 32768.0f) * (kPi / 180.0f);
constexpr float kGToDps = 250.0f / 32768.0f;
constexpr float kAToG = 1.0f / 16384.0f;

inline int16_t be16(uint8_t hi, uint8_t lo) {
    return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
}

}  // namespace

Mpu6050AhrsNg::Mpu6050AhrsNg(uint8_t addr) : addr_(addr) {}

Mpu6050AhrsNg::~Mpu6050AhrsNg() {
    if (dev_ != nullptr && bus_ != nullptr) {
        i2c_master_bus_rm_device(dev_);
        dev_ = nullptr;
    }
    if (owns_bus_ && bus_ != nullptr) {
        i2c_del_master_bus(bus_);
        bus_ = nullptr;
    }
}

bool Mpu6050AhrsNg::begin(int sda_pin, int scl_pin, uint32_t i2c_clock_hz, i2c_port_num_t i2c_port) {
    port_ = i2c_port;

    if (i2c_master_get_bus_handle(port_, &bus_) != ESP_OK) {
        i2c_master_bus_config_t bus_cfg = {};
        bus_cfg.i2c_port = port_;
        bus_cfg.sda_io_num = static_cast<gpio_num_t>(sda_pin);
        bus_cfg.scl_io_num = static_cast<gpio_num_t>(scl_pin);
        bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.flags.enable_internal_pullup = true;
        if (i2c_new_master_bus(&bus_cfg, &bus_) != ESP_OK) {
            ESP_LOGW(kTag, "i2c_new_master_bus failed");
            return false;
        }
        owns_bus_ = true;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = addr_;
    dev_cfg.scl_speed_hz = i2c_clock_hz;
    if (i2c_master_bus_add_device(bus_, &dev_cfg, &dev_) != ESP_OK) {
        ESP_LOGW(kTag, "i2c_master_bus_add_device failed");
        return false;
    }

    uint8_t whoami = 0xFF;
    if (read_reg(0x75, &whoami) != ESP_OK) {
        ESP_LOGW(kTag, "WHO_AM_I read failed");
        return false;
    }

    bool found = (whoami == 0x68 || whoami == 0x72 || whoami == 0x70 || whoami == 0x71 || whoami == 0x73 || whoami == 0x98);
    if (!found) {
        ESP_LOGW(kTag, "unexpected WHO_AM_I=0x%02x", whoami);
    }

    write_reg(0x6B, 0x80);
    vTaskDelay(pdMS_TO_TICKS(100));
    write_reg(0x6B, 0x01);
    vTaskDelay(pdMS_TO_TICKS(100));
    write_reg(0x1B, 0x00);
    write_reg(0x1C, 0x00);
    write_reg(0x1A, 0x03);
    write_reg(0x19, 0x00);
    vTaskDelay(pdMS_TO_TICKS(50));

    last_us_ = esp_timer_get_time();
    calibrating_ = true;
    cal_count_ = 0;
    gsum_[0] = gsum_[1] = gsum_[2] = 0;
    ix_ = iy_ = iz_ = 0.0f;
    q_[0] = 1.0f;
    q_[1] = q_[2] = q_[3] = 0.0f;
    return found;
}

esp_err_t Mpu6050AhrsNg::write_reg(uint8_t reg, uint8_t value) {
    uint8_t write_buf[2] = {reg, value};
    return i2c_master_transmit(dev_, write_buf, sizeof(write_buf), 100);
}

esp_err_t Mpu6050AhrsNg::read_reg(uint8_t reg, uint8_t* out) {
    return i2c_master_transmit_receive(dev_, &reg, 1, out, 1, 100);
}

bool Mpu6050AhrsNg::read_sensors() {
    uint8_t reg = 0x3B;
    uint8_t raw[14] = {};
    if (i2c_master_transmit_receive(dev_, &reg, 1, raw, sizeof(raw), 100) != ESP_OK) {
        return false;
    }

    data_.rawAx = be16(raw[0], raw[1]);
    data_.rawAy = be16(raw[2], raw[3]);
    data_.rawAz = be16(raw[4], raw[5]);
    data_.rawTemp = be16(raw[6], raw[7]);
    data_.rawGx = be16(raw[8], raw[9]);
    data_.rawGy = be16(raw[10], raw[11]);
    data_.rawGz = be16(raw[12], raw[13]);
    return true;
}

bool Mpu6050AhrsNg::update() {
    if (!read_sensors()) {
        return false;
    }

    if (calibrating_) {
        gsum_[0] += data_.rawGx;
        gsum_[1] += data_.rawGy;
        gsum_[2] += data_.rawGz;
        cal_count_++;
        if (cal_count_ >= 500) {
            calibrating_ = false;
            g_off_[0] = static_cast<float>(gsum_[0]) / 500.0f;
            g_off_[1] = static_cast<float>(gsum_[1]) / 500.0f;
            g_off_[2] = static_cast<float>(gsum_[2]) / 500.0f;
            last_us_ = esp_timer_get_time();
        }
        return false;
    }

    data_.tempC = static_cast<float>(data_.rawTemp) / 340.0f + 36.53f;

    const float ax = (static_cast<float>(data_.rawAx) - a_cal_[0]) * a_cal_[3];
    const float ay = (static_cast<float>(data_.rawAy) - a_cal_[1]) * a_cal_[4];
    const float az = (static_cast<float>(data_.rawAz) - a_cal_[2]) * a_cal_[5];

    data_.axG = ax * kAToG;
    data_.ayG = ay * kAToG;
    data_.azG = az * kAToG;

    float gx = (static_cast<float>(data_.rawGx) - g_off_[0]) * kGscale;
    float gy = (static_cast<float>(data_.rawGy) - g_off_[1]) * kGscale;
    float gz = (static_cast<float>(data_.rawGz) - g_off_[2]) * kGscale;

    data_.gxDps = (static_cast<float>(data_.rawGx) - g_off_[0]) * kGToDps;
    data_.gyDps = (static_cast<float>(data_.rawGy) - g_off_[1]) * kGToDps;
    data_.gzDps = (static_cast<float>(data_.rawGz) - g_off_[2]) * kGToDps;

    const int64_t now_us = esp_timer_get_time();
    const float dt = static_cast<float>(now_us - last_us_) * 1.0e-6f;
    last_us_ = now_us;

    mahony_update(ax, ay, az, gx, gy, gz, dt);
    compute_euler();
    return true;
}

void Mpu6050AhrsNg::set_accel_cal(float off_x, float off_y, float off_z, float sc_x, float sc_y, float sc_z) {
    a_cal_[0] = off_x;
    a_cal_[1] = off_y;
    a_cal_[2] = off_z;
    a_cal_[3] = sc_x;
    a_cal_[4] = sc_y;
    a_cal_[5] = sc_z;
}

void Mpu6050AhrsNg::set_gyro_offsets(float off_x, float off_y, float off_z) {
    g_off_[0] = off_x;
    g_off_[1] = off_y;
    g_off_[2] = off_z;
    calibrating_ = false;
    cal_count_ = 500;
}

void Mpu6050AhrsNg::sleep() {
    write_reg(0x6B, 0x41);
}

void Mpu6050AhrsNg::wake() {
    write_reg(0x6B, 0x01);
    vTaskDelay(pdMS_TO_TICKS(100));
    write_reg(0x6C, 0x00);
    write_reg(0x1B, 0x00);
    write_reg(0x1C, 0x00);
    write_reg(0x1A, 0x03);
    write_reg(0x19, 0x00);
    last_us_ = esp_timer_get_time();
}

void Mpu6050AhrsNg::low_power_accel_only(uint8_t wake_freq) {
    write_reg(0x6B, 0x28);
    const uint8_t pwr2 = static_cast<uint8_t>(((wake_freq & 0x03U) << 6) | 0x07U);
    write_reg(0x6C, pwr2);
}

void Mpu6050AhrsNg::compute_euler() {
    data_.roll = std::atan2((q_[0] * q_[1] + q_[2] * q_[3]), 0.5f - (q_[1] * q_[1] + q_[2] * q_[2])) * 180.0f / kPi;
    data_.pitch = std::asin(2.0f * (q_[0] * q_[2] - q_[1] * q_[3])) * 180.0f / kPi;
    data_.yaw = -std::atan2((q_[1] * q_[2] + q_[0] * q_[3]), 0.5f - (q_[2] * q_[2] + q_[3] * q_[3])) * 180.0f / kPi;
}

void Mpu6050AhrsNg::mahony_update(float ax, float ay, float az, float gx, float gy, float gz, float dt) {
    const float tmp = ax * ax + ay * ay + az * az;
    if (tmp > 0.0f) {
        const float recip_norm = 1.0f / std::sqrt(tmp);
        ax *= recip_norm;
        ay *= recip_norm;
        az *= recip_norm;

        const float vx = q_[1] * q_[3] - q_[0] * q_[2];
        const float vy = q_[0] * q_[1] + q_[2] * q_[3];
        const float vz = q_[0] * q_[0] - 0.5f + q_[3] * q_[3];

        const float ex = ay * vz - az * vy;
        const float ey = az * vx - ax * vz;
        const float ez = ax * vy - ay * vx;

        if (ki_ > 0.0f) {
            ix_ += ki_ * ex * dt;
            iy_ += ki_ * ey * dt;
            iz_ += ki_ * ez * dt;
            gx += ix_;
            gy += iy_;
            gz += iz_;
        }

        gx += kp_ * ex;
        gy += kp_ * ey;
        gz += kp_ * ez;
    }

    dt *= 0.5f;
    gx *= dt;
    gy *= dt;
    gz *= dt;
    const float qa = q_[0], qb = q_[1], qc = q_[2];

    q_[0] += (-qb * gx - qc * gy - q_[3] * gz);
    q_[1] += (qa * gx + qc * gz - q_[3] * gy);
    q_[2] += (qa * gy - qb * gz + q_[3] * gx);
    q_[3] += (qa * gz + qb * gy - qc * gx);

    const float recip_norm = 1.0f / std::sqrt(q_[0] * q_[0] + q_[1] * q_[1] + q_[2] * q_[2] + q_[3] * q_[3]);
    q_[0] *= recip_norm;
    q_[1] *= recip_norm;
    q_[2] *= recip_norm;
    q_[3] *= recip_norm;
}

}  // namespace leor
