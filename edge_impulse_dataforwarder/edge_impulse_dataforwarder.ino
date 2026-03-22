/*
 * Edge Impulse Data Forwarder - MPU6050_AHRS
 * 
 * Uses the local MPU6050_AHRS implementation for calibration and raw data output.
 * Outputs gyro (°/s) and accelerometer (g) data at 24Hz for Edge Impulse.
 * 
 * Usage:
 * 1. Flash this sketch to your ESP32
 * 2. Wait for calibration (keep device still when prompted)
 * 3. Run: edge-impulse-data-forwarder --baud-rate 115200
 * 4. When prompted for sensor axes, enter: gX, gY, gZ, aX, aY, aZ
 */

#include "../mpu/MPU6050_AHRS.h"

#define IMU_ADDRESS 0x68

MPU6050_AHRS mpu(IMU_ADDRESS);

static void waitForCalibration() {
    Serial.println(F("\n--- CALIBRATION ---"));
    Serial.println(F("Keep device STILL..."));

    int lastPercent = -1;

    while (!mpu.isCalibrated()) {
        mpu.update();
        uint16_t progress = mpu.calibrationProgress();
        if (progress > 500) progress = 500;
        int percent = (int)map((long)progress, 0L, 500L, 0L, 100L);
        if (percent != lastPercent) {
            Serial.print(F("Calibration: "));
            Serial.print(percent);
            Serial.println(F("%"));
            lastPercent = percent;
        }
        delay(2);
    }

    Serial.println(F("Calibration done!"));
}

void setup() {
    Wire.begin();
    Wire.setClock(400000);
    Serial.begin(115200);
    while (!Serial) ;
    
    Serial.println(F("\n=== Edge Impulse Data Forwarder ==="));
    Serial.println(F("Using MPU6050_AHRS with calibration"));

    if (!mpu.begin()) {
        Serial.println(F("Error initializing IMU"));
        while (true) delay(1000);
    }
    Serial.println(F("IMU initialized"));

    waitForCalibration();
    
    Serial.println(F("\n=== Starting data output ==="));
    Serial.println(F("Format: gX\\tgY\\tgZ\\taX\\taY\\taZ @ 24Hz"));
    delay(1000);
}

void loop() {
    if (!mpu.update()) {
        return;
    }

    const MPU6050Data& d = mpu.getData();
    
    // Output in Edge Impulse data forwarder format (tab-separated)
    // Gyro in °/s, Accel in g
    Serial.print(d.gxDps, 4);
    Serial.print('\t');
    Serial.print(d.gyDps, 4);
    Serial.print('\t');
    Serial.print(d.gzDps, 4);
    Serial.print('\t');
    Serial.print(d.axG, 4);
    Serial.print('\t');
    Serial.print(d.ayG, 4);
    Serial.print('\t');
    Serial.println(d.azG, 4);
    
    delay(43);  // 23Hz sample rate (matches Edge Impulse leor model)
}
