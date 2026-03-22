// MPU6050_AHRS.cpp — Implementation
// Based on jremington/MPU-6050-Fusion

#include "MPU6050_AHRS.h"

MPU6050_AHRS::MPU6050_AHRS(uint8_t addr)
  : _addr(addr),
    _calibrating(true),
    _calCount(0),
    _Kp(30.0f), _Ki(0.0f),
    _ix(0), _iy(0), _iz(0),
    _lastMicros(0),
    _data{0}
{
  _q[0] = 1.0f; _q[1] = 0; _q[2] = 0; _q[3] = 0;
  _gsum[0] = 0; _gsum[1] = 0; _gsum[2] = 0;
  _G_off[0] = 0; _G_off[1] = 0; _G_off[2] = 0;
  _A_cal[0] = 0; _A_cal[1] = 0; _A_cal[2] = 0;
  _A_cal[3] = 1; _A_cal[4] = 1; _A_cal[5] = 1;
}

// ---- I2C helpers ----

void MPU6050_AHRS::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
  delay(10);
}

uint8_t MPU6050_AHRS::readReg(uint8_t reg) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(_addr, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

// ---- Public API ----

bool MPU6050_AHRS::begin(int sdaPin, int sclPin, uint32_t i2cClock) {
  // Start I2C
  if (sdaPin >= 0 && sclPin >= 0) {
    Wire.begin(sdaPin, sclPin);
  } else {
    Wire.begin();
  }
  Wire.setClock(i2cClock);
  delay(100);

  // Check WHO_AM_I
  uint8_t whoami = readReg(0x75);
  Serial.print("[MPU6050] WHO_AM_I: 0x");
  Serial.println(whoami, HEX);

  bool found = (whoami == 0x68 || whoami == 0x72 || whoami == 0x70 ||
                whoami == 0x71 || whoami == 0x73 || whoami == 0x98);

  if (!found) {
    Serial.println("[MPU6050] WARNING: Unexpected WHO_AM_I, check wiring/address");
  }

  // Reset
  writeReg(0x6B, 0x80);
  delay(100);

  // Wake up with PLL X gyro as clock source (recommended by datasheet)
  writeReg(0x6B, 0x01);  // PWR_MGMT_1: SLEEP=0, CLKSEL=1 (PLL X gyro)
  delay(100);

  // Gyro ±250 °/s
  writeReg(0x1B, 0x00);
  // Accel ±2g
  writeReg(0x1C, 0x00);
  // DLPF ~44Hz
  writeReg(0x1A, 0x03);
  // Sample rate divider
  writeReg(0x19, 0x00);

  delay(50);

  // Verify
  uint8_t pwr = readReg(0x6B);
  Serial.print("[MPU6050] PWR_MGMT_1: 0x");
  Serial.println(pwr, HEX);

  _lastMicros = micros();
  _calibrating = true;
  _calCount = 0;

  Serial.println("[MPU6050] Calibrating gyro... keep still");
  return found;
}

bool MPU6050_AHRS::readSensors() {
  Wire.beginTransmission(_addr);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom(_addr, (uint8_t)14) < 14) {
    while (Wire.available()) Wire.read();
    return false;
  }

  _data.rawAx = (int16_t)(Wire.read() << 8 | Wire.read());
  _data.rawAy = (int16_t)(Wire.read() << 8 | Wire.read());
  _data.rawAz = (int16_t)(Wire.read() << 8 | Wire.read());
  _data.rawTemp = (int16_t)(Wire.read() << 8 | Wire.read());
  _data.rawGx = (int16_t)(Wire.read() << 8 | Wire.read());
  _data.rawGy = (int16_t)(Wire.read() << 8 | Wire.read());
  _data.rawGz = (int16_t)(Wire.read() << 8 | Wire.read());

  return true;
}

bool MPU6050_AHRS::update() {
  if (!readSensors()) return false;

  // --- Calibration phase ---
  if (_calibrating) {
    _gsum[0] += _data.rawGx;
    _gsum[1] += _data.rawGy;
    _gsum[2] += _data.rawGz;
    _calCount++;

    if (_calCount >= 500) {
      _calibrating = false;
      _G_off[0] = (float)_gsum[0] / 500.0f;
      _G_off[1] = (float)_gsum[1] / 500.0f;
      _G_off[2] = (float)_gsum[2] / 500.0f;

      Serial.println("[MPU6050] Gyro calibration complete!");
      Serial.print("[MPU6050] G_off: ");
      Serial.print(_G_off[0], 1); Serial.print(", ");
      Serial.print(_G_off[1], 1); Serial.print(", ");
      Serial.println(_G_off[2], 1);

      _lastMicros = micros();
    }
    return false; // not ready yet
  }

  // --- Temperature ---
  _data.tempC = (float)_data.rawTemp / 340.0f + 36.53f;

  // --- Apply calibration ---
  float ax = ((float)_data.rawAx - _A_cal[0]) * _A_cal[3];
  float ay = ((float)_data.rawAy - _A_cal[1]) * _A_cal[4];
  float az = ((float)_data.rawAz - _A_cal[2]) * _A_cal[5];

  // Calibrated accel in g
  _data.axG = ax * A_TO_G;
  _data.ayG = ay * A_TO_G;
  _data.azG = az * A_TO_G;

  float gx = ((float)_data.rawGx - _G_off[0]) * GSCALE;
  float gy = ((float)_data.rawGy - _G_off[1]) * GSCALE;
  float gz = ((float)_data.rawGz - _G_off[2]) * GSCALE;

  // Calibrated gyro in deg/s
  _data.gxDps = ((float)_data.rawGx - _G_off[0]) * G_TO_DPS;
  _data.gyDps = ((float)_data.rawGy - _G_off[1]) * G_TO_DPS;
  _data.gzDps = ((float)_data.rawGz - _G_off[2]) * G_TO_DPS;

  // --- Delta time ---
  unsigned long now = micros();
  float dt = (now - _lastMicros) * 1.0e-6f;
  _lastMicros = now;

  // --- Mahony AHRS ---
  mahonyUpdate(ax, ay, az, gx, gy, gz, dt);

  // --- Euler angles ---
  computeEuler();

  return true;
}

