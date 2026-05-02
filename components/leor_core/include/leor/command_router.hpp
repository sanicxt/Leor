#pragma once

#include "leor/ble_service.hpp"
#include "leor/clock_service.hpp"
#include "leor/gesture_service.hpp"
#include "leor/mochi_eyes_engine.hpp"
#include "leor/notification_overlay.hpp"
#include "leor/power_service.hpp"
#include "leor/preferences.hpp"
#include "leor/shuffle_service.hpp"
#include "leor/display_backend.hpp"
#include "leor/config.hpp"

#include <string>

namespace leor {

class CommandRouter {
  public:
    CommandRouter(Preferences& preferences,
                  DisplayConfig& display_config,
                  DisplayBackend& display,
                  MochiEyesEngine& eyes,
                  GestureService& gestures,
                  ShuffleService& shuffle,
                  ClockService& clock,
                  PowerService& power,
                  BleService& ble);

    std::string handle(std::string cmd, uint32_t now_ms, bool is_manual = true);
    void set_notif_overlay(NotificationOverlay* notif);
    uint32_t notif_duration_ms() const { return notif_duration_ms_; }

  private:
    std::string handle_settings(const std::string& params, uint32_t now_ms);
    std::string handle_shuffle(const std::string& params);
    std::string handle_display(const std::string& params);
    std::string handle_clock(const std::string& params, uint32_t now_ms);
    std::string sync_json(uint32_t now_ms) const;
    void reset_effects();

    Preferences& preferences_;
    DisplayConfig& display_config_;
    DisplayBackend& display_;
    MochiEyesEngine& eyes_;
    GestureService& gestures_;
    ShuffleService& shuffle_;
    ClockService& clock_;
    PowerService& power_;
    BleService& ble_;
    bool mpu_verbose_ = false;
    bool reacting_ = false;
    NotificationOverlay* notif_overlay_ = nullptr;
    uint32_t notif_duration_ms_ = 5000;
};

}  // namespace leor
