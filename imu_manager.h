/*
 * imu_manager.h - IMU sensor, calibration, and gesture processing for leor
 */

#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "display_manager.h"
#include "ei_gesture.h"
#include "mpu/MPU6050_AHRS.h"

// Re-define DEFAULT (Edge Impulse #undefs it)
#ifndef DEFAULT
#define DEFAULT 0
#endif

// Forward declaration (defined in ble_manager.h)
void sendBLEGesture(const String& gesture);

// ==================== IMU Objects ====================
#define IMU_ADDRESS 0x68

MPU6050_AHRS mpu(IMU_ADDRESS);
MPU6050Data  mpuData;
bool         mpuAvailable  = false;
bool         mpuCalibrated  = false;

// Compatibility vars used by other modules
float dpsGX, dpsGY, dpsGZ;
float gForceAX, gForceAY, gForceAZ;

#define FREQUENCY_HZ 50
#define INTERVAL_MS  (1000 / FREQUENCY_HZ)
unsigned long last_interval_ms = 0;

// ==================== State ====================
unsigned long lastMpuRead         = 0;
unsigned long GESTURE_REACTION_MS = 1500;
unsigned long reactionStartTime   = 0;
bool          isReacting          = false;
bool          mpuVerbose          = false;

// Forward declaration (implemented in commands.h / Leor.ino)
String handleCommand(String cmd);
void resetEffects();

// ==================== Helpers ====================

void syncMpuCompatibilityData() {
    const MPU6050Data& data = mpu.getData();
    mpuData = data;

    dpsGX    = data.gxDps;
    dpsGY    = data.gyDps;
    dpsGZ    = data.gzDps;
    gForceAX = data.axG;
    gForceAY = data.ayG;
    gForceAZ = data.azG;
}

void drawMpuCalibrationScreen(int percent, uint16_t samples, bool complete) {
    uint16_t WHITE_COLOR = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;

    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        display_ssd1306->setTextColor(WHITE_COLOR);
        display_ssd1306->setTextSize(1);
        
        if (complete) {
            display_ssd1306->setCursor(16, 28);
            display_ssd1306->print(F("Calibration Done!"));
        } else {
            display_ssd1306->setCursor(20, 20);
            display_ssd1306->print(F("Calibrating IMU"));
            
            // Minimal thin progress bar
            display_ssd1306->drawRect(14, 35, 100, 6, WHITE_COLOR);
            int fillWidth = map(percent, 0, 100, 0, 96);
            display_ssd1306->fillRect(16, 37, fillWidth, 2, WHITE_COLOR);
        }
        display_ssd1306->display();
        pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        display_sh1106->setTextColor(WHITE_COLOR);
        display_sh1106->setTextSize(1);
        
        if (complete) {
            display_sh1106->setCursor(16, 28);
            display_sh1106->print(F("Calibration Done!"));
        } else {
            display_sh1106->setCursor(20, 20);
            display_sh1106->print(F("Calibrating IMU"));
            
            // Minimal thin progress bar
            display_sh1106->drawRect(14, 35, 100, 6, WHITE_COLOR);
            int fillWidth = map(percent, 0, 100, 0, 96);
            display_sh1106->fillRect(16, 37, fillWidth, 2, WHITE_COLOR);
        }
        display_sh1106->display();
        pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

void showMpuCalibrationProgress() {
    static int lastDrawnPercent = -1;
    uint16_t samples = mpu.calibrationProgress();
    if (samples > 500) samples = 500;
    
    // Only calculate progress in 10% chunks (0, 10, 20... 100)
    int percent = (int)map((long)samples, 0L, 500L, 0L, 10L) * 10;
    
    // Draw only when passing a 10% threshold or at the exact edges
    // This dramatically reduces I2C traffic, preventing bus collisions
    // between the MPU6050 and OLED display!
    if (percent != lastDrawnPercent || samples == 0 || samples == 500) {
        lastDrawnPercent = percent;
        drawMpuCalibrationScreen(percent, samples, samples >= 500);
    }
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
    mpuAvailable = mpu.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    if (!mpuAvailable) {
        Serial.println(F("IMU init error: MPU6050 not detected"));
        return;
    }

    Serial.println(F("IMU ready (MPU6050_AHRS)"));
    Serial.println(F("  Calibrating gyro (keep still)..."));

    unsigned long startMs = millis();
    const unsigned long calibrationTimeoutMs = 15000; // 15 seconds (plenty of time now that I2C is freed)
    while (!mpu.isCalibrated()) {
        if (millis() - startMs > calibrationTimeoutMs) {
            Serial.println(F("  IMU calibration timeout"));
            
            // Show timeout error directly on screen
            if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
                display_ssd1306->clearDisplay();
                display_ssd1306->setTextColor(SSD1306_WHITE);
                display_ssd1306->setTextSize(1);
                display_ssd1306->setCursor(10, 24);
                display_ssd1306->print(F("IMU TIMEOUT ERROR!"));
                display_ssd1306->display();
                pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            } else if (display_sh1106) {
                display_sh1106->clearDisplay();
                display_sh1106->setTextColor(SH110X_WHITE);
                display_sh1106->setTextSize(1);
                display_sh1106->setCursor(10, 24);
                display_sh1106->print(F("IMU TIMEOUT ERROR!"));
                display_sh1106->display();
                pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            }
            delay(3000);
            
            mpuAvailable = false;
            return;
        }

        mpu.update();
        showMpuCalibrationProgress();
        delay(2);
    }

    mpuCalibrated = true;
    drawMpuCalibrationScreen(100, 500, true);
    delay(1200);

    // Clear the screen buffer and push it so MochiEyes starts with a blank slate
    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
    }
    pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    Serial.println(F("  Done!"));
    syncMpuCompatibilityData();
    Serial.print(F("  Gyro bias: "));
    Serial.print(mpuData.gxDps, 2); Serial.print(", ");
    Serial.print(mpuData.gyDps, 2); Serial.print(", ");
    Serial.println(mpuData.gzDps, 2);
    Serial.print(F("  Accel: "));
    Serial.print(mpuData.axG, 3); Serial.print(", ");
    Serial.print(mpuData.ayG, 3); Serial.print(", ");
    Serial.println(mpuData.azG, 3);
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
    if (!mpuAvailable || !mpuCalibrated) return;
    if (now - lastMpuRead < MPU_SAMPLE_RATE_MS) return;

    lastMpuRead = now;

    if (!mpu.update()) {
        return;
    }

    syncMpuCompatibilityData();

    // Debug output (same format as data forwarder)
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
