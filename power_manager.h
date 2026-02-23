/*
 * power_manager.h - Power control, deep sleep, and touch wake for leor
 *
 * Board-specific: uses #if SOC_RTCIO_PIN_COUNT to separate
 * ESP32 (rtc_gpio) vs ESP32-C3/C6 (gpio) APIs.
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "driver/gpio.h"
#include "config.h"
#include "display_manager.h"

// Board-specific: ESP32/S2/S3 have RTC GPIO, C3/C6 do not
#if SOC_RTCIO_PIN_COUNT > 0
  #include "driver/rtc_io.h"
#endif

// ==================== State Variables ====================
unsigned long touchHoldMs = TOUCH_HOLD_DEFAULT_MS;
bool touchSleepArmed = false;

// Runtime-configurable GPIO pins (loaded from preferences; default from config.h)
uint8_t runtimeTouchPin = TOUCH_WAKE_PIN;
uint8_t runtimePwrPin   = PWR_CTRL_PIN;

// Touch detection state (simple hold timer)
unsigned long touchPressStartMs   = 0;
bool          touchLastState      = false;
unsigned long touchDetectEnableAtMs = 0;

// ==================== Helpers ====================

inline bool isTouchWakePressed() {
    int pinState = digitalRead(runtimeTouchPin);
    return (TOUCH_WAKE_ACTIVE_LEVEL == 1) ? (pinState == HIGH) : (pinState == LOW);
}

// ==================== Deep Sleep ====================

void enterDeepSleepFromTouch() {
    Serial.println(F("[POWER] Long-touch detected → entering deep sleep"));

    // --- 1. Show "Bye bye!" on OLED ---
    uint8_t dispAddr = I2C_ADDRESS;
    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        display_ssd1306->setTextColor(SSD1306_WHITE);
        display_ssd1306->setTextSize(2);
        display_ssd1306->setCursor(28, 24);
        display_ssd1306->print(F("Bye bye!"));
        display_ssd1306->display();
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        display_sh1106->setTextColor(SH110X_WHITE);
        display_sh1106->setTextSize(2);
        display_sh1106->setCursor(28, 24);
        display_sh1106->print(F("Bye bye!"));
        display_sh1106->display();
    }
    delay(300);

    // --- 1b. Display OFF command (0xAE) ---
    Wire.beginTransmission(dispAddr);
    Wire.write(0x00);
    Wire.write(0xAE);
    Wire.endTransmission();

    // --- 2. Put MPU-6050 into sleep mode ---
    Wire.beginTransmission(0x68);  // IMU_ADDRESS
    Wire.write(0x6B);
    Wire.write(0x40);  // SLEEP = 1
    Wire.endTransmission();

    // --- 3. Release I2C bus and kill all signal-line current paths ---
    Wire.end();

#if SOC_RTCIO_PIN_COUNT > 0
    // ESP32 / S2 / S3 — use RTC GPIO API
    // Drive SDA/SCL LOW via rtc_gpio to prevent parasitic powering
    rtc_gpio_set_level((gpio_num_t)SDA, 0);
    rtc_gpio_hold_en((gpio_num_t)SDA);
    rtc_gpio_set_level((gpio_num_t)SCL, 0);
    rtc_gpio_hold_en((gpio_num_t)SCL);

    // Cut peripheral power
    rtc_gpio_set_level((gpio_num_t)runtimePwrPin, 1);
    rtc_gpio_hold_en((gpio_num_t)runtimePwrPin);
#else
    // ESP32-C3 / C6 — use standard GPIO API
    // Drive SDA/SCL LOW and hold to prevent parasitic powering through ESD diodes
    gpio_set_direction((gpio_num_t)SDA, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SDA, 0);
    gpio_pullup_dis((gpio_num_t)SDA);
    gpio_pulldown_dis((gpio_num_t)SDA);
    gpio_hold_en((gpio_num_t)SDA);

    gpio_set_direction((gpio_num_t)SCL, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)SCL, 0);
    gpio_pullup_dis((gpio_num_t)SCL);
    gpio_pulldown_dis((gpio_num_t)SCL);
    gpio_hold_en((gpio_num_t)SCL);

    // Cut peripheral power
    gpio_set_level((gpio_num_t)runtimePwrPin, 1);
    gpio_hold_en((gpio_num_t)runtimePwrPin);
#endif

    // --- 4. Stop BLE advertising ---
    NimBLEDevice::getAdvertising()->stop();
    delay(30);

    // --- 5. Configure GPIO wake source ---
#if SOC_RTCIO_PIN_COUNT > 0
    esp_sleep_enable_ext1_wakeup_io(
        (1ULL << runtimeTouchPin),
        (TOUCH_WAKE_ACTIVE_LEVEL == 0) ? ESP_EXT1_WAKEUP_ANY_LOW : ESP_EXT1_WAKEUP_ANY_HIGH
    );
#else
    esp_deep_sleep_enable_gpio_wakeup(
        (1ULL << runtimeTouchPin),
        (TOUCH_WAKE_ACTIVE_LEVEL == 0) ? ESP_GPIO_WAKEUP_GPIO_LOW : ESP_GPIO_WAKEUP_GPIO_HIGH
    );
#endif

    // Hold pin at the NON-active level during sleep
#if SOC_RTCIO_PIN_COUNT > 0
    if (TOUCH_WAKE_ACTIVE_LEVEL == 0) {
        rtc_gpio_pullup_en((gpio_num_t)runtimeTouchPin);
        rtc_gpio_pulldown_dis((gpio_num_t)runtimeTouchPin);
    } else {
        rtc_gpio_pulldown_en((gpio_num_t)runtimeTouchPin);
        rtc_gpio_pullup_dis((gpio_num_t)runtimeTouchPin);
    }
    rtc_gpio_hold_en((gpio_num_t)runtimeTouchPin);
#else
    if (TOUCH_WAKE_ACTIVE_LEVEL == 0) {
        gpio_pullup_en((gpio_num_t)runtimeTouchPin);
        gpio_pulldown_dis((gpio_num_t)runtimeTouchPin);
    } else {
        gpio_pulldown_en((gpio_num_t)runtimeTouchPin);
        gpio_pullup_dis((gpio_num_t)runtimeTouchPin);
    }
    gpio_hold_en((gpio_num_t)runtimeTouchPin);
#endif

    // Wait for button release before sleeping
    Serial.println(F("[POWER] Waiting for button release before sleep..."));
    Serial.flush();
    unsigned long releaseWaitStart = millis();
    while (isTouchWakePressed()) {
        if (millis() - releaseWaitStart > 5000) {
            Serial.println(F("[POWER] Sleep aborted: button held too long"));
            touchPressStartMs = 0;
            return;
        }
        delay(10);
    }
    delay(50);

    Serial.println(F("[POWER] Button released. Sleeping now. Press to wake."));
    Serial.flush();

#if SOC_RTCIO_PIN_COUNT == 0
    // C3/C6: keep digital GPIO power domain alive so holds on non-RTC pins survive
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
#endif

    esp_deep_sleep_start();
}

// ==================== Touch Button Handler ====================

void handleTouchWakeButton() {
    if (millis() < touchDetectEnableAtMs) return;

    bool currentState = isTouchWakePressed();
    unsigned long now = millis();

    // LOW → HIGH edge: touched
    if (!touchLastState && currentState) {
        touchPressStartMs = now;
        Serial.println(F("[POWER] Tap pressed"));
    }

    // HIGH → LOW edge: released
    if (touchLastState && !currentState) {
        if (touchPressStartMs != 0) {
            touchPressStartMs = 0;
            Serial.println(F("[POWER] Hold cancelled"));
        }
    }

    // Check hold duration
    if (currentState && touchPressStartMs != 0) {
        unsigned long held = now - touchPressStartMs;
        if (touchSleepArmed && held >= touchHoldMs) {
            touchPressStartMs = 0;
            if (!isOtaInProgress()) {
                enterDeepSleepFromTouch();
            }
        }
    }

    touchLastState = currentState;
}

// ==================== Power Init (called from setup) ====================

void initPower(Preferences& preferences) {
    runtimeTouchPin = (uint8_t)preferences.getUInt("wake_pin", TOUCH_WAKE_PIN);
    runtimePwrPin   = (uint8_t)preferences.getUInt("pwr_pin",  PWR_CTRL_PIN);

    // Release holds from previous deep sleep and power on peripherals
#if SOC_RTCIO_PIN_COUNT > 0
    rtc_gpio_hold_dis((gpio_num_t)runtimePwrPin);
    rtc_gpio_init((gpio_num_t)runtimePwrPin);
    rtc_gpio_set_direction((gpio_num_t)runtimePwrPin, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)runtimePwrPin, 0);
    delay(20);
    rtc_gpio_hold_dis((gpio_num_t)runtimeTouchPin);
    rtc_gpio_hold_dis((gpio_num_t)SDA);
    rtc_gpio_hold_dis((gpio_num_t)SCL);
#else
    gpio_hold_dis((gpio_num_t)runtimePwrPin);
    gpio_reset_pin((gpio_num_t)runtimePwrPin);
    gpio_set_direction((gpio_num_t)runtimePwrPin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)runtimePwrPin, 0);
    delay(20);
    gpio_hold_dis((gpio_num_t)runtimeTouchPin);
    gpio_hold_dis((gpio_num_t)SDA);
    gpio_hold_dis((gpio_num_t)SCL);
#endif

    // Touch wake pin setup
    if (TOUCH_WAKE_USE_PULLUP) {
        pinMode(runtimeTouchPin, INPUT_PULLUP);
    } else {
        pinMode(runtimeTouchPin, INPUT);
    }
    delay(10);
    touchPressStartMs = 0;
    touchLastState = isTouchWakePressed();
    touchDetectEnableAtMs = millis() + 1000;
    touchHoldMs = preferences.getUInt("touch_ms", TOUCH_HOLD_DEFAULT_MS);
    if (touchHoldMs < 1000) touchHoldMs = 1000;
    if (touchHoldMs > 15000) touchHoldMs = 15000;

    // Check wake cause
    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    bool isTouchWake = (wakeCause == ESP_SLEEP_WAKEUP_EXT1 ||
                        wakeCause == ESP_SLEEP_WAKEUP_GPIO ||
                        wakeCause == ESP_SLEEP_WAKEUP_EXT0);

    if (isTouchWake) {
        Serial.println(F("[POWER] Wakeup from deep sleep. Booting in 1s..."));
        delay(1000);
        Serial.println(F("[POWER] Starting up."));
        touchPressStartMs = 0;
        touchLastState = isTouchWakePressed();
        touchDetectEnableAtMs = millis() + 1000;
    }

    touchSleepArmed = true;
}

#endif // POWER_MANAGER_H
