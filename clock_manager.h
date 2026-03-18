/*
 * clock_manager.h - Clock state, formatting, sync, and OLED rendering for leor
 */

#ifndef CLOCK_MANAGER_H
#define CLOCK_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <esp_attr.h>
#include <esp_sleep.h>
#include <time.h>
#include "config.h"
#include "ble_manager.h"
#include "display_manager.h"

// ==================== Clock Mode ====================
RTC_DATA_ATTR static bool     clockRtcValid   = false;
RTC_DATA_ATTR static bool     clockRtcEnabled = false;
RTC_DATA_ATTR static bool     clockRtcHasTime = false;
RTC_DATA_ATTR static bool     clockRtc24Hour  = true;
RTC_DATA_ATTR static uint32_t clockRtcSeconds = 0;
RTC_DATA_ATTR static uint64_t clockRtcEpochMs = 0;
RTC_DATA_ATTR static int16_t  clockRtcTzOffset = 0;

static bool     clockModeEnabled = false;
static bool     clockHasTime     = false;
static uint32_t clockBaseMillis  = 0;
static uint32_t clockBaseSeconds  = 0;
static uint64_t clockBaseEpochMs = 0;
static int16_t  clockTimezoneOffsetMinutes = 0;
static bool     clockUse24Hour   = true;
static uint32_t clockLastDrawKey = UINT32_MAX;

inline uint32_t getClockSecondsOfDay();

inline void persistClockToRtc() {
    clockRtcValid = true;
    clockRtcEnabled = clockModeEnabled;
    clockRtcHasTime = clockHasTime;
    clockRtc24Hour = clockUse24Hour;
    clockRtcSeconds = clockHasTime ? getClockSecondsOfDay() : 0;
    clockRtcEpochMs = clockBaseEpochMs;
    clockRtcTzOffset = clockTimezoneOffsetMinutes;
}

inline void restoreClockFromRtc() {
    if (!clockRtcValid || !clockRtcHasTime) {
        return;
    }

    clockModeEnabled = clockRtcEnabled;
    clockHasTime = true;
    clockUse24Hour = clockRtc24Hour;
    clockBaseSeconds = clockRtcSeconds % 86400UL;
    clockBaseMillis = millis();
    clockBaseEpochMs = clockRtcEpochMs;
    clockTimezoneOffsetMinutes = clockRtcTzOffset;
}

inline void restoreClockFromPreferences(Preferences& preferences) {
    esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    bool fromDeepSleep = (wakeCause == ESP_SLEEP_WAKEUP_EXT1 ||
                          wakeCause == ESP_SLEEP_WAKEUP_GPIO ||
                          wakeCause == ESP_SLEEP_WAKEUP_EXT0);

    if (fromDeepSleep && clockRtcValid && clockRtcHasTime) {
        restoreClockFromRtc();
        return;
    }

    clockRtcValid = false;

    clockTimezoneOffsetMinutes = (int16_t)preferences.getInt("clk_tz", 0);
    uint32_t clockSeconds = preferences.getUInt("clk_sec", 0);
    clockBaseEpochMs = preferences.getULong64("clk_epoch", 0ULL);
    clockModeEnabled = preferences.getBool("clk_on", false);
    clockUse24Hour = preferences.getBool("clk_24", true);
    clockHasTime = (clockSeconds < 86400UL);
    clockBaseSeconds = clockSeconds % 86400UL;
    clockBaseMillis = millis();
}

inline void setClockBaseSeconds(uint32_t secondsOfDay) {
    clockBaseSeconds = secondsOfDay % 86400UL;
    clockBaseMillis = millis();
    clockBaseEpochMs = 0;
    clockHasTime = true;
    clockLastDrawKey = -1;
    persistClockToRtc();
}

inline void setClockEnabled(bool enabled) {
    clockModeEnabled = enabled;
    if (enabled && !clockHasTime) {
        clockBaseMillis = millis();
        clockBaseSeconds = 0;
        clockHasTime = true;
    }
    clockLastDrawKey = -1;
    persistClockToRtc();
}

inline void setClockTimeOfDay(uint8_t hours, uint8_t minutes, uint8_t seconds = 0) {
    setClockBaseSeconds(((uint32_t)(hours % 24) * 3600UL) +
                        ((uint32_t)(minutes % 60) * 60UL) +
                        (uint32_t)(seconds % 60));
}

inline void setClockFromEpochMs(uint64_t epochMs, int16_t tzOffsetMinutes = 0) {
    clockTimezoneOffsetMinutes = tzOffsetMinutes;
    clockBaseEpochMs = epochMs;
    int64_t localMs = (int64_t)epochMs - ((int64_t)tzOffsetMinutes * 60000LL);
    if (localMs < 0) {
        localMs = 0;
    }
    clockBaseSeconds = (uint32_t)((uint64_t)localMs / 1000ULL) % 86400UL;
    clockBaseMillis = millis();
    clockHasTime = true;
    clockLastDrawKey = -1;
    persistClockToRtc();
}

