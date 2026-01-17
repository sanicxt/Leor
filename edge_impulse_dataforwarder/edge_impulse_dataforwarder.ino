/*
 * Edge Impulse Data Forwarder - FastIMU with Calibration
 * 
 * Uses FastIMU library for counterfeit/generic IMU support with calibration.
 * Outputs gyro (°/s) and accelerometer (g) data at 24Hz for Edge Impulse.
 * 
 * Usage:
 * 1. Flash this sketch to your ESP32
 * 2. Wait for calibration (keep device still when prompted)
 * 3. Run: edge-impulse-data-forwarder --baud-rate 115200
 * 4. When prompted for sensor axes, enter: gX, gY, gZ, aX, aY, aZ
 */

#include "FastIMU.h"
#include <Wire.h>

#define IMU_ADDRESS 0x68
#define PERFORM_CALIBRATION  // Comment to skip calibration

// Use IMU_Generic for counterfeit sensors, or MPU6050 for genuine
IMU_Generic IMU;

calData calib = { 0 };
AccelData accelData;
GyroData gyroData;

void setup() {
    Wire.begin();
    Wire.setClock(400000);
    Serial.begin(115200);
    while (!Serial) ;
    
    Serial.println(F("\n=== Edge Impulse Data Forwarder ==="));
    Serial.println(F("Using FastIMU with calibration"));
    
    int err = IMU.init(calib, IMU_ADDRESS);
    if (err != 0) {
        Serial.print(F("Error initializing IMU: "));
        Serial.println(err);
        while (true) delay(1000);
    }
    Serial.println(F("✓ IMU initialized"));
    
#ifdef PERFORM_CALIBRATION
    Serial.println(F("\n--- CALIBRATION ---"));
    Serial.println(F("Keep device LEVEL and STILL..."));
    delay(2000);
    
    // Calibrate accelerometer and gyroscope
    IMU.calibrateAccelGyro(&calib);
    
    Serial.println(F("✓ Calibration done!"));
    Serial.print(F("Accel bias: "));
    Serial.print(calib.accelBias[0], 2); Serial.print(", ");
    Serial.print(calib.accelBias[1], 2); Serial.print(", ");
    Serial.println(calib.accelBias[2], 2);
    Serial.print(F("Gyro bias: "));
    Serial.print(calib.gyroBias[0], 2); Serial.print(", ");
    Serial.print(calib.gyroBias[1], 2); Serial.print(", ");
    Serial.println(calib.gyroBias[2], 2);
    
    // Re-init with calibration data
    IMU.init(calib, IMU_ADDRESS);
    delay(500);
#endif
    
    Serial.println(F("\n=== Starting data output ==="));
    Serial.println(F("Format: gX\\tgY\\tgZ\\taX\\taY\\taZ @ 24Hz"));
    delay(1000);
}

void loop() {
    IMU.update();
    
    IMU.getGyro(&gyroData);
    IMU.getAccel(&accelData);
    
    // Output in Edge Impulse data forwarder format (tab-separated)
    // Gyro in °/s, Accel in g
    Serial.print(gyroData.gyroX, 4);
    Serial.print('\t');
    Serial.print(gyroData.gyroY, 4);
    Serial.print('\t');
    Serial.print(gyroData.gyroZ, 4);
    Serial.print('\t');
    Serial.print(accelData.accelX, 4);
    Serial.print('\t');
    Serial.print(accelData.accelY, 4);
    Serial.print('\t');
    Serial.println(accelData.accelZ, 4);
    
    delay(43);  // 23Hz sample rate (matches Edge Impulse leor model)
}
