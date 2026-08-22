#include "leor/wifi_time_sync_service.hpp"

#include <cstring>
#include <ctime>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "leor/preferences.hpp"

namespace leor {

namespace {
constexpr char kTag[] = "leor_wifi";
constexpr uint32_t kSyncTaskStack = 4096;

struct SyncContext {
  WifiTimeSyncService* self;
  std::function<void(const std::string&)> on_done;
};

void sync_task(void* arg) {
  auto* ctx = static_cast<SyncContext*>(arg);
  const bool ok = ctx->self->sync_blocking(10000);
  if (ctx->on_done) {
    ctx->on_done(ok ? "wifi sync ok" : "wifi sync failed");
  }
  delete ctx;
  vTaskDelete(nullptr);
}
}  // namespace

void WifiTimeSyncService::init(Preferences& prefs) {
  prefs_ = &prefs;
  ssid_ = prefs.getString("wifi_ssid", "");
  pass_ = prefs.getString("wifi_pass", "");
}

bool WifiTimeSyncService::configured() const {
  return !ssid_.empty();
}

void WifiTimeSyncService::set_ssid(const std::string& ssid) {
  ssid_ = ssid;
  if (prefs_) prefs_->putString("wifi_ssid", ssid.c_str());
}

void WifiTimeSyncService::set_pass(const std::string& pass) {
  pass_ = pass;
  if (prefs_) prefs_->putString("wifi_pass", pass.c_str());
}

std::string WifiTimeSyncService::ssid() const { return ssid_; }
std::string WifiTimeSyncService::last_status() const { return last_status_; }

void WifiTimeSyncService::set_time_callback(std::function<void(uint64_t)> cb) {
  on_time_ = std::move(cb);
}

bool WifiTimeSyncService::sync_blocking(uint32_t timeout_ms) {
  if (!configured()) {
    last_status_ = "no wifi configured";
    return false;
  }
  if (syncing_) {
    last_status_ = "already syncing";
    return false;
  }
  syncing_ = true;

  ESP_LOGI(kTag, "wifi sync start ssid=%s", ssid_.c_str());
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  if (esp_wifi_init(&cfg) != ESP_OK) {
    last_status_ = "wifi init failed";
    esp_netif_deinit();
    syncing_ = false;
    return false;
  }

  esp_wifi_set_mode(WIFI_MODE_STA);
  wifi_config_t wc = {};
  std::strncpy(reinterpret_cast<char*>(wc.sta.ssid), ssid_.c_str(), sizeof(wc.sta.ssid) - 1);
  std::strncpy(reinterpret_cast<char*>(wc.sta.password), pass_.c_str(), sizeof(wc.sta.password) - 1);
  esp_wifi_set_config(WIFI_IF_STA, &wc);
  esp_wifi_start();

  esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  esp_netif_ip_info_t ip{};
  const uint32_t deadline = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) + timeout_ms;
  bool got_ip = false;
  while (static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) < deadline) {
    esp_netif_get_ip_info(netif, &ip);
    if (ip.ip.addr != 0) {
      got_ip = true;
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (!got_ip) {
    last_status_ = "wifi connect timeout";
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_deinit();
    syncing_ = false;
    return false;
  }

  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
  bool synced = false;
  while (static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) < deadline) {
    if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      synced = true;
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  esp_sntp_stop();

  if (synced) {
    time_t now = 0;
    time(&now);
    if (now > 1000000000) {
      if (on_time_) on_time_(static_cast<uint64_t>(now) * 1000ULL);
      last_status_ = "synced";
    } else {
      synced = false;
      last_status_ = "sntp time invalid";
    }
  } else {
    last_status_ = "sntp timeout";
  }

  esp_wifi_stop();
  esp_wifi_deinit();
  esp_event_loop_delete_default();
  esp_netif_deinit();
  syncing_ = false;
  ESP_LOGI(kTag, "wifi sync done ok=%d", synced);
  return synced;
}

void WifiTimeSyncService::sync_async(std::function<void(const std::string&)> on_done) {
  auto* ctx = new SyncContext{this, std::move(on_done)};
  xTaskCreate(sync_task, "wifi_sync", kSyncTaskStack, ctx, tskIDLE_PRIORITY + 1, nullptr);
}

}  // namespace leor
