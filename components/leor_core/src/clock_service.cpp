#include "leor/clock_service.hpp"

#include <cstdio>
#include <ctime>

#include "esp_attr.h"
#include "esp_rtc_time.h"

namespace leor {

namespace {

struct RetainedClockState {
    uint32_t magic = 0;
    bool enabled = false;
    bool has_time = false;
    bool use_24_hour = true;
    uint64_t base_rtc_us = 0;
    uint32_t base_seconds = 0;
    uint64_t base_epoch_ms = 0;
    int16_t tz_offset_minutes = 0;
};

constexpr uint32_t kClockStateMagic = 0x4C434B31U;

RTC_NOINIT_ATTR RetainedClockState s_clock_state;

constexpr int kTopRowBaselineY = 10;
constexpr int kDateX = 3;
constexpr int kClockBaselineY = 54;
constexpr int kClockGapPx = 7;
constexpr int kColonDotSizePx = 4;
constexpr int kColonTopDotY = 29;
constexpr int kColonBottomDotY = 40;
constexpr int kAmPmY = 62;

constexpr uint32_t kDrawKeyColonBit = 0x80000000U;
constexpr uint32_t kDrawKeyBleBit = 0x40000000U;

void draw_colon_dots(DisplayBackend& display, int center_x, bool on) {
    if (!on) {
        return;
    }
    const int colon_x = center_x - (kColonDotSizePx / 2);
    display.fill_box(colon_x, kColonTopDotY, kColonDotSizePx, kColonDotSizePx);
    display.fill_box(colon_x, kColonBottomDotY, kColonDotSizePx, kColonDotSizePx);
}

void draw_ble_icon(DisplayBackend& display, int x, int y, bool connected) {
    display.draw_frame(x, y, 10, 10);
    if (connected) {
        display.draw_line(x + 5, y + 1, x + 5, y + 8);
        display.draw_line(x + 3, y + 3, x + 7, y + 3);
        display.draw_line(x + 3, y + 6, x + 7, y + 6);
    } else {
        display.draw_line(x + 2, y + 2, x + 7, y + 7);
        display.draw_line(x + 7, y + 2, x + 2, y + 7);
    }
}

}  // namespace

uint8_t ClockService::to_display_hour(uint8_t hh24, bool* is_pm) const {
    if (is_pm != nullptr) {
        *is_pm = hh24 >= 12U;
    }
    if (use_24_hour_) {
        return hh24;
    }
    uint8_t hh12 = hh24 % 12U;
    if (hh12 == 0U) {
        hh12 = 12U;
    }
    return hh12;
}

uint32_t ClockService::make_draw_key(uint32_t minute_of_day, bool colon_on, bool ble_connected) const {
    uint32_t key = minute_of_day;
    if (colon_on) {
        key |= kDrawKeyColonBit;
    }
    if (ble_connected) {
        key |= kDrawKeyBleBit;
    }
    return key;
}

void ClockService::format_date(char* out, std::size_t out_size, uint32_t now_ms) const {
    if (out == nullptr || out_size == 0U) {
        return;
    }
    if (base_epoch_ms_ == 0) {
        std::snprintf(out, out_size, "--- --");
        return;
    }
    static const char* const weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const time_t epoch_s = static_cast<time_t>(epoch_ms(now_ms) / 1000ULL) - static_cast<time_t>(tz_offset_minutes_) * 60;
    struct tm tm_value;
    gmtime_r(&epoch_s, &tm_value);
    std::snprintf(out, out_size, "%s %02d", weekdays[tm_value.tm_wday], tm_value.tm_mday);
}

void ClockService::restore(bool enabled, bool use24, uint32_t seconds, int16_t tz_offset, uint64_t epoch_ms_value, uint32_t /*now_ms*/) {
    if (s_clock_state.magic == kClockStateMagic && s_clock_state.has_time) {
        enabled_ = s_clock_state.enabled;
        use_24_hour_ = s_clock_state.use_24_hour;
        has_time_ = s_clock_state.has_time;
        base_rtc_us_ = s_clock_state.base_rtc_us;
        base_seconds_ = s_clock_state.base_seconds;
        base_epoch_ms_ = s_clock_state.base_epoch_ms;
        tz_offset_minutes_ = s_clock_state.tz_offset_minutes;
    } else {
        enabled_ = enabled;
        use_24_hour_ = use24;
        base_seconds_ = seconds % 86400U;
        tz_offset_minutes_ = tz_offset;
        base_epoch_ms_ = epoch_ms_value;
        has_time_ = true;
        base_rtc_us_ = esp_rtc_get_time_us();
    }
    s_clock_state = {kClockStateMagic, enabled_, has_time_, use_24_hour_, base_rtc_us_, base_seconds_, base_epoch_ms_, tz_offset_minutes_};
    last_draw_key_ = UINT32_MAX;
}

void ClockService::set_enabled(bool enabled) {
    enabled_ = enabled;
    s_clock_state.enabled = enabled_;
    s_clock_state.magic = kClockStateMagic;
    last_draw_key_ = UINT32_MAX;
}

void ClockService::set_use_24_hour(bool enabled) {
    use_24_hour_ = enabled;
    s_clock_state.use_24_hour = use_24_hour_;
    s_clock_state.magic = kClockStateMagic;
    last_draw_key_ = UINT32_MAX;
}

void ClockService::set_time_of_day(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t /*now_ms*/) {
    base_seconds_ = static_cast<uint32_t>(hours % 24U) * 3600U + static_cast<uint32_t>(minutes % 60U) * 60U + static_cast<uint32_t>(seconds % 60U);
    base_rtc_us_ = esp_rtc_get_time_us();
    base_epoch_ms_ = 0;
    has_time_ = true;
    s_clock_state = {kClockStateMagic, enabled_, has_time_, use_24_hour_, base_rtc_us_, base_seconds_, base_epoch_ms_, tz_offset_minutes_};
    last_draw_key_ = UINT32_MAX;
}

void ClockService::set_from_epoch_ms(uint64_t epoch_ms_value, int16_t tz_offset_minutes, uint32_t /*now_ms*/) {
    base_epoch_ms_ = epoch_ms_value;
    tz_offset_minutes_ = tz_offset_minutes;
    has_time_ = true;
    base_rtc_us_ = esp_rtc_get_time_us();
    const int64_t local_ms = static_cast<int64_t>(epoch_ms_value) - static_cast<int64_t>(tz_offset_minutes) * 60000LL;
    base_seconds_ = static_cast<uint32_t>((local_ms < 0 ? 0 : local_ms) / 1000ULL) % 86400U;
    s_clock_state = {kClockStateMagic, enabled_, has_time_, use_24_hour_, base_rtc_us_, base_seconds_, base_epoch_ms_, tz_offset_minutes_};
    last_draw_key_ = UINT32_MAX;
}

uint32_t ClockService::seconds_of_day(uint32_t now_ms) const {
    if (!has_time_) {
        return 0;
    }
    const uint64_t elapsed_us = esp_rtc_get_time_us() - base_rtc_us_;
    return (base_seconds_ + static_cast<uint32_t>(elapsed_us / 1000000ULL)) % 86400U;
}

uint64_t ClockService::epoch_ms(uint32_t now_ms) const {
    if (base_epoch_ms_ == 0) {
        return 0;
    }
    const uint64_t elapsed_us = esp_rtc_get_time_us() - base_rtc_us_;
    return base_epoch_ms_ + (elapsed_us / 1000ULL);
}

std::string ClockService::status_string(uint32_t now_ms, bool ble_connected) const {
    const uint32_t seconds = seconds_of_day(now_ms);
    const uint8_t hh = seconds / 3600U;
    const uint8_t mm = (seconds % 3600U) / 60U;
    const uint8_t ss = seconds % 60U;
    char buf[96];
    std::snprintf(buf, sizeof(buf), "clock:%s %02u:%02u:%02u tz=%d fmt=%s ble=%s",
                  enabled_ ? "on" : "off", hh, mm, ss, tz_offset_minutes_, use_24_hour_ ? "24" : "12", ble_connected ? "1" : "0");
    return buf;
}

void ClockService::draw(DisplayBackend& display, uint32_t now_ms, bool ble_connected) {
    const uint32_t seconds = seconds_of_day(now_ms);
    const uint32_t minute_of_day = seconds / 60U;
    const bool colon_on = (seconds % 2U) == 0U;
    const uint8_t hh24 = seconds / 3600U;
    const uint8_t mm = (seconds % 3600U) / 60U;
    bool is_pm = false;
    const uint8_t display_hour = to_display_hour(hh24, &is_pm);
    const uint32_t draw_key = make_draw_key(minute_of_day, colon_on, ble_connected);
    if (draw_key == last_draw_key_) {
        return;
    }
    last_draw_key_ = draw_key;

    char hour_buf[4];
    char minute_buf[4];
    std::snprintf(hour_buf, sizeof(hour_buf), "%02u", display_hour);
    std::snprintf(minute_buf, sizeof(minute_buf), "%02u", mm);

    char date_buf[16];
    format_date(date_buf, sizeof(date_buf), now_ms);

    display.clear();
    display.set_font_small();
    display.draw_text(kDateX, kTopRowBaselineY, date_buf);
    const int ble_text_x = display.width() - 22;
    const int ble_icon_x = ble_text_x - 12;
    draw_ble_icon(display, ble_icon_x, 1, ble_connected);
    display.draw_text(ble_text_x, kTopRowBaselineY, ble_connected ? "ON" : "OFF");

    display.set_font_large();
    const int hour_w = display.text_width(hour_buf);
    const int center_x = display.width() / 2;
    const int hour_x = center_x - kClockGapPx - hour_w;
    const int minute_x = center_x + kClockGapPx;
    display.draw_text(hour_x, kClockBaselineY, hour_buf);
    draw_colon_dots(display, center_x, colon_on);
    display.draw_text(minute_x, kClockBaselineY, minute_buf);

    if (!use_24_hour_) {
        display.set_font_small();
        display.draw_text(display.width() - 18, kAmPmY, is_pm ? "PM" : "AM");
    }
    display.send_buffer();
}

}  // namespace leor
