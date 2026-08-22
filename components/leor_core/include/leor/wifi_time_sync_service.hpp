#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace leor {

class Preferences;

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

 private:
  Preferences* prefs_ = nullptr;
  std::string ssid_;
  std::string pass_;
  std::string last_status_;
  std::function<void(uint64_t)> on_time_;
  bool syncing_ = false;
};

}  // namespace leor
