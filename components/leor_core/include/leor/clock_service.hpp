#pragma once

#include "leor/display_backend.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace leor {

class ClockService {
  public:
    void restore(bool enabled, bool use24, int16_t tz_offset, uint64_t epoch_ms, uint32_t manual_sec);
    void set_enabled(bool enabled);
    bool enabled() const { return enabled_; }
    void set_use_24_hour(bool enabled);
    bool use_24_hour() const { return use_24_hour_; }
    void set_time_of_day(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void set_from_epoch_ms(uint64_t epoch_ms, int16_t tz_offset_minutes);
    uint32_t seconds_of_day() const;
    uint64_t epoch_ms() const;
    int16_t tz_offset() const { return tz_offset_minutes_; }
    std::string status_string(bool ble_connected) const;
    void draw(DisplayBackend& display, bool ble_connected);

  private:
    uint8_t to_display_hour(uint8_t hh24, bool* is_pm) const;
    uint32_t make_draw_key(uint32_t minute_of_day, bool colon_on, bool ble_connected) const;
    void format_date(char* out, std::size_t out_size) const;
    void save_retained_state();

    bool enabled_ = false;
    bool has_time_ = false;
    bool has_epoch_ = false;
    bool use_24_hour_ = true;
    int16_t tz_offset_minutes_ = 0;
    uint32_t last_draw_key_ = UINT32_MAX;
};

}  // namespace leor
