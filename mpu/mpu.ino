// MPU-6050 Gyro Visualizer — Main sketch
// Uses MPU6050_AHRS class for sensor fusion

#include "MPU6050_AHRS.h"

// I2C pins (set to -1 to use board defaults)
#define I2C_SDA_PIN 10
#define I2C_SCL_PIN 7

// Create sensor instance (0x68 default, use 0x69 if AD0 is HIGH)
MPU6050_AHRS mpu(0x68);

// Print interval
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 100; // ms

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=== MPU-6050 Gyro + Calibration ===");

  if (!mpu.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
    Serial.println("WARNING: MPU-6050 may not be connected");
  }

  // Optional: set known accel calibration
  // mpu.setAccelCal(offsetX, offsetY, offsetZ, scaleX, scaleY, scaleZ);

  // Optional: skip auto-calibration with known gyro offsets
  // mpu.setGyroOffsets(-499.5, -17.7, -82.0);
}

void loop() {
  // Update sensor — returns false during calibration
  if (!mpu.update()) return;

  // Throttle serial output
  unsigned long now = millis();
  if (now - lastPrint < PRINT_INTERVAL) return;
  lastPrint = now;

  const MPU6050Data& d = mpu.getData();

  // CSV: RAW_AX, RAW_AY, RAW_AZ, AX, AY, AZ, RAW_GX, RAW_GY, RAW_GZ, CAL_GX, CAL_GY, CAL_GZ, YAW, PITCH, ROLL, TEMP
  Serial.print(d.rawAx); Serial.print(", ");
  Serial.print(d.rawAy); Serial.print(", ");
  Serial.print(d.rawAz); Serial.print(", ");

  Serial.print(d.axG, 3); Serial.print(", ");
  Serial.print(d.ayG, 3); Serial.print(", ");
  Serial.print(d.azG, 3); Serial.print(", ");

  Serial.print(d.rawGx); Serial.print(", ");
  Serial.print(d.rawGy); Serial.print(", ");
  Serial.print(d.rawGz); Serial.print(", ");

  Serial.print(d.gxDps, 2); Serial.print(", ");
  Serial.print(d.gyDps, 2); Serial.print(", ");
  Serial.print(d.gzDps, 2); Serial.print(", ");

  Serial.print(d.yaw, 1);   Serial.print(", ");
  Serial.print(d.pitch, 1); Serial.print(", ");
  Serial.print(d.roll, 1);  Serial.print(", ");
  Serial.println(d.tempC, 1);
}
