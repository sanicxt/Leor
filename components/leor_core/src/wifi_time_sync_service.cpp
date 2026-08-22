#include "leor/wifi_time_sync_service.hpp"

#include <algorithm>
#include <cstring>
#include <ctime>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_coexist.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "leor/preferences.hpp"

namespace leor {

namespace {
constexpr char kTag[] = "leor_wifi";
constexpr uint32_t kSyncTaskStack = 4096;
constexpr uint16_t kMaxScanAps = 20;

struct SyncContext {
  WifiTimeSyncService* self;
  std::function<void(const std::string&)> on_done;
  std::function<void()> pre_sync;
};

struct ScanContext {
  WifiTimeSyncService* self;
  std::function<void(const std::vector<WifiApInfo>&)> on_done;
};

volatile bool s_sta_started = false;
volatile bool s_got_ip = false;
volatile int s_disconnect_reason = 0;
static bool s_wifi_infra_inited = false;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START:
        s_sta_started = true;
        break;
      case WIFI_EVENT_STA_DISCONNECTED: {
        const auto* d = static_cast<wifi_event_sta_disconnected_t*>(event_data);
        s_disconnect_reason = d ? d->reason : -1;
        break;
      }
      default:
        break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    s_got_ip = true;
  }
}

void sync_task(void* arg) {
  auto* ctx = static_cast<SyncContext*>(arg);
  if (ctx->pre_sync) ctx->pre_sync();
  const bool ok = ctx->self->sync_blocking(20000);
  if (ctx->on_done) {
    ctx->on_done(ok ? "wifi sync ok" : "wifi sync failed");
  }
  delete ctx;
  vTaskDelete(nullptr);
}

void scan_task(void* arg) {
  auto* ctx = static_cast<ScanContext*>(arg);
  std::vector<WifiApInfo> aps;
  ctx->self->scan_aps(5000, aps);
  if (ctx->on_done) {
    ctx->on_done(aps);
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
std::string WifiTimeSyncService::pass() const { return pass_; }
std::string WifiTimeSyncService::last_status() const { return last_status_; }

void WifiTimeSyncService::set_time_callback(std::function<void(uint64_t)> cb) {
  on_time_ = std::move(cb);
}

bool WifiTimeSyncService::bring_up() {
  s_sta_started = false;
  s_got_ip = false;
  s_disconnect_reason = 0;

  // Light sleep (CONFIG_PM_ENABLE) stalls the wifi task mid-start while the
  // station is in the disconnected state, so STA_START never fires.
  esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "wifi_sync", &pm_lock_);
  if (pm_lock_) esp_pm_lock_acquire(pm_lock_);

  if (!s_wifi_infra_inited) {
    esp_netif_init();
    esp_event_loop_create_default();
    s_wifi_infra_inited = true;
  }
  netif_ = esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  const esp_err_t init_rc = esp_wifi_init(&cfg);
  if (init_rc != ESP_OK) {
    last_status_ = "wifi init failed";
    if (netif_) {
      esp_netif_destroy_default_wifi(netif_);
      netif_ = nullptr;
    }
    if (pm_lock_) {
      esp_pm_lock_release(pm_lock_);
      esp_pm_lock_delete(pm_lock_);
      pm_lock_ = nullptr;
    }
    return false;
  }

  esp_wifi_set_mode(WIFI_MODE_STA);
  wifi_config_t wc = {};
  std::strncpy(reinterpret_cast<char*>(wc.sta.ssid), ssid_.c_str(), sizeof(wc.sta.ssid) - 1);
  std::strncpy(reinterpret_cast<char*>(wc.sta.password), pass_.c_str(), sizeof(wc.sta.password) - 1);
  wc.sta.pmf_cfg.capable = true;
  esp_wifi_set_config(WIFI_IF_STA, &wc);
  if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr) != ESP_OK) {
    ESP_LOGW(kTag, "failed to register WIFI_EVENT handler");
  }
  if (esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr) != ESP_OK) {
    ESP_LOGW(kTag, "failed to register IP_EVENT handler");
  }
  if (esp_wifi_start() != ESP_OK) {
    last_status_ = "wifi start failed";
    esp_wifi_deinit();
    if (netif_) {
      esp_netif_destroy_default_wifi(netif_);
      netif_ = nullptr;
    }
    if (pm_lock_) {
      esp_pm_lock_release(pm_lock_);
      esp_pm_lock_delete(pm_lock_);
      pm_lock_ = nullptr;
    }
    return false;
  }

  // esp_coex_preference_set is deprecated in IDF v6.0.2, but the replacement
  // esp_coex_status_bit_set/clear only expose BLE/BT status bits (no WIFI
  // equivalent), so there is no non-deprecated way to prefer wifi here.
  esp_coex_preference_set(ESP_COEX_PREFER_WIFI);

  const uint32_t start_deadline = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) + 3000;
  while (!s_sta_started && static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) < start_deadline) {
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  return true;
}

