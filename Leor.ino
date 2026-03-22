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
#include <esp_mac.h>
#include <queue>

// ==================== Modules ====================
#include "config.h"
#include "display_manager.h"    // Display objects, MochiEyes, OLED helpers
#include "ota_manager.h"        // OTA firmware update service
#include "power_manager.h"      // Deep sleep, touch wake, board-specific GPIO
#include "imu_manager.h"        // IMU sensor, calibration, gesture processing
#include "mpu/MPU6050_AHRS.cpp" // MPU6050_AHRS implementation for Arduino build
#include "shuffle_manager.h"    // Auto-expression shuffle
#include "ble_manager.h"        // NimBLE BLE stack
#include "commands.h"           // Command parser
#include "ei_gesture.h"         // Edge Impulse gesture inference
#include "esp_pm.h"             // ESP32 power management

// ==================== Globals ====================
Preferences preferences;
static String inputBuffer = "";

// ==================== BLE Command Queue ====================
std::queue<String> bleCommandQueue;
SemaphoreHandle_t bleCommandMutex = NULL;

void enqueueBleCommand(String cmd) {
    if (bleCommandMutex && xSemaphoreTake(bleCommandMutex, portMAX_DELAY)) {
        bleCommandQueue.push(cmd);
        xSemaphoreGive(bleCommandMutex);
    }
}

void processBleCommands() {
    String cmd = "";
    if (bleCommandMutex && xSemaphoreTake(bleCommandMutex, 0)) {
        if (!bleCommandQueue.empty()) {
            cmd = bleCommandQueue.front();
            bleCommandQueue.pop();
        }
        xSemaphoreGive(bleCommandMutex);
    }
    
    if (cmd.length() > 0) {
        String response = handleCommand(cmd);
        if (response.length() > 0) {
            // sendBLEStatus is in ble_manager.h
            extern void sendBLEStatus(const String&);
            sendBLEStatus(response);
        }
    }
}

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
    
    // Initialize mutex for BLE command queue
    bleCommandMutex = xSemaphoreCreateMutex();

    // Enable Automatic Light Sleep
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 10,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pm_config);

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
    int bi = preferences.getInt("bi", 3);

    MOCHI_CALL_VOID(setWidth, ew, ew);
    MOCHI_CALL_VOID(setHeight, eh, eh);
    MOCHI_CALL_VOID(setSpacebetween, es);
    MOCHI_CALL_VOID(setBorderradius, er, er);
    MOCHI_CALL_VOID(setMouthSize, mw, 6);
    MOCHI_CALL_VOID(setAutoblinker, true, (float)bi, (float)BLINK_VARIATION);

    // 5. Load breathing settings
    bool breathEnabled   = preferences.getBool("br_en", true);
    float breathIntensity = preferences.getFloat("br_int", 0.08f);
    float breathSpeed     = preferences.getFloat("br_spd", 0.3f);
    MOCHI_CALL_VOID(setBreathing, breathEnabled, breathIntensity, breathSpeed);

    // 6. Shuffle
    initShuffle(preferences);

    // 6b. Clock mode
    restoreClockFromPreferences(preferences);

    // 7. BLE
    // If no custom name is saved, generate a unique one using the MAC address
    String bleName = preferences.getString("ble_name", "");
    if (bleName.length() == 0) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_BT);
        char defaultName[16];
        snprintf(defaultName, sizeof(defaultName), "Leor-%02X%02X", mac[4], mac[5]);
        bleName = String(defaultName);
    }
    initBLE(bleName.c_str());
    bool bleLP = preferences.getBool("ble_lp", false);
    setBLELowPowerMode(bleLP);
    setDisplayLowPowerMode(bleLP);

    // Initialize the default eye state before IMU calibration starts.
    // This avoids overwriting the final calibration screen immediately after initIMU().
    MOCHI_CALL_VOID(setMood, 0);
    MOCHI_CALL_VOID(setPosition, 0);

    // 8. IMU + Gesture
    initIMU();
    initEIGesture();

    // Re-arm touch detection now that all init is done
    // (replaces the 30s guard set in initPower so touch works immediately)
    armTouchDetect(500);

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
        if (isClockTransitionActive()) {
            if (!updateClockTransitionFrame()) {
                if (isClockEnabled()) {
                    drawClockScreen();
                } else if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) {
                    pMochiEyes_ssd1306->update();
                } else if (pMochiEyes_sh1106) {
                    pMochiEyes_sh1106->update();
                }
            }
        } else if (isClockEnabled()) {
            drawClockScreen();
        } else if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) {
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
    
    // Process queued BLE commands (prevents race conditions with MochiEyes)
    processBleCommands();

    // Yield to FreeRTOS to allow automatic light sleep
    vTaskDelay(pdMS_TO_TICKS(5));
}
