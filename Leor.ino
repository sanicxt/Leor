/*
 * Leor.ino - Animated Robot Eyes for ESP32 / ESP32-C3 / ESP32-C6
 * 
 * Copyright (C) 2026 sanicxt
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// ==================== Libraries ====================
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <NimBLEDevice.h>
#include <Preferences.h>

// ==================== Modules ====================
#include "config.h"
#include "display_manager.h"    // Display objects, MochiEyes, OLED helpers
#include "ota_manager.h"        // OTA firmware update service
#include "power_manager.h"      // Deep sleep, touch wake, board-specific GPIO
#include "imu_manager.h"        // IMU sensor, calibration, gesture processing
#include "shuffle_manager.h"    // Auto-expression shuffle
#include "ble_manager.h"        // NimBLE BLE stack
#include "commands.h"           // Command parser
#include "ei_gesture.h"         // Edge Impulse gesture inference

// ==================== Globals ====================
Preferences preferences;
static String inputBuffer = "";

// ==================== Serial Input ====================
void handleSerialInput() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                handleCommand(inputBuffer);
                inputBuffer = "";
            }
        } else {
            inputBuffer += c;
        }
    }
}

// ==================== Setup ====================
void setup() {
    Serial.begin(115200);
    delay(250);
    randomSeed((uint32_t)micros());

    preferences.begin("leor", false);

    // 1. Power — release holds, power on peripherals, configure touch wake
    initPower(preferences);

    Serial.println(F("\n============================="));
    Serial.println(F("leor v3.0"));
    Serial.println(F("=============================\n"));

    // 2. Display — init OLED + MochiEyes
    initDisplay(preferences);

    // 3. OTA display callback
    setOtaDisplayCallback(drawOtaScreen);

    // 4. Load appearance settings
    int ew = preferences.getInt("ew", 36);
    int eh = preferences.getInt("eh", 36);
    int es = preferences.getInt("es", 10);
    int er = preferences.getInt("er", 8);
    int mw = preferences.getInt("mw", 20);
    int lt = preferences.getInt("lt", 1000);
    int vt = preferences.getInt("vt", 2000);
    int bi = preferences.getInt("bi", 3);

    MOCHI_CALL_VOID(setWidth, ew, ew);
    MOCHI_CALL_VOID(setHeight, eh, eh);
    MOCHI_CALL_VOID(setSpacebetween, es);
    MOCHI_CALL_VOID(setBorderradius, er, er);
    MOCHI_CALL_VOID(setMouthSize, mw, 6);
    MOCHI_CALL_VOID(setLaughDuration, lt);
    MOCHI_CALL_VOID(setLoveDuration, vt);
    MOCHI_CALL_VOID(setAutoblinker, true, (float)bi, (float)BLINK_VARIATION);

    // 5. Load breathing settings
    bool breathEnabled   = preferences.getBool("br_en", true);
    float breathIntensity = preferences.getFloat("br_int", 0.08f);
    float breathSpeed     = preferences.getFloat("br_spd", 0.3f);
    MOCHI_CALL_VOID(setBreathing, breathEnabled, breathIntensity, breathSpeed);

    // 6. Shuffle
    initShuffle(preferences);

    // 7. BLE
    initBLE(BLE_DEVICE_NAME);
    bool bleLP = preferences.getBool("ble_lp", false);
    setBLELowPowerMode(bleLP);

    // 8. IMU + Gesture
    initIMU();
    initEIGesture();

    // Ready!
    MOCHI_CALL_VOID(setMood, 0);
    MOCHI_CALL_VOID(setPosition, 0);

    Serial.println(F("\n============================="));
    Serial.println(F("    leor Ready!"));
    Serial.println(F("============================="));
    Serial.println(F("Type 'help' for commands\n"));
}

// ==================== Main Loop ====================
void loop() {
    // Touch check FIRST — skip during OTA
    if (!isOtaInProgress()) {
        handleTouchWakeButton();
    }

    // During OTA skip all rendering
    if (isOtaInProgress()) {
        otaCheckReboot();
        delay(10);
        return;
    }

    // BLE connection state
    handleBLEConnection();

    // Update display (skip during training)
    bool training = isTraining();
    if (!training) {
        if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) {
            pMochiEyes_ssd1306->update();
        } else if (pMochiEyes_sh1106) {
            pMochiEyes_sh1106->update();
        }
    }

    // Streaming overlay
    if (isStreaming()) {
        drawStreamingOverlay();
    }

    // IMU + gesture detection
    handleMPU6050();

    // Auto expression shuffle (skip during training)
    if (!training) {
        maybeShuffleExpression();
    }

    // Serial commands
    handleSerialInput();
}
