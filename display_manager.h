/*
 * display_manager.h - Display initialization and OLED helpers for leor
 *
 * Manages SSD1306/SH1106 selection, MochiEyes instances, and
 * screen overlay functions (calibration, OTA, streaming, training).
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>
#include <Preferences.h>
#include "config.h"
#include "MochiEyes.h"

// ==================== Display Type ====================
enum DisplayType { DISP_SSD1306, DISP_SH1106 };
DisplayType activeDisplayType = DISP_SH1106;

// Display objects — one will be initialized based on preferences
Adafruit_SSD1306*  display_ssd1306 = nullptr;
Adafruit_SH1106G*  display_sh1106  = nullptr;
Adafruit_GFX*      display         = nullptr;
const char*        DISPLAY_TYPE    = "Unknown";

// ==================== MochiEyes Instances ====================
MochiEyes<Adafruit_SSD1306>*  pMochiEyes_ssd1306 = nullptr;
MochiEyes<Adafruit_SH1106G>* pMochiEyes_sh1106  = nullptr;

// Helper macro: call a method on whichever MochiEyes is active
#define MOCHI_CALL_VOID(method, ...) \
  do { \
    if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) \
      pMochiEyes_ssd1306->method(__VA_ARGS__); \
    else if (pMochiEyes_sh1106) \
      pMochiEyes_sh1106->method(__VA_ARGS__); \
  } while (0)

#define MOCHI_CALL(ret, method, ...) \
  do { \
    if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) \
      ret = pMochiEyes_ssd1306->method(__VA_ARGS__); \
    else if (pMochiEyes_sh1106) \
      ret = pMochiEyes_sh1106->method(__VA_ARGS__); \
  } while (0)

// Helper macro: get typed pointer to active instance (via void*)
#define MOCHI_GET(method) \
  (activeDisplayType == DISP_SSD1306 \
    ? pMochiEyes_ssd1306->method() \
    : pMochiEyes_sh1106->method())

#define GET_MOCHI() \
  (activeDisplayType == DISP_SSD1306 ? (void*)pMochiEyes_ssd1306 : (void*)pMochiEyes_sh1106)

// ==================== OLED Screen Helpers ====================

void drawCalibrationScreen(int progress, const char* status) {
    uint16_t WHITE_COLOR = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;

    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        display_ssd1306->setTextSize(1);
        display_ssd1306->setTextColor(WHITE_COLOR);
        display_ssd1306->setCursor(20, 5);
        display_ssd1306->print(F("IMU CALIBRATION"));
        display_ssd1306->setCursor(10, 25);
        display_ssd1306->print(status);
        display_ssd1306->drawRect(10, 42, 108, 12, WHITE_COLOR);
        int fillWidth = map(progress, 0, 100, 0, 104);
        display_ssd1306->fillRect(12, 44, fillWidth, 8, WHITE_COLOR);
        display_ssd1306->setCursor(50, 56);
        display_ssd1306->print(progress);
        display_ssd1306->print(F("%"));
        display_ssd1306->display();
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        display_sh1106->setTextSize(1);
        display_sh1106->setTextColor(WHITE_COLOR);
        display_sh1106->setCursor(20, 5);
        display_sh1106->print(F("IMU CALIBRATION"));
        display_sh1106->setCursor(10, 25);
        display_sh1106->print(status);
        display_sh1106->drawRect(10, 42, 108, 12, WHITE_COLOR);
        int fillWidth = map(progress, 0, 100, 0, 104);
        display_sh1106->fillRect(12, 44, fillWidth, 8, WHITE_COLOR);
        display_sh1106->setCursor(50, 56);
        display_sh1106->print(progress);
        display_sh1106->print(F("%"));
        display_sh1106->display();
    }
}

// OTA progress screen (called from BLE task via callback — keep it fast)
void drawOtaScreen(int pct, const char* line1, const char* line2) {
    uint16_t W = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;

    #define OTA_DRAW(d) \
    do { \
        (d)->clearDisplay(); \
        const char* title = line1 ? line1 : "OTA UPDATE"; \
        int16_t tw = (int16_t)strlen(title) * 6; \
        (d)->setTextSize(1); \
        (d)->setTextColor(W); \
        (d)->setCursor((128 - tw) / 2, 2); \
        (d)->print(title); \
        (d)->drawFastHLine(0, 12, 128, W); \
        if (line2) { \
            int16_t sw = (int16_t)strlen(line2) * 6; \
            (d)->setCursor((128 - sw) / 2, 16); \
            (d)->print(line2); \
        } \
        if (pct >= 0) { \
            int16_t barX = 4, barY = 28, barW = 120, barH = 14; \
            (d)->drawRoundRect(barX, barY, barW, barH, 3, W); \
            int fill = (int)((long)(pct) * (barW - 4) / 100); \
            if (fill > 0) { \
                (d)->fillRoundRect(barX + 2, barY + 2, fill, barH - 4, 2, W); \
            } \
            char _pb[5]; snprintf(_pb, sizeof(_pb), "%d%%", pct); \
            int16_t px = (128 - (int16_t)strlen(_pb) * 6) / 2; \
            (d)->setCursor(px, barY + 3); \
            (d)->setTextColor(pct > 40 ? 0 : W); \
            (d)->print(_pb); \
            (d)->setTextColor(W); \
        } \
        if (pct >= 0 && pct < 100) { \
            int dots = (millis() / 400) % 4; \
            char dotStr[5] = "   "; \
            for (int _d = 0; _d < dots; _d++) dotStr[_d] = '.'; \
            (d)->setCursor(4, 48); \
            (d)->print(F("Flashing")); \
            (d)->print(dotStr); \
        } else if (pct == 100) { \
            (d)->setCursor(40, 50); \
            (d)->setTextSize(1); \
            (d)->print(F("Rebooting...")); \
        } \
        (d)->display(); \
    } while (0)

    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        OTA_DRAW(display_ssd1306);
    } else if (display_sh1106) {
        OTA_DRAW(display_sh1106);
    }

    #undef OTA_DRAW
}

// Draw streaming overlay on OLED
void drawStreamingOverlay() {
    uint16_t WHITE_COLOR = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;

    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        display_ssd1306->setTextColor(WHITE_COLOR);
        display_ssd1306->setTextSize(2);
        display_ssd1306->setCursor(10, 2);
        display_ssd1306->print(F("STREAMING"));
        display_ssd1306->drawLine(0, 20, SCREEN_WIDTH, 20, WHITE_COLOR);
        display_ssd1306->setTextSize(1);
        if ((millis() / 300) % 2 == 0) {
            display_ssd1306->fillCircle(64, 40, 8, WHITE_COLOR);
        } else {
            display_ssd1306->drawCircle(64, 40, 8, WHITE_COLOR);
        }
        display_ssd1306->setCursor(30, 54);
        display_ssd1306->print(F("Recording..."));
        display_ssd1306->display();
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        display_sh1106->setTextColor(WHITE_COLOR);
        display_sh1106->setTextSize(2);
        display_sh1106->setCursor(10, 2);
        display_sh1106->print(F("STREAMING"));
        display_sh1106->drawLine(0, 20, SCREEN_WIDTH, 20, WHITE_COLOR);
        display_sh1106->setTextSize(1);
        if ((millis() / 300) % 2 == 0) {
            display_sh1106->fillCircle(64, 40, 8, WHITE_COLOR);
        } else {
            display_sh1106->drawCircle(64, 40, 8, WHITE_COLOR);
        }
        display_sh1106->setCursor(30, 54);
        display_sh1106->print(F("Recording..."));
        display_sh1106->display();
    }
}

// Gesture training overlay stub (defined in gesture_trainer.h)
void drawTrainingOverlay(const char* status, int progress);

// ==================== Display Initialization ====================

void initDisplay(Preferences& preferences) {
    String savedDispType = preferences.getString("disp_type", "sh1106");
    uint8_t savedDispAddr = preferences.getUInt("disp_addr", I2C_ADDRESS);

    if (savedDispType == "ssd1306") {
        activeDisplayType = DISP_SSD1306;
        DISPLAY_TYPE = "SSD1306";
        display_ssd1306 = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
        display = display_ssd1306;
    } else {
        activeDisplayType = DISP_SH1106;
        DISPLAY_TYPE = "SH1106";
        display_sh1106 = new Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
        display = display_sh1106;
    }

    Serial.print(F("Display Type: "));
    Serial.print(DISPLAY_TYPE);
    Serial.print(F(" @ 0x"));
    Serial.println(savedDispAddr, HEX);

    Wire.begin();

    bool ok = false;
    if (activeDisplayType == DISP_SSD1306) {
        ok = display_ssd1306->begin(SSD1306_SWITCHCAPVCC, savedDispAddr);
    } else {
        ok = display_sh1106->begin(savedDispAddr, true);
    }

    if (!ok) {
        Serial.print(F("⚠️  Display init failed at 0x"));
        Serial.println(savedDispAddr, HEX);
        Serial.println(F("Continuing without display — BLE will still activate."));
        Serial.println(F("Use display:addr or display:type commands via BLE to fix."));
        // Null out display pointers so MOCHI_CALL_VOID safely no-ops
        delete display_ssd1306; display_ssd1306 = nullptr;
        delete display_sh1106;  display_sh1106  = nullptr;
        display = nullptr;
        return;  // Skip MochiEyes init, but don't halt
    }

    // Initialize MochiEyes with the active display
    if (activeDisplayType == DISP_SSD1306) {
        static MochiEyes<Adafruit_SSD1306> mochiEyes(*display_ssd1306);
        pMochiEyes_ssd1306 = &mochiEyes;
        pMochiEyes_ssd1306->begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
    } else {
        static MochiEyes<Adafruit_SH1106G> mochiEyes(*display_sh1106);
        pMochiEyes_sh1106 = &mochiEyes;
        pMochiEyes_sh1106->begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
    }

    Serial.print(F("✓ Display initialized ("));
    Serial.print(DISPLAY_TYPE);
    Serial.print(F(" @ 0x"));
    Serial.print(savedDispAddr, HEX);
    Serial.println(F(")"));
}

#endif // DISPLAY_MANAGER_H