void MPU6050_AHRS::setAccelCal(float offX, float offY, float offZ,
                                float scX, float scY, float scZ) {
  _A_cal[0] = offX; _A_cal[1] = offY; _A_cal[2] = offZ;
  _A_cal[3] = scX;  _A_cal[4] = scY;  _A_cal[5] = scZ;
}

void MPU6050_AHRS::setGyroOffsets(float offX, float offY, float offZ) {
  _G_off[0] = offX; _G_off[1] = offY; _G_off[2] = offZ;
  _calibrating = false;
  _calCount = 500;
}

void MPU6050_AHRS::sleep() {
  // PWR_MGMT_1: SLEEP=1, keep CLKSEL=1
  writeReg(0x6B, 0x41); // bit6=SLEEP, bit0=CLKSEL PLL X
  Serial.println("[MPU6050] Sensor sleeping");
}

void MPU6050_AHRS::wake() {
  // PWR_MGMT_1: SLEEP=0, CLKSEL=1 (PLL X gyro)
  writeReg(0x6B, 0x01);
  delay(100);

  // Clear any standby bits
  writeReg(0x6C, 0x00); // PWR_MGMT_2: all axes enabled

  // Re-apply config
  writeReg(0x1B, 0x00); // Gyro ±250 °/s
  writeReg(0x1C, 0x00); // Accel ±2g
  writeReg(0x1A, 0x03); // DLPF ~44Hz
  writeReg(0x19, 0x00); // Sample rate divider

  _lastMicros = micros();
  Serial.println("[MPU6050] Sensor awake");
}

void MPU6050_AHRS::lowPowerAccelOnly(uint8_t wakeFreq) {
  // Per datasheet Section 4.31:
  // 1. Set CYCLE=1, SLEEP=0, TEMP_DIS=1, CLKSEL=0 (internal 8MHz)
  writeReg(0x6B, 0x28); // bit5=CYCLE, bit3=TEMP_DIS, CLKSEL=0

  // 2. Put all gyro axes into standby (PWR_MGMT_2)
  //    LP_WAKE_CTRL[7:6] = wakeFreq, STBY_XG=1, STBY_YG=1, STBY_ZG=1
  uint8_t pwr2 = ((wakeFreq & 0x03) << 6) | 0x07; // bits 2:0 = gyro standby
  writeReg(0x6C, pwr2);

  Serial.print("[MPU6050] Low-power accel-only mode, wake freq: ");
  const char* freqs[] = {"1.25Hz", "5Hz", "20Hz", "40Hz"};
  Serial.println(freqs[wakeFreq & 0x03]);
}

// ---- Euler angles ----

void MPU6050_AHRS::computeEuler() {
  _data.roll  = atan2((_q[0]*_q[1] + _q[2]*_q[3]),
                       0.5f - (_q[1]*_q[1] + _q[2]*_q[2])) * 180.0f / PI;
  _data.pitch = asin(2.0f * (_q[0]*_q[2] - _q[1]*_q[3])) * 180.0f / PI;
  _data.yaw   = -atan2((_q[1]*_q[2] + _q[0]*_q[3]),
                        0.5f - (_q[2]*_q[2] + _q[3]*_q[3])) * 180.0f / PI;
}

// ---- Mahony AHRS filter ----

void MPU6050_AHRS::mahonyUpdate(float ax, float ay, float az,
                                 float gx, float gy, float gz,
                                 float dt) {
  float tmp = ax*ax + ay*ay + az*az;

  if (tmp > 0.0f) {
    float recipNorm = 1.0f / sqrtf(tmp);
    ax *= recipNorm; ay *= recipNorm; az *= recipNorm;

    // Estimated gravity direction
    float vx = _q[1]*_q[3] - _q[0]*_q[2];
    float vy = _q[0]*_q[1] + _q[2]*_q[3];
    float vz = _q[0]*_q[0] - 0.5f + _q[3]*_q[3];

    // Error (cross product)
    float ex = ay*vz - az*vy;
    float ey = az*vx - ax*vz;
    float ez = ax*vy - ay*vx;

    // Integral feedback
    if (_Ki > 0.0f) {
      _ix += _Ki * ex * dt;
      _iy += _Ki * ey * dt;
      _iz += _Ki * ez * dt;
      gx += _ix; gy += _iy; gz += _iz;
    }

    // Proportional feedback
    gx += _Kp * ex;
    gy += _Kp * ey;
    gz += _Kp * ez;
  }

  // Integrate quaternion
  dt *= 0.5f;
  gx *= dt; gy *= dt; gz *= dt;
  float qa = _q[0], qb = _q[1], qc = _q[2];

  _q[0] += (-qb*gx - qc*gy - _q[3]*gz);
  _q[1] += ( qa*gx + qc*gz - _q[3]*gy);
  _q[2] += ( qa*gy - qb*gz + _q[3]*gx);
  _q[3] += ( qa*gz + qb*gy - qc   *gx);

  // Normalize
  float recipNorm = 1.0f / sqrtf(_q[0]*_q[0] + _q[1]*_q[1] +
                                  _q[2]*_q[2] + _q[3]*_q[3]);
  _q[0] *= recipNorm; _q[1] *= recipNorm;
  _q[2] *= recipNorm; _q[3] *= recipNorm;
}