inline void setClock24Hour(bool enabled) {
    clockUse24Hour = enabled;
    clockLastDrawKey = -1;
    persistClockToRtc();
}

inline bool isClock24Hour() {
    return clockUse24Hour;
}

inline bool isClockEnabled() {
    return clockModeEnabled;
}

inline uint32_t getClockSecondsOfDay() {
    if (!clockHasTime) {
        return 0;
    }
    uint32_t elapsedSeconds = (uint32_t)(millis() - clockBaseMillis) / 1000UL;
    return (clockBaseSeconds + elapsedSeconds) % 86400UL;
}

inline uint64_t getClockEpochMs() {
    if (clockBaseEpochMs == 0) {
        return 0;
    }
    return clockBaseEpochMs + (uint64_t)(millis() - clockBaseMillis);
}

inline String getClockStatusString() {
    uint32_t seconds = getClockSecondsOfDay();
    uint8_t hh = seconds / 3600UL;
    uint8_t mm = (seconds % 3600UL) / 60UL;
    uint8_t ss = seconds % 60UL;
    char buf[40];
    snprintf(buf, sizeof(buf), "clock:%s %02u:%02u:%02u tz=%d fmt=%s",
             clockModeEnabled ? "on" : "off", hh, mm, ss, clockTimezoneOffsetMinutes,
             clockUse24Hour ? "24" : "12");
    return String(buf);
}

inline void drawDefaultClockFace(Adafruit_GFX* display, const char* dateBuf, const char* timeBuf, bool isPM) {
    display->setTextWrap(false);

    display->setTextSize(1);
    display->setCursor(4, 4);
    display->print(dateBuf);

    const char* bleText = isBLEConnected() ? "BLE OK" : "BLE OFF";
    int16_t bleWidth = (int16_t)strlen(bleText) * 6;
    int16_t bleX = SCREEN_WIDTH - bleWidth - 4;
    if (bleX < 4) bleX = 4;
    display->setCursor(bleX, 4);
    display->print(bleText);

    display->setTextSize(4);
    int16_t timeWidth = (int16_t)strlen(timeBuf) * 24;
    int16_t timeX = (SCREEN_WIDTH - timeWidth) / 2;
    if (timeX < 0) timeX = 0;
    display->setCursor(timeX, 20);
    display->print(timeBuf);

    if (!clockUse24Hour) {
        const char* ampmText = isPM ? "PM" : "AM";
        display->setTextSize(1);
        int16_t ampmWidth = (int16_t)strlen(ampmText) * 6;
        int16_t ampmX = SCREEN_WIDTH - ampmWidth - 4;
        if (ampmX < 4) ampmX = 4;
        display->setCursor(ampmX, 54);
        display->print(ampmText);
    }
}

inline void drawClockScreen() {
    uint16_t WHITE_COLOR = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;
    uint32_t seconds = getClockSecondsOfDay();
    uint32_t minuteOfDay = seconds / 60UL;
    uint8_t hh = seconds / 3600UL;
    uint8_t mm = (seconds % 3600UL) / 60UL;
    uint8_t displayHour = hh;
    bool isPM = (hh >= 12);
    if (!clockUse24Hour) {
        displayHour = hh % 12;
        if (displayHour == 0) displayHour = 12;
    }

    bool colonOn = ((seconds % 2UL) == 0);
    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02u%c%02u", displayHour, colonOn ? ':' : ' ', mm);
    uint32_t drawKey = minuteOfDay |
                       ((seconds % 2UL) << 29) |
                       (clockUse24Hour ? 0x80000000UL : 0UL) |
                       (isBLEConnected() ? 0x40000000UL : 0UL);
    if (drawKey == clockLastDrawKey) {
        return;
    }
    clockLastDrawKey = drawKey;

    static const char* const WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    char dateBuf[16] = "--- --";
    if (clockBaseEpochMs != 0) {
        uint64_t epochMs = getClockEpochMs();
        time_t epochS = (time_t)(epochMs / 1000ULL) - ((time_t)clockTimezoneOffsetMinutes * 60);
        struct tm localTm;
        gmtime_r(&epochS, &localTm); // Avoid system timezone offsets, since we already offset manually
        int wday = localTm.tm_wday;
        int mday = localTm.tm_mday;
        if (wday < 0 || wday > 6) wday = 0;
        snprintf(dateBuf, sizeof(dateBuf), "%s %02d", WEEKDAYS[wday], mday);
    }

    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        drawDefaultClockFace(display_ssd1306, dateBuf, timeBuf, isPM);
        display_ssd1306->display();
        pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        drawDefaultClockFace(display_sh1106, dateBuf, timeBuf, isPM);
        display_sh1106->display();
        pushPartialUpdate(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}

#endif // CLOCK_MANAGER_H
