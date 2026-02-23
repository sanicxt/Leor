/*
 * imu_manager.h - IMU sensor, calibration, and gesture processing for leor
 */

#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "FastIMU.h"
#include "config.h"
#include "display_manager.h"
#include "ei_gesture.h"

// Re-define DEFAULT (Edge Impulse #undefs it)
#ifndef DEFAULT
#define DEFAULT 0
#endif

// Forward declaration (defined in ble_manager.h)
void sendBLEGesture(const String& gesture);

// ==================== IMU Objects ====================
#define IMU_ADDRESS 0x68

IMU_Generic    IMU;
calData        calib = { 0 };
AccelData      accelData;
GyroData       gyroData;
bool           mpuAvailable   = false;
bool           mpuCalibrated  = false;

// Compatibility vars used by other modules
float dpsGX, dpsGY, dpsGZ;
float gForceAX, gForceAY, gForceAZ;

#define FREQUENCY_HZ 50
#define INTERVAL_MS  (1000 / (FREQUENCY_HZ))
unsigned long last_interval_ms = 0;

// ==================== State ====================
unsigned long lastMpuRead        = 0;
unsigned long GESTURE_REACTION_MS = 1500;
unsigned long reactionStartTime  = 0;
bool          isReacting         = false;
bool          mpuVerbose         = false;

// Forward declaration (implemented in commands.h / Leor.ino)
String handleCommand(String cmd);
void resetEffects();

// ==================== Calibration ====================

void calibrateIMU() {
    Serial.println(F("  Calibrating IMU (keep still)..."));
    drawCalibrationScreen(0, "Keep device STILL");
    delay(500);

    drawCalibrationScreen(20, "Sampling gyro...");
    delay(200);

    IMU.calibrateAccelGyro(&calib);

    drawCalibrationScreen(80, "Calculating bias...");
    delay(200);

    IMU.init(calib, IMU_ADDRESS);
    mpuCalibrated = true;

    drawCalibrationScreen(100, "Calibration done!");
    delay(500);

    Serial.println(F("  Done!"));
    Serial.print(F("  Gyro bias: "));
    Serial.print(calib.gyroBias[0], 2); Serial.print(", ");
    Serial.print(calib.gyroBias[1], 2); Serial.print(", ");
    Serial.println(calib.gyroBias[2], 2);
    Serial.print(F("  Accel bias: "));
    Serial.print(calib.accelBias[0], 2); Serial.print(", ");
    Serial.print(calib.accelBias[1], 2); Serial.print(", ");
    Serial.println(calib.accelBias[2], 2);
}

// ==================== Gesture Action ====================

void executeGestureAction(const String& action) {
    isReacting = true;
    reactionStartTime = millis();
    Serial.print(F("[GESTURE] Matched! Action: "));
    Serial.println(action);

    sendBLEGesture("matched:" + action);
    handleCommand(action);
}

// ==================== IMU Init ====================

void initIMU() {
    Wire.setClock(400000);
    int err = IMU.init(calib, IMU_ADDRESS);
    if (err != 0) {
        Serial.print(F("IMU init error: "));
        Serial.println(err);
        mpuAvailable = false;
    } else {
        mpuAvailable = true;
        Serial.println(F("✓ IMU ready (FastIMU)"));
        calibrateIMU();
    }
}

// ==================== Main Sensor Loop ====================

void handleMPU6050() {
    unsigned long now = millis();

    // Reset reaction after timeout
    if (isReacting && (now - reactionStartTime >= GESTURE_REACTION_MS)) {
        isReacting = false;
        resetEffects();
        MOCHI_CALL_VOID(setMood, DEFAULT);
        MOCHI_CALL_VOID(setPosition, DEFAULT);
        MOCHI_CALL_VOID(setMouthType, 1);
        Serial.println(F("[GESTURE] Reaction ended, ready to detect"));
    }

    if (isReacting) return;
    if (!mpuAvailable) return;
    if (now - lastMpuRead < MPU_SAMPLE_RATE_MS) return;

    lastMpuRead = now;

    // Read raw sensor data — MUST match data forwarder format exactly:
    //   Gyro: degrees per second (°/s)
    //   Accel: g-force (g)
    //   No filters, no unit conversion.
    IMU.update();
    IMU.getGyro(&gyroData);
    IMU.getAccel(&accelData);

    dpsGX    = gyroData.gyroX;
    dpsGY    = gyroData.gyroY;
    dpsGZ    = gyroData.gyroZ;
    gForceAX = accelData.accelX;
    gForceAY = accelData.accelY;
    gForceAZ = accelData.accelZ;

    // Debug output (raw, same format as data forwarder)
    if (mpuVerbose) {
        Serial.print(dpsGX, 4); Serial.print('\t');
        Serial.print(dpsGY, 4); Serial.print('\t');
        Serial.print(dpsGZ, 4); Serial.print('\t');
        Serial.print(gForceAX, 4); Serial.print('\t');
        Serial.print(gForceAY, 4); Serial.print('\t');
        Serial.println(gForceAZ, 4);
    }

    // Streaming mode (sends raw data to browser for EI data collection)
    if (isEIStreaming()) {
        streamEISample(dpsGX, dpsGY, dpsGZ, gForceAX, gForceAY, gForceAZ);
    }

    // Inference mode (feeds raw data to EI model — same as training)
    if (isEIMatchingEnabled()) {
        processEISample(dpsGX, dpsGY, dpsGZ, gForceAX, gForceAY, gForceAZ);
    }
}

#endif // IMU_MANAGER_H
