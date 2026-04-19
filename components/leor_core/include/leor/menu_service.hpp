#pragma once

#include "leor/display_backend.hpp"
#include <cstdint>

namespace leor {

enum class MenuAction { kNone, kPowerOff, kToggleMode };

class MenuService {
public:
  void on_short_press(uint32_t now_ms);
  void on_long_press(uint32_t now_ms);

  MenuAction consume_action();
  bool is_open() const;
  void close();
  uint32_t last_activity_ms() const { return last_activity_ms_; }
  static constexpr uint32_t kTimeoutMs = 5000;

  void draw(DisplayBackend &display, bool currently_clock_mode, uint32_t now_ms);

private:
  bool open_ = false;
  int cursor_ = 0; // 0 = Power, 1 = Toggle
  MenuAction pending_ = MenuAction::kNone;
  uint32_t last_activity_ms_ = 0;
};

} // namespace leor
