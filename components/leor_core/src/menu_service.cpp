#include "leor/menu_service.hpp"
#include "leor/icons/icons.h"

namespace leor {

void MenuService::on_short_press(uint32_t now_ms) {
  if (open_) {
    cursor_ = (cursor_ + 1) % 2;
    last_activity_ms_ = now_ms;
  }
}

void MenuService::on_long_press(uint32_t now_ms) {
  if (!open_) {
    open_ = true;
    last_activity_ms_ = now_ms;
    cursor_ = 0; // Default to Power Off
  } else {
    if (cursor_ == 0) {
      pending_ = MenuAction::kPowerOff;
    } else {
      pending_ = MenuAction::kToggleMode;
    }
    open_ = false;
  }
}

MenuAction MenuService::consume_action() {
  MenuAction act = pending_;
  pending_ = MenuAction::kNone;
  return act;
}

bool MenuService::is_open() const { return open_; }

void MenuService::close() {
  open_ = false;
  pending_ = MenuAction::kNone;
}

void MenuService::draw(DisplayBackend &display, bool currently_clock_mode, uint32_t now_ms) {
  if (!open_) return;

  display.set_font_small();

  auto draw_panel = [&](int x, int y, int w, int h, const uint8_t* icon, const char* label, bool selected) {
    if (selected) {
      display.set_color(1);
      display.fill_rbox(x, y, w, h, 6);
      display.set_color(0);
    } else {
      display.set_color(1);
      display.fill_rbox(x, y, w, h, 6);
      display.set_color(0);
      display.fill_rbox(x + 1, y + 1, w - 2, h - 2, 5);
      display.set_color(1);
    }

    int icx = x + (w - 16) / 2;
    int icy = y + 3;
    display.draw_xbmp(icx, icy, 16, 16, icon);

    int tw = display.text_width(label);
    display.draw_text(x + (w - tw) / 2, icy + 16 + 3 + 10, label);
    display.set_color(1);
  };

  const uint8_t* right_icon;
  const char* right_label;
  if (currently_clock_mode) {
    right_icon = leor::icons::mochi;
    right_label = "MOCHI";
  } else {
    right_icon = leor::icons::clock;
    right_label = "CLOCK";
  }

  draw_panel(10, 12, 50, 44, leor::icons::sleep, "SLEEP", cursor_ == 0);
  draw_panel(68, 12, 50, 44, right_icon, right_label, cursor_ == 1);
}

} // namespace leor
