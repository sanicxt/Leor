#pragma once

#include "leor/ble_service.hpp"
#include "leor/clock_service.hpp"
#include "leor/command_router.hpp"
#include "leor/config.hpp"
#include "leor/display_backend.hpp"
#include "leor/gesture_service.hpp"
#include "leor/mochi_eyes_engine.hpp"
#include "leor/power_service.hpp"
#include "leor/preferences.hpp"
#include "leor/shuffle_service.hpp"

#include "esp_err.h"

#include <memory>

namespace leor {

class Application {
public:
  Application();

  esp_err_t start();
  void tick();

private:
  RuntimeConfig config_{};
  Preferences preferences_{};
  std::unique_ptr<DisplayBackend> display_;
  std::unique_ptr<MochiEyesEngine> eyes_;
  GestureService gesture_;
  ClockService clock_;
  ShuffleService shuffle_;
  PowerService power_;
  BleService ble_;
  std::unique_ptr<CommandRouter> commands_;
  bool was_clock_enabled_ = false;
};

} // namespace leor
