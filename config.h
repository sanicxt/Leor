/*
 * config.h - Configuration settings for leor
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

#ifndef CONFIG_H
#define CONFIG_H

// ==================== BLE Settings ====================
// BLE Device Name (shown when scanning for Bluetooth devices)
const char* BLE_DEVICE_NAME = "Leor2";

// BLE Robustness Settings
#define BLE_NOTIFY_MIN_INTERVAL_MS 20   // Minimum ms between notifications (prevent flooding)
#define BLE_CONNECTION_TIMEOUT_MS 30000 // Detect stale connections (30 seconds no activity)
#define BLE_MTU_REQUEST 185             // Request larger MTU (ESP32 max is typically 517, but 185 is safer)
#define BLE_ADV_WATCHDOG_MS 60000       // Restart advertising if no connection after 60s

// BLE Power Saving Settings
#define BLE_TX_POWER_LOW   ESP_PWR_LVL_N3   // -3dBm (low power, ~10m range)
#define BLE_TX_POWER_HIGH  ESP_PWR_LVL_P9   // +9dBm (high power, ~30m range)
#define BLE_ADV_INTERVAL_FAST  100          // Fast advertising interval (ms) - for quick connection
#define BLE_ADV_INTERVAL_SLOW  1000         // Slow advertising interval (ms) - power saving mode

// Note: BLE uses significantly less power than WiFi
// Control via Bluetooth using a mobile app or web Bluetooth API

// ==================== Display Settings ====================
// Display type is selected at runtime via preferences (disp_type: sh1106 or ssd1306)
// Change via command: display:type=sh1106 or display:type=ssd1306 (requires restart)

#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define FRAME_RATE 100  // FPS

// ==================== MPU6050 Sensor Settings ====================
#define MPU_SAMPLE_RATE_MS 20     // Read interval in ms
#define GESTURE_MAG_MIN 2.0f      // Min magnitude (rad/s) to start gesture sampling
#define GESTURE_COOLDOWN_MS 1500  // Cooldown after gesture match before next detection
// Note: GESTURE_REACTION_MS is now a modifiable variable in pio.ino

// ==================== Autoblinker Settings ====================
#define BLINK_INTERVAL 3          // Base interval in seconds
#define BLINK_VARIATION 3         // Random variation in seconds

// ==================== Touch Wake / Deep Sleep ====================
// Use a digital touch/button signal pin to enter deep sleep and wake up.
// Default wiring assumes touch module outputs HIGH when touched.
#define TOUCH_WAKE_PIN 0
#define TOUCH_WAKE_ACTIVE_LEVEL 1      // 0 = LOW is pressed, 1 = HIGH is pressed
#define TOUCH_WAKE_USE_PULLUP false
#define TOUCH_HOLD_DEFAULT_MS 3000     // Long-touch hold duration (max 3s via WebUI)

// ==================== Power Control (PNP Transistor) ====================
// PNP transistor base connected to this pin.
// LOW  → transistor ON  → peripheral power rail energized (normal operation)
// HIGH → transistor OFF → peripheral power rail cut (deep sleep)
#define PWR_CTRL_PIN 1

#endif // CONFIG_H
