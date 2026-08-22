#include "leor/wifi_time_sync_service.hpp"

#include <cstring>
#include <ctime>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
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
};

struct ScanContext {
  WifiTimeSyncService* self;
  std::function<void(const std::vector<WifiApInfo>&)> on_done;
};

volatile bool s_sta_started = false;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START:
        s_sta_started = true;
        ESP_LOGI(kTag, "event: STA_START");
        break;
      case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(kTag, "event: STA_CONNECTED");
        break;
      case WIFI_EVENT_STA_DISCONNECTED: {
        const auto* d = static_cast<wifi_event_sta_disconnected_t*>(event_data);
        ESP_LOGW(kTag, "event: STA_DISCONNECTED reason=%d", d ? d->reason : -1);
        break;
      }
      default:
        ESP_LOGI(kTag, "event: WIFI_EVENT id=%ld", static_cast<long>(event_id));
        break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    const auto* e = static_cast<ip_event_got_ip_t*>(event_data);
    ESP_LOGI(kTag, "event: GOT_IP ip=" IPSTR, IP2STR(&e->ip_info.ip));
  }
}

void sync_task(void* arg) {
  auto* ctx = static_cast<SyncContext*>(arg);
  const bool ok = ctx->self->sync_blocking(10000);
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

  // Hold a no-light-sleep PM lock for the whole wifi session: with light sleep
  // enabled (CONFIG_PM_ENABLE) the wifi task stalls mid-start while the station
  // is in the disconnected state, so WIFI_EVENT_STA_START never fires.
  esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "wifi_sync", &pm_lock_);
  if (pm_lock_) esp_pm_lock_acquire(pm_lock_);

  ESP_LOGI(kTag, "wifi bring up ssid=%s pass=%s", ssid_.c_str(), pass_.c_str());
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  const esp_err_t init_rc = esp_wifi_init(&cfg);
  ESP_LOGI(kTag, "esp_wifi_init rc=0x%x", init_rc);
  if (init_rc != ESP_OK) {
    last_status_ = "wifi init failed";
    esp_netif_deinit();
    esp_event_loop_delete_default();
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
  const esp_err_t setcfg_rc = esp_wifi_set_config(WIFI_IF_STA, &wc);
  ESP_LOGI(kTag, "esp_wifi_set_config rc=0x%x ssid=%s pass=%s", setcfg_rc, ssid_.c_str(), pass_.c_str());
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr);
  const esp_err_t start_rc = esp_wifi_start();
  ESP_LOGI(kTag, "esp_wifi_start rc=0x%x", start_rc);

  // Wait for the driver to post STA_START before touching the radio.
  const uint32_t start_deadline = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) + 3000;
  while (!s_sta_started && static_cast<uint32_t>(esp_timer_get_time() / 1000ULL) < start_deadline) {
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  ESP_LOGI(kTag, "sta_started=%d", s_sta_started);
  return true;
}

void WifiTimeSyncService::bring_down() {
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_event_loop_delete_default();
  esp_netif_deinit();
  if (pm_lock_) {
    esp_pm_lock_release(pm_lock_);
    esp_pm_lock_delete(pm_lock_);
    pm_lock_ = nullptr;
  }
  syncing_ = false;
  ESP_LOGI(kTag, "wifi brought down");
}

bool WifiTimeSyncService::sync_locked(uint32_t timeout_ms) {
  // Scan once and log every visible AP so a missing network is diagnosable.
  wifi_scan_config_t scan_cfg = {};
  scan_cfg.show_hidden = true;
  scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  scan_cfg.scan_time.active.min = 120;
  scan_cfg.scan_time.active.max = 300;
  const esp_err_t scan_rc = esp_wifi_scan_start(&scan_cfg, true);
  ESP_LOGI(kTag, "esp_wifi_scan_start rc=0x%x", scan_rc);
  if (scan_rc == ESP_OK) {
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(kTag, "scan found %u APs", ap_count);
    if (ap_count > 0) {
      auto* records = static_cast<wifi_ap_record_t*>(calloc(ap_count, sizeof(wifi_ap_record_t)));
      if (records) {
        esp_wifi_scan_get_ap_records(&ap_count, records);
        for (uint16_t i = 0; i < ap_count; ++i) {
          ESP_LOGI(kTag, "ap[%u] ssid=%s rssi=%d chan=%u auth=%d",
                   i, reinterpret_cast<const char*>(records[i].ssid), records[i].rssi,
                   records[i].primary, records[i].authmode);
        }
        free(records);
      }
    }
  }

  const esp_err_t connect_rc = esp_wifi_connect();
  ESP_LOGI(kTag, "esp_wifi_connect rc=0x%x", connect_rc);

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
  ESP_LOGI(kTag, "ip poll done got_ip=%d ip=" IPSTR, got_ip, IP2STR(&ip.ip));
  if (!got_ip) {
    last_status_ = "wifi connect timeout";
    return false;
  }

  esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  const esp_err_t sntp_rc = esp_netif_sntp_init(&sntp_config);
  ESP_LOGI(kTag, "esp_netif_sntp_init rc=0x%x", sntp_rc);
  if (sntp_rc != ESP_OK) {
    last_status_ = "sntp init failed";
    return false;
  }
  const esp_err_t wait_rc = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(timeout_ms));
  ESP_LOGI(kTag, "esp_netif_sntp_sync_wait rc=0x%x", wait_rc);
  bool synced = (wait_rc == ESP_OK);
  esp_netif_sntp_deinit();

  if (synced) {
    time_t now = 0;
    time(&now);
    ESP_LOGI(kTag, "sntp time=%lld", static_cast<long long>(now));
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

  ESP_LOGI(kTag, "wifi sync done ok=%d", synced);
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
  scan_cfg.scan_time.active.min = 120;
  scan_cfg.scan_time.active.max = 300;
  const esp_err_t scan_rc = esp_wifi_scan_start(&scan_cfg, true);
  ESP_LOGI(kTag, "esp_wifi_scan_start rc=0x%x", scan_rc);
  if (scan_rc == ESP_OK) {
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(kTag, "scan found %u APs", ap_count);
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

void WifiTimeSyncService::sync_async(std::function<void(const std::string&)> on_done) {
  auto* ctx = new SyncContext{this, std::move(on_done)};
  xTaskCreate(sync_task, "wifi_sync", kSyncTaskStack, ctx, tskIDLE_PRIORITY + 1, nullptr);
}

}  // namespace leor
