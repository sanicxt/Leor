// MPU6050_AHRS.h — MPU-6050 with Mahony AHRS fusion
// Based on jremington/MPU-6050-Fusion

#ifndef MPU6050_AHRS_H
#define MPU6050_AHRS_H

#include <Arduino.h>
#include <Wire.h>

struct MPU6050Data {
  // Raw sensor values
  int16_t rawAx, rawAy, rawAz;
  int16_t rawGx, rawGy, rawGz;
  int16_t rawTemp;

  // Temperature in °C
  float tempC;

  // Calibrated accel in g
  float axG, ayG, azG;

  // Calibrated gyro in deg/s
  float gxDps, gyDps, gzDps;

  // Euler angles in degrees
  float yaw, pitch, roll;
};

class MPU6050_AHRS {
public:
  // Constructor: I2C address (0x68 or 0x69)
  MPU6050_AHRS(uint8_t addr = 0x68);

  // Initialize with custom I2C pins (ESP32). Returns true if sensor found.
  bool begin(int sdaPin = -1, int sclPin = -1, uint32_t i2cClock = 400000);

  // Run one update cycle: read sensors, calibrate or compute AHRS.
  // Returns true when data is valid (calibration complete).
  bool update();

  // Get latest data
  const MPU6050Data& getData() const { return _data; }

  // Is gyro calibration done?
  bool isCalibrated() const { return !_calibrating; }

  // Get calibration progress (0–500)
  uint16_t calibrationProgress() const { return _calCount; }

  // Set Mahony filter gains (default Kp=30, Ki=0)
  void setFilterGains(float kp, float ki) { _Kp = kp; _Ki = ki; }

  // Set accelerometer calibration offsets and scale
  void setAccelCal(float offX, float offY, float offZ,
                   float scX, float scY, float scZ);

  // Manually set gyro offsets (disables auto-calibration)
  void setGyroOffsets(float offX, float offY, float offZ);

  // Power down the sensor (full sleep, ~5µA)
  void sleep();

  // Wake the sensor back up and re-apply config
  void wake();

  // Accelerometer-only low-power cycle mode (gyro off, temp off)
  // wakeFreq: 0=1.25Hz, 1=5Hz, 2=20Hz, 3=40Hz
  void lowPowerAccelOnly(uint8_t wakeFreq = 2);

private:
  uint8_t _addr;

  // Calibration
  bool    _calibrating;
  uint16_t _calCount;
  long    _gsum[3];
  float   _G_off[3];
  float   _A_cal[6];

  // Mahony filter state
  float _q[4];       // quaternion
  float _Kp, _Ki;
  float _ix, _iy, _iz; // integral feedback

  // Timing
  unsigned long _lastMicros;

  // Output
  MPU6050Data _data;

  // I2C helpers
  void    writeReg(uint8_t reg, uint8_t val);
  uint8_t readReg(uint8_t reg);
  bool    readSensors();

  // Mahony AHRS
  void mahonyUpdate(float ax, float ay, float az,
                    float gx, float gy, float gz,
                    float dt);

  // Compute Euler angles from quaternion
  void computeEuler();

  static constexpr float GSCALE = (250.0 / 32768.0) * (PI / 180.0);
  static constexpr float G_TO_DPS = 250.0 / 32768.0;
  static constexpr float A_TO_G = 1.0 / 16384.0; // ±2g full-scale
};

#endif
