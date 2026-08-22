#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "esp_pm.h"

namespace leor {

class Preferences;

struct WifiApInfo {
  std::string ssid;
  int rssi;
  int authmode;
};

class WifiTimeSyncService {
 public:
  void init(Preferences& prefs);
  bool configured() const;
  bool sync_blocking(uint32_t timeout_ms);
  void sync_async(std::function<void(const std::string&)> on_done);
  void set_ssid(const std::string& ssid);
  void set_pass(const std::string& pass);
  std::string ssid() const;
  std::string pass() const;
  std::string last_status() const;
  void set_time_callback(std::function<void(uint64_t epoch_ms)> cb);
  bool scan_aps(uint32_t timeout_ms, std::vector<WifiApInfo>& out);
  void scan_async(std::function<void(const std::vector<WifiApInfo>&)> on_done);

 private:
  bool bring_up();
  void bring_down();
  bool sync_locked(uint32_t timeout_ms);

  Preferences* prefs_ = nullptr;
  std::string ssid_;
  std::string pass_;
  std::string last_status_;
  std::function<void(uint64_t)> on_time_;
  bool syncing_ = false;
  esp_pm_lock_handle_t pm_lock_ = nullptr;
};

}  // namespace leor
