/*
 * config.h - Configuration settings for leor
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== BLE Settings ====================
// BLE Device Name (shown when scanning for Bluetooth devices)
const char* BLE_DEVICE_NAME = "Leor";

// BLE Robustness Settings
#define BLE_NOTIFY_MIN_INTERVAL_MS 20   // Minimum ms between notifications (prevent flooding)
#define BLE_CONNECTION_TIMEOUT_MS 30000 // Detect stale connections (30 seconds no activity)
#define BLE_MTU_REQUEST 185             // Request larger MTU (ESP32 max is typically 517, but 185 is safer)
#define BLE_ADV_WATCHDOG_MS 60000       // Restart advertising if no connection after 60s

// Note: BLE uses significantly less power than WiFi
// Control via Bluetooth using a mobile app or web Bluetooth API

// ==================== Display Settings ====================
#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define FRAME_RATE 100  // FPS

// ==================== MPU6050 Sensor Settings ====================
#define MPU_SAMPLE_RATE_MS 20     // Read interval in ms
#define GESTURE_MAG_MIN 2.0f      // Min magnitude (rad/s) to start gesture sampling
#define GESTURE_COOLDOWN_MS 1500  // Cooldown after gesture match before next detection
#define GESTURE_REACTION_MS 1500  // How long to show reaction expression

// ==================== Autoblinker Settings ====================
#define BLINK_INTERVAL 3          // Base interval in seconds
#define BLINK_VARIATION 3         // Random variation in seconds

#endif // CONFIG_H
