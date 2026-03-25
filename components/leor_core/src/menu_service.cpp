#include "leor/menu_service.hpp"

namespace leor {

void MenuService::on_short_press(uint32_t now_ms) {
  if (open_) {
    cursor_ = (cursor_ + 1) % 2;
    open_at_ms_ = now_ms;
  }
}

void MenuService::on_long_press(uint32_t now_ms) {
  if (!open_) {
    open_ = true;
    open_at_ms_ = now_ms;
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

  // Helper lambda for standard button-like selection boxes
  auto draw_panel = [&](int x, int y, int w, int h, const char* label, bool selected) {
    if (selected) {
      display.set_color(1);
      display.fill_rbox(x, y, w, h, 6);
      display.set_color(0); // invert text
    } else {
      display.set_color(1);
      // To mimic a frame with rounded corners, we do fill clear and then a pseudo-frame
      // Since U8G2 wrapper lacks true draw_rframe, we use primitive boxes or filled lines
      // The display_backend has fill_rbox, so we can do a hack:
      display.fill_rbox(x, y, w, h, 6);
      display.set_color(0);
      display.fill_rbox(x + 1, y + 1, w - 2, h - 2, 5);
      display.set_color(1); // text is standard white
    }
    
    int tw = display.text_width(label);
    display.draw_text(x + (w - tw) / 2, y + h / 2 + 4, label);
    display.set_color(1); // restore
  };

  // Split screen in two 52x40 panels
  draw_panel(10, 12, 50, 40, "SLEEP", cursor_ == 0);
  draw_panel(68, 12, 50, 40, currently_clock_mode ? "MOCHI" : "CLOCK", cursor_ == 1);
}

} // namespace leor
