/*
 * power_manager.h - Power control, deep sleep, and touch wake for leor
 *
 * ============================================================
 * HARDWARE DESIGN
 * ============================================================
 * A PNP transistor (BC557, TO-92) is a high-side switch that cuts power
 * to the display (SH1106) and IMU (MPU-6050) during deep sleep.
 *
 *   3.3V ── Emitter (pin 1)
 *                │
 *              BC557
 *                │
 *   VCC_PERIPH ── Collector (pin 3) ── display VCC, IMU VCC
 *
 *   GPIO1 ──[1kΩ]── Base (pin 2)
 *
 *   GPIO1 LOW  → transistor ON  → peripherals powered  (normal operation)
 *   GPIO1 HIGH → transistor OFF → peripherals cut       (deep sleep)
 *
 * TO-92 pinout (flat side toward you, leads pointing down): 1=E  2=B  3=C
 *
 * I2C PULL-UP RESISTORS must connect to VCC_PERIPH (the switched rail),
 * NOT to always-on 3.3V. If wired to always-on 3.3V, leakage through
 * ESD diodes keeps ~2.7V on VCC_PERIPH even when the PNP is off,
 * causing faint OLED glow and IMU corruption during sleep.
 *
 * ============================================================
 * DEEP SLEEP SEQUENCE (ESP32-C3)
 * ============================================================
 *  1. Show "Bye bye!" on OLED, send display-off (0xAE), sleep IMU.
 *  2. Wire.end() — detach I2C peripheral from IO_MUX.
 *  3. esp_rom_gpio_pad_select_gpio(SDA/SCL) — hand pad back to GPIO so
 *     gpio_set_level(LOW) wins over the I2C idle-HIGH which would
 *     otherwise parasitically power VCC_PERIPH through ESD diodes.
 *     Drive SDA/SCL LOW + gpio_hold_en.
 *  4. Release the LOW hold on GPIO1, drive HIGH (PNP off), re-hold.
 *  5. Configure GPIO wake source + touch pin hold.
 *  6. gpio_deep_sleep_hold_en() — latches all gpio_hold_en() state
 *     across deep sleep (C3/C6 requirement).
 *  7. esp_deep_sleep_start().
 *
 * WAKEUP SEQUENCE (initPower, called at top of setup())
 *  1. gpio_deep_sleep_hold_dis() — must come before gpio_hold_dis().
 *  2. gpio_hold_dis() on all held pins.
 *  3. gpio_set_level(runtimePwrPin, LOW) → PNP ON → VCC_PERIPH rises.
 *  4. gpio_hold_en(runtimePwrPin) — latch LOW so any glitch can't float it.
 *  5. delay(20) — wait for rail to stabilise before I2C/display init.
 * ============================================================
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "config.h"
#include "display_manager.h"

#if SOC_RTCIO_PIN_COUNT > 0
  #include "driver/rtc_io.h"
#endif

// ==================== State ====================
unsigned long touchHoldMs           = TOUCH_HOLD_DEFAULT_MS;
bool          touchSleepArmed       = false;
uint8_t       runtimeTouchPin       = TOUCH_WAKE_PIN;
uint8_t       runtimePwrPin         = PWR_CTRL_PIN;
unsigned long touchPressStartMs     = 0;
bool          touchLastState        = false;
unsigned long touchDetectEnableAtMs = 0;

// ==================== Helper ====================

inline bool isTouchWakePressed() {
    return (TOUCH_WAKE_ACTIVE_LEVEL == 1)
        ? (digitalRead(runtimeTouchPin) == HIGH)
        : (digitalRead(runtimeTouchPin) == LOW);
}

// ==================== Deep Sleep Entry ====================

void enterDeepSleepFromTouch() {
    Serial.println(F("[POWER] Long-touch → entering deep sleep"));

    // 1. Goodbye screen
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

    // 2. Display OFF (0xAE)
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x00);
    Wire.write(0xAE);
    Wire.endTransmission();

    // 3. IMU sleep mode
    Wire.beginTransmission(0x68);
    Wire.write(0x6B);
    Wire.write(0x40);
    Wire.endTransmission();

    // 4. Release I2C bus
    Wire.end();

    // 5. Drive SDA/SCL LOW and hold — blocks pull-up leakage into VCC_PERIPH.
    //    On C3/C6, esp_rom_gpio_pad_select_gpio() must be called first to
    //    wrest IO_MUX ownership from the I2C peripheral (whose idle-HIGH
    //    would otherwise override our gpio_set_level).
#if SOC_RTCIO_PIN_COUNT > 0
    rtc_gpio_set_level((gpio_num_t)I2C_SDA_PIN, 0);  rtc_gpio_hold_en((gpio_num_t)I2C_SDA_PIN);
    rtc_gpio_set_level((gpio_num_t)I2C_SCL_PIN, 0);  rtc_gpio_hold_en((gpio_num_t)I2C_SCL_PIN);
#else
    esp_rom_gpio_pad_select_gpio(I2C_SDA_PIN);
    gpio_set_direction((gpio_num_t)I2C_SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)I2C_SDA_PIN, 0);
    gpio_pullup_dis((gpio_num_t)I2C_SDA_PIN);
    gpio_pulldown_dis((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_en((gpio_num_t)I2C_SDA_PIN);

    esp_rom_gpio_pad_select_gpio(I2C_SCL_PIN);
    gpio_set_direction((gpio_num_t)I2C_SCL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)I2C_SCL_PIN, 0);
    gpio_pullup_dis((gpio_num_t)I2C_SCL_PIN);
    gpio_pulldown_dis((gpio_num_t)I2C_SCL_PIN);
    gpio_hold_en((gpio_num_t)I2C_SCL_PIN);
#endif

    // 6. Cut peripheral power: GPIO1 HIGH → PNP off → VCC_PERIPH cut.
    //    Release the existing LOW hold first; gpio_set_level while held is ignored.
#if SOC_RTCIO_PIN_COUNT > 0
    rtc_gpio_set_level((gpio_num_t)runtimePwrPin, 1);
    rtc_gpio_hold_en((gpio_num_t)runtimePwrPin);
#else
    gpio_hold_dis((gpio_num_t)runtimePwrPin);
    gpio_set_level((gpio_num_t)runtimePwrPin, 1);
    gpio_hold_en((gpio_num_t)runtimePwrPin);
#endif

    // 7. Stop BLE
    NimBLEDevice::getAdvertising()->stop();
    delay(30);

    // 8. GPIO wake source
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

    // 9. Hold touch pin at its INACTIVE level during sleep
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

    // 10. Wait for button release before sleeping
    Serial.println(F("[POWER] Waiting for button release..."));
    Serial.flush();
    {
        unsigned long t0 = millis();
        while (isTouchWakePressed()) {
            if (millis() - t0 > 5000) {
                Serial.println(F("[POWER] Sleep aborted: button held too long"));
                touchPressStartMs = 0;
                // Restore PNP-ON state so normal operation continues
#if SOC_RTCIO_PIN_COUNT > 0
                rtc_gpio_hold_dis((gpio_num_t)runtimePwrPin);
                rtc_gpio_set_level((gpio_num_t)runtimePwrPin, 0);
#else
                gpio_hold_dis((gpio_num_t)runtimePwrPin);
                gpio_set_level((gpio_num_t)runtimePwrPin, 0);
                gpio_hold_en((gpio_num_t)runtimePwrPin);
#endif
                return;
            }
            delay(10);
        }
    }
    delay(50);

    Serial.println(F("[POWER] Sleeping. Press to wake."));
    Serial.flush();

#if SOC_RTCIO_PIN_COUNT == 0
    // Latch all gpio_hold_en() state across deep sleep.
    // Without this every hold is silently released on sleep entry and
    // GPIO1 floats → PNP base undriven → VCC_PERIPH not fully cut.
    gpio_deep_sleep_hold_en();
#endif

    esp_deep_sleep_start();
}

// ==================== Touch Button Handler ====================

void handleTouchWakeButton() {
    if (millis() < touchDetectEnableAtMs) return;

    bool currentState = isTouchWakePressed();
    unsigned long now = millis();

    if (!touchLastState && currentState) {
        touchPressStartMs = now;
        Serial.println(F("[POWER] Tap pressed"));
    }
    if (touchLastState && !currentState) {
        if (touchPressStartMs != 0) {
            touchPressStartMs = 0;
            Serial.println(F("[POWER] Hold cancelled"));
        }
    }
    if (currentState && touchPressStartMs != 0) {
        if (touchSleepArmed && (now - touchPressStartMs) >= touchHoldMs) {
            touchPressStartMs = 0;
            if (!isOtaInProgress()) enterDeepSleepFromTouch();
        }
    }

    touchLastState = currentState;
}

// ==================== Power Init (called from setup) ====================

void initPower(Preferences& preferences) {
    runtimeTouchPin = (uint8_t)preferences.getUInt("wake_pin", TOUCH_WAKE_PIN);
    runtimePwrPin   = (uint8_t)preferences.getUInt("pwr_pin",  PWR_CTRL_PIN);

    // ---- 1. Release any holds left by a previous deep sleep ----
    // gpio_deep_sleep_hold_dis() MUST come before gpio_hold_dis(); while the
    // deep-sleep latch is active, gpio_hold_dis() is silently ignored.
#if SOC_RTCIO_PIN_COUNT > 0
    rtc_gpio_hold_dis((gpio_num_t)runtimePwrPin);
    rtc_gpio_hold_dis((gpio_num_t)runtimeTouchPin);
    rtc_gpio_hold_dis((gpio_num_t)I2C_SDA_PIN);
    rtc_gpio_hold_dis((gpio_num_t)I2C_SCL_PIN);
#else
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)runtimePwrPin);
    gpio_hold_dis((gpio_num_t)runtimeTouchPin);
    gpio_hold_dis((gpio_num_t)I2C_SDA_PIN);
    gpio_hold_dis((gpio_num_t)I2C_SCL_PIN);
#endif

    // ---- 2. Power on peripherals: GPIO1 LOW → PNP ON → VCC_PERIPH rises ----
#if SOC_RTCIO_PIN_COUNT > 0
    rtc_gpio_init((gpio_num_t)runtimePwrPin);
    rtc_gpio_set_direction((gpio_num_t)runtimePwrPin, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)runtimePwrPin, 0);
#else
    // gpio_config() is atomic: sets direction, pull mode, and interrupt in one
    // shot — the pin never floats between steps (unlike gpio_reset_pin() which
    // has a brief floating-input window that can glitch the PNP base).
    {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask  = (1ULL << runtimePwrPin);
        cfg.mode          = GPIO_MODE_OUTPUT;
        cfg.pull_up_en    = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en  = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type     = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
    }
    gpio_set_level((gpio_num_t)runtimePwrPin, 0);  // LOW → PNP ON
    gpio_hold_en((gpio_num_t)runtimePwrPin);        // latch; won't float on glitch
#endif

    // Wait for VCC_PERIPH rail to stabilise before any I2C traffic
    delay(20);

    // ---- 3. Built-in LED sleep config (GPIO8, active-LOW) ----
    // gpio_sleep_sel_en() registers a separate pad config that fires automatically
    // on sleep entry; no action needed in enterDeepSleepFromTouch().
    // INPUT mode removes the output driver entirely; PULLUP brings the cathode
    // to ~3.3V (same as anode) → zero voltage across LED → no current → off.
#if defined(BUILTIN_SLEEP_LED_PIN) && BUILTIN_SLEEP_LED_PIN >= 0
    gpio_sleep_sel_en((gpio_num_t)BUILTIN_SLEEP_LED_PIN);
    gpio_sleep_set_direction((gpio_num_t)BUILTIN_SLEEP_LED_PIN, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode((gpio_num_t)BUILTIN_SLEEP_LED_PIN, GPIO_PULLUP_ONLY);
#endif

    // ---- 4. Touch wake pin: hold INACTIVE level to prevent phantom wakes ----
    if (TOUCH_WAKE_ACTIVE_LEVEL == 1) {
        pinMode(runtimeTouchPin, INPUT_PULLDOWN);  // active-HIGH → idle LOW
    } else {
        pinMode(runtimeTouchPin, INPUT_PULLUP);    // active-LOW  → idle HIGH
    }
    delay(10);

    touchPressStartMs = 0;
    touchLastState    = isTouchWakePressed();
    // 30s guard covers BLE stack + IMU calibration (5–15s typical).
    // armTouchDetect(500) at end of setup() re-arms precisely when ready.
    touchDetectEnableAtMs = millis() + 30000;

    touchHoldMs = preferences.getUInt("touch_ms", TOUCH_HOLD_DEFAULT_MS);
    if (touchHoldMs < 1000)  touchHoldMs = 1000;
    if (touchHoldMs > 15000) touchHoldMs = 15000;

    // ---- 5. Shorten guard on wakeup from deep sleep ----
    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    bool fromSleep = (wakeCause == ESP_SLEEP_WAKEUP_EXT1 ||
                      wakeCause == ESP_SLEEP_WAKEUP_GPIO  ||
                      wakeCause == ESP_SLEEP_WAKEUP_EXT0);
    if (fromSleep) {
        Serial.println(F("[POWER] Wakeup from deep sleep. Booting in 1s..."));
        delay(1000);
        Serial.println(F("[POWER] Starting up."));
        touchPressStartMs     = 0;
        touchLastState        = isTouchWakePressed();
        touchDetectEnableAtMs = millis() + 1000;
    }

    touchSleepArmed = true;
}

// ==================== Re-arm (call at end of setup) ====================
// Replaces the 30s init guard with a short settle delay so touch is
// active as soon as all peripherals are ready.
inline void armTouchDetect(unsigned long delayMs = 500) {
    touchPressStartMs     = 0;
    touchLastState        = isTouchWakePressed();
    touchDetectEnableAtMs = millis() + delayMs;
}

#endif // POWER_MANAGER_H