void WifiTimeSyncService::bring_down() {
  esp_wifi_stop();
  esp_wifi_deinit();
  if (netif_) {
    esp_netif_destroy_default_wifi(netif_);
    netif_ = nullptr;
  }
  if (pm_lock_) {
    esp_pm_lock_release(pm_lock_);
    esp_pm_lock_delete(pm_lock_);
    pm_lock_ = nullptr;
  }
  syncing_ = false;
}

bool WifiTimeSyncService::sync_locked(uint32_t timeout_ms) {
  const uint32_t deadline = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) + timeout_ms;
  bool got_ip = false;

  for (int attempt = 0; attempt < 5 && !got_ip; ++attempt) {
    s_disconnect_reason = 0;
    s_got_ip = false;
    const esp_err_t connect_rc = esp_wifi_connect();
    if (connect_rc == ESP_OK) {
      esp_wifi_set_max_tx_power(60);
    }
    if (connect_rc != ESP_OK) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }
    const uint32_t attempt_deadline = std::min(
        static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) + 6000, deadline);
    while (static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) < attempt_deadline) {
      if (s_got_ip) {
        got_ip = true;
        break;
      }
      if (s_disconnect_reason != 0) break;
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (got_ip) break;
    if (static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) >= deadline) break;
    esp_wifi_set_max_tx_power(80);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  if (!got_ip) {
    last_status_ = "wifi connect timeout";
    return false;
  }

  const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
  const uint32_t remaining = (deadline > now_ms) ? (deadline - now_ms) : 1000;

  esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.google.com");
  const esp_err_t sntp_rc = esp_netif_sntp_init(&sntp_config);
  if (sntp_rc != ESP_OK) {
    last_status_ = "sntp init failed";
    return false;
  }
  const esp_err_t wait_rc = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(remaining));
  bool synced = (wait_rc == ESP_OK);
  esp_netif_sntp_deinit();

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

  return synced;
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
  if (!bring_up()) {
    syncing_ = false;
    return false;
  }
  const bool ok = sync_locked(timeout_ms);
  bring_down();
  return ok;
}

bool WifiTimeSyncService::scan_aps(uint32_t timeout_ms, std::vector<WifiApInfo>& out) {
  out.clear();
  if (syncing_) {
    last_status_ = "already syncing";
    return false;
  }
  syncing_ = true;
  if (!bring_up()) {
    syncing_ = false;
    return false;
  }

  wifi_scan_config_t scan_cfg = {};
  scan_cfg.show_hidden = true;
  scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  const esp_err_t scan_rc = esp_wifi_scan_start(&scan_cfg, true);
  if (scan_rc == ESP_OK) {
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count > kMaxScanAps) ap_count = kMaxScanAps;
    if (ap_count > 0) {
      auto* records = static_cast<wifi_ap_record_t*>(calloc(ap_count, sizeof(wifi_ap_record_t)));
      if (records) {
        esp_wifi_scan_get_ap_records(&ap_count, records);
        for (uint16_t i = 0; i < ap_count; ++i) {
          WifiApInfo info;
          info.ssid = reinterpret_cast<const char*>(records[i].ssid);
          info.rssi = records[i].rssi;
          info.authmode = records[i].authmode;
          out.push_back(info);
        }
        free(records);
      }
    }
  }
  last_status_ = scan_rc == ESP_OK ? "scan done" : "scan failed";
  bring_down();
  return scan_rc == ESP_OK;
}

void WifiTimeSyncService::scan_async(std::function<void(const std::vector<WifiApInfo>&)> on_done) {
  auto* ctx = new ScanContext{this, std::move(on_done)};
  xTaskCreate(scan_task, "wifi_scan", kSyncTaskStack, ctx, tskIDLE_PRIORITY + 1, nullptr);
}

void WifiTimeSyncService::sync_async(std::function<void(const std::string&)> on_done,
                                     std::function<void()> pre_sync) {
  auto* ctx = new SyncContext{this, std::move(on_done), std::move(pre_sync)};
  xTaskCreate(sync_task, "wifi_sync", kSyncTaskStack, ctx, tskIDLE_PRIORITY + 1, nullptr);
}

}  // namespace leor