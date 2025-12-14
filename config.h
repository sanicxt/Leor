/*
 * config.h - Configuration settings for Leora
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== WiFi Settings ====================
// WiFi credentials - CHANGE THESE to your network
const char* WIFI_SSID = "Charmander";
const char* WIFI_PASSWORD = "sage312@";

// Access Point settings (used if WiFi connection fails)
const char* AP_SSID = "Leora";
const char* AP_PASSWORD = "leora123";  // Min 8 characters, or "" for open

// Hostname for WiFi (STA) + mDNS (if enabled elsewhere)
#define HOSTNAME "leora-bot"

// ==================== Display Settings ====================
#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define FRAME_RATE 100  // FPS

// ==================== TOF Sensor Settings ====================
#define TOF_INTERVAL 80           // Read interval in ms (balance speed vs performance)
#define GESTURE_DISTANCE_CM 15    // Max distance to detect gestures (cm)
#define GESTURE_MIN_CM 3          // Minimum valid distance (cm)

// Gesture detection
#define PAT_THRESHOLD 30          // Distance change (mm) to count as movement (filter noise)
#define PAT_COUNT_THRESHOLD 4     // Number of direction changes for pat (more reliable)
#define PAT_WINDOW_MS 1500        // Pat movements must happen within this time
#define RUB_TIME_MS 1200          // Time hand must stay steady for rub (longer = more intentional)
#define RUB_TOLERANCE 15          // Max movement (mm) still considered "steady"
#define GESTURE_TIMEOUT_MS 800    // Reset gesture if no hand detected
#define GESTURE_REACTION_MS 1500  // How long to show reaction (reduced for responsiveness)

// ==================== Autoblinker Settings ====================
#define BLINK_INTERVAL 3          // Base interval in seconds
#define BLINK_VARIATION 3         // Random variation in seconds

#endif // CONFIG_H
