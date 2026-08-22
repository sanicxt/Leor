# WiFi Time-Sync Feature Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a one-shot WiFi + SNTP time sync that runs as a boot stage (like gyro calibration) and can also be triggered manually from the web dashboard, then disconnects WiFi.

**Architecture:** A new `WifiTimeSyncService` follows the existing service pattern (hpp in `include/leor/`, cpp in `src/`, owned by `Application`, NVS via `Preferences`). It connects to a web-configured SSID/password (NVS keys `wifi_ssid`/`wifi_pass`), syncs time from `pool.ntp.org` via lwIP SNTP, hands the epoch to `ClockService::set_from_epoch_ms`, then stops and deinitializes WiFi. Boot gains a `kWifi` stage (only when credentials are configured) with a 10s timeout. The web dashboard gains a WiFi settings panel (SSID/password inputs, Sync-now button, status).

**Tech Stack:** ESP-IDF v6.0.2 (esp_wifi, esp_netif, lwIP SNTP), C++17, FreeRTOS, Svelte 5 + Tailwind v4 web dashboard.

**Spec:** none (bounded feature; design approved in chat on 2026-08-21: web-configured credentials, boot stage + manual trigger, pool.ntp.org).

## Global Constraints

- ESP-IDF v6.0.2, C++17, target esp32c3, 4MB flash.
- Firmware namespace `leor`, `#pragma once`, headers under `components/leor_core/include/leor/`.
- NVS namespace is `"leor"` (via `Preferences` wrapper: `getString`/`putString` exist).
- Display is 128x64; fonts: small = 6x10 (ascent 8), medium = 8x13 (ascent 10), large = logisoso32. `draw_text(x,y)` y is the BASELINE.
- Commit messages: SEMANTIC (`feat:`/`fix:`/`chore:`). NEVER add a Sisyphus co-author.
- Comments: only non-obvious bug-fix rationale. No self-explanatory comments.
- Firmware build: `export IDF_PYTHON_ENV_PATH=/home/sanic/.espressif/tools/python/v6.0.2/venv IDF_PYTHON_CHECK_CONSTRAINTS=0 && . ~/.espressif/v6.0.2/esp-idf/export.sh >/dev/null 2>&1 && idf.py build` from `/home/sanic/Leor` — must exit 0.
- Web build: `npm run build` in `web/` — must pass. `npm run check` has 8 pre-existing errors + 9 warnings in unrelated files; do not add new ones.
- WiFi must be fully stopped/deinitialized after each sync (user requirement: "disconnect wifi after").
- Never log or expose the WiFi password in sync JSON or logs.

---

### Task 1: WifiTimeSyncService (header + implementation + CMake deps)

**Files:**
- Create: `components/leor_core/include/leor/wifi_time_sync_service.hpp`
- Create: `components/leor_core/src/wifi_time_sync_service.cpp`
- Modify: `components/leor_core/CMakeLists.txt` (REQUIRES line)

**Interfaces:**
- Consumes: `Preferences` (namespace `leor`, `getString`/`putString`), `esp_wifi`, `esp_netif`, `esp_sntp` (lwIP), FreeRTOS.
- Produces (used by Tasks 2-3):
  - `void init(Preferences& prefs)` — loads `wifi_ssid`/`wifi_pass` from NVS.
  - `bool configured() const` — true when `wifi_ssid` non-empty.
  - `bool sync_blocking(uint32_t timeout_ms)` — one-shot connect+SNTP+disconnect; returns true on successful time sync.
  - `void sync_async(std::function<void(const std::string&)> on_done)` — runs `sync_blocking(10000)` on a FreeRTOS task, then calls `on_done("wifi sync ok" | "wifi sync failed")`.
  - `void set_ssid(const std::string& ssid)` / `void set_pass(const std::string& pass)` — persist to NVS.
  - `std::string ssid() const` / `std::string last_status() const`.
  - `void set_time_callback(std::function<void(uint64_t epoch_ms)> cb)` — called with epoch ms on successful sync.

- [ ] **Step 1: Create the header**

`components/leor_core/include/leor/wifi_time_sync_service.hpp`:

```cpp
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
```

- [ ] **Step 2: Create the implementation**

`components/leor_core/src/wifi_time_sync_service.cpp`:

```cpp
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
```

- [ ] **Step 3: Add CMake dependencies**

In `components/leor_core/CMakeLists.txt`, extend the `REQUIRES` list to include `esp_wifi esp_netif lwip` (keep all existing entries):

```cmake
REQUIRES bt driver esp_driver_rmt esp_hw_support esp_timer app_update log nvs_flash nixy4__u8g2 freertos esp_wifi esp_netif lwip
```

- [ ] **Step 4: Build**

Run the firmware build command from `/home/sanic/Leor`. Expected: exit 0, `Project build complete`.

- [ ] **Step 5: Commit**

```bash
git add components/leor_core/include/leor/wifi_time_sync_service.hpp components/leor_core/src/wifi_time_sync_service.cpp components/leor_core/CMakeLists.txt
git commit -m "feat: add WifiTimeSyncService for one-shot SNTP time sync"
```

---

### Task 2: Boot stage integration in Application

**Files:**
- Modify: `components/leor_core/include/leor/application.hpp` (add member)
- Modify: `components/leor_core/src/application.cpp` (BootStage enum, draw_boot_screen, start() wiring)

**Interfaces:**
- Consumes: `WifiTimeSyncService` (Task 1), `ClockService::set_from_epoch_ms(uint64_t epoch_ms, int16_t tz_offset_minutes)`, `Preferences::getInt("clk_tz", 0)`.
- Produces: `BootStage::kWifi` stage; `draw_boot_screen` gains `bool wifi_waiting, bool wifi_ok` params (all call sites updated).

- [ ] **Step 1: Add the member to application.hpp**

After the `HardwareStatus hw_{};` member (near line 54), add:

```cpp
  WifiTimeSyncService wifi_{};
```

And add the include after the existing `#include "leor/hardware_status.hpp"`:

```cpp
#include "leor/wifi_time_sync_service.hpp"
```

- [ ] **Step 2: Extend BootStage and draw_boot_screen**

In `application.cpp` line 119, change the enum to:

```cpp
enum class BootStage { kDisplay, kGyro, kTouch, kBuzzer, kPower, kWifi, kDone };
```

Change the `draw_boot_screen` signature (line 121-122) to:

```cpp
void draw_boot_screen(DisplayBackend& disp, BootStage stage, const HardwareStatus& hw,
                      int gyro_pct, bool touch_waiting, bool wifi_waiting, bool wifi_ok,
                      uint32_t now_ms) {
```

Add the kWifi label case in the switch (after `case BootStage::kPower:`):

```cpp
    case BootStage::kWifi:    label = "WIFI";    break;
```

Add the kWifi hint case in the hint if-chain (after the `kTouch` case, before the closing brace of the chain):

```cpp
  } else if (stage == BootStage::kWifi) {
    std::snprintf(pct_s, sizeof(pct_s), "%s", wifi_waiting ? "SYNC..." : (wifi_ok ? "OK" : "FAIL"));
  }
```

- [ ] **Step 3: Update ALL existing draw_boot_screen call sites**

Every existing call passes two new `false` args before `now_ms`. Call sites (search `draw_boot_screen(` in application.cpp): the kDisplay draw (~line 339), kGyro draw (~357), the probe_cb lambda redraw (~363), the 5s touch-window redraw (~390), kGyro 100% (~417), kTouch 100% (~419), kBuzzer (~447), kPower (~449). Example — the kDisplay call becomes:

```cpp
    draw_boot_screen(*display_, BootStage::kDisplay, hw_, 0, false, false, false, now_ms);
```

- [ ] **Step 4: Wire the service in start()**

After the buzzer init block (~line 447, before the final kBuzzer/kPower draws), add:

```cpp
  wifi_.init(preferences_);
  wifi_.set_time_callback([this](uint64_t epoch_ms) {
    clock_.set_from_epoch_ms(epoch_ms, static_cast<int16_t>(preferences_.getInt("clk_tz", 0)));
  });
```

After the kPower draw + its `vTaskDelay(pdMS_TO_TICKS(200))`, add the WiFi boot stage:

```cpp
  if (wifi_.configured()) {
    const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    draw_boot_screen(*display_, BootStage::kWifi, hw_, 100, false, true, false, now_ms);
    const bool wifi_ok = wifi_.sync_blocking(10000);
    draw_boot_screen(*display_, BootStage::kWifi, hw_, 100, false, false, wifi_ok, now_ms);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
```

- [ ] **Step 5: Build**

Run the firmware build. Expected: exit 0. Fix any missed call sites (compiler will flag them).

- [ ] **Step 6: Commit**

```bash
git add components/leor_core/include/leor/application.hpp components/leor_core/src/application.cpp
git commit -m "feat: add WiFi time-sync boot stage with 10s timeout"
```

---

### Task 3: CommandRouter commands + sync JSON

**Files:**
- Modify: `components/leor_core/include/leor/command_router.hpp` (ctor + member)
- Modify: `components/leor_core/src/command_router.cpp` (ctor init, sync_json, dispatch)
- Modify: `components/leor_core/src/application.cpp` (CommandRouter construction)

**Interfaces:**
- Consumes: `WifiTimeSyncService` (Task 1), `BleService::notify_status(const std::string&)` (public, thread-safe).
- Produces: commands `wifi:ssid=<ssid>`, `wifi:pass=<pass>`, `wifi:sync`; sync JSON gains `"wifi":{"ssid":"%s","status":"%s"}`.

- [ ] **Step 1: command_router.hpp**

Add include `#include "leor/wifi_time_sync_service.hpp"`. Add ctor param `WifiTimeSyncService& wifi` after `const HardwareStatus& hw`, add private member `WifiTimeSyncService& wifi_;` after `hw_`.

- [ ] **Step 2: command_router.cpp ctor**

Add `wifi_(wifi)` to the init list (after `hw_(hw)`).

- [ ] **Step 3: sync_json**

Change the format string tail from `"hardware":{...}}` to add the wifi object before the closing brace:

```cpp
        "\"gesture\":%s,\"hardware\":{\"display\":%d,\"gyro\":%d,\"buzzer\":%d,\"touch\":%d,\"power\":%d},"
        "\"wifi\":{\"ssid\":\"%s\",\"status\":\"%s\"}}",
```

Append two args after the hardware args:

```cpp
        static_cast<int>(hw_.power),
        wifi_.ssid().c_str(), wifi_.last_status().c_str());
```

- [ ] **Step 4: Dispatch**

Near the `hw:status` dispatch, add:

```cpp
    if (starts_with(cmd, "wifi:ssid=")) {
        wifi_.set_ssid(cmd.substr(10));
        return "ssid saved";
    }
    if (starts_with(cmd, "wifi:pass=")) {
        wifi_.set_pass(cmd.substr(10));
        return "pass saved";
    }
    if (cmd == "wifi:sync") {
        wifi_.sync_async([this](const std::string& status) { ble_.notify_status(status); });
        return "wifi sync started";
    }
```

- [ ] **Step 5: application.cpp CommandRouter construction**

Append `, wifi_` to the CommandRouter constructor call (after the `hw_` argument).

- [ ] **Step 6: Build**

Run the firmware build. Expected: exit 0.

- [ ] **Step 7: Commit**

```bash
git add components/leor_core/include/leor/command_router.hpp components/leor_core/src/command_router.cpp components/leor_core/src/application.cpp
git commit -m "feat: add wifi:ssid/wifi:pass/wifi:sync commands and wifi sync state"
```

---

### Task 4: Web BLE client — wifi state, parse, commands

**Files:**
- Modify: `web/src/lib/ble.svelte.ts`

**Interfaces:**
- Consumes: sync JSON `data.wifi = {ssid, status}` (Task 3).
- Produces: `bleState.wifi = {ssid:'', status:''}`, `getWifiSsid()`, `getWifiStatus()`, `saveWifiCredentials(ssid, pass)`, `sendWifiSync()`.

- [ ] **Step 1: Add state**

In `bleState` (after the `hardware` object), add:

```ts
	wifi: { ssid: '', status: '' },
```

- [ ] **Step 2: Add getters**

After `getHardware()`, add:

```ts
export function getWifiSsid() {
	return bleState.wifi.ssid;
}

export function getWifiStatus() {
	return bleState.wifi.status;
}
```

- [ ] **Step 3: Parse in sync handler**

Inside the sync parse block (after the hardware parse), add:

```ts
		if (data.wifi) {
			if ('ssid' in data.wifi) bleState.wifi.ssid = data.wifi.ssid;
			if ('status' in data.wifi) bleState.wifi.status = data.wifi.status;
		}
```

- [ ] **Step 4: Add command senders**

After `setBuzzerMode`, add:

```ts
export async function saveWifiCredentials(ssid: string, pass: string) {
	await sendCommand(`wifi:ssid=${ssid}`);
	await sendCommand(`wifi:pass=${pass}`);
}

export async function sendWifiSync() {
	await sendCommand('wifi:sync');
}
```

- [ ] **Step 5: Build + check**

Run `npm run build` in `web/` (expected: All good!) and `npm run check` (expected: same 8 pre-existing errors + 9 warnings, no new ones).

- [ ] **Step 6: Commit**

```bash
git add web/src/lib/ble.svelte.ts
git commit -m "feat: add wifi state, parsing, and commands to web BLE client"
```

---

### Task 5: WifiSettings panel component + page integration

**Files:**
- Create: `web/src/lib/components/WifiSettings.svelte`
- Modify: `web/src/routes/+page.svelte` (import + render in system settings section)

**Interfaces:**
- Consumes: `getWifiSsid`, `getWifiStatus`, `saveWifiCredentials`, `sendWifiSync` (Task 4).
- Produces: a bento-card panel with SSID input, password input, Save button, Sync now button, status line.

- [ ] **Step 1: Create WifiSettings.svelte**

Follow the existing panel style (see `PowerSettings.svelte`: `bento-card bg-paper p-4`, header `mb-4 border-b-2 border-bento-border pb-2` with `h2` `font-display text-xl uppercase` and `p` subtitle, controls in `flex` rows). Content:

```svelte
<script lang="ts">
	import { getWifiSsid, getWifiStatus, saveWifiCredentials, sendWifiSync } from '$lib/ble.svelte';

	let ssid = $derived(getWifiSsid());
	let status = $derived(getWifiStatus());
	let ssidInput = $state('');
	let passInput = $state('');
	let saving = $state(false);
	let syncing = $state(false);

	async function save() {
		saving = true;
		await saveWifiCredentials(ssidInput, passInput);
		saving = false;
	}

	async function syncNow() {
		syncing = true;
		await sendWifiSync();
		setTimeout(() => (syncing = false), 3000);
	}
</script>

<div class="bento-card bg-paper p-4">
	<div class="mb-4 border-b-2 border-bento-border pb-2">
		<h2 class="font-display text-xl uppercase">WiFi Time Sync</h2>
		<p class="text-sm font-bold opacity-80">One-shot NTP sync, then WiFi disconnects</p>
	</div>
	<div class="flex flex-col gap-3">
		<input
			bind:value={ssidInput}
			placeholder="SSID"
			class="rounded-xl border-2 border-bento-border bg-bento-pink px-3 py-2 text-sm font-bold"
		/>
		<input
			bind:value={passInput}
			type="password"
			placeholder="Password"
			class="rounded-xl border-2 border-bento-border bg-bento-pink px-3 py-2 text-sm font-bold"
		/>
		<div class="flex gap-2">
			<button
				class="bento-button bg-bento-blue px-4 py-2 text-sm font-bold uppercase"
				onclick={save}
				disabled={saving}
			>
				{saving ? 'Saving...' : 'Save'}
			</button>
			<button
				class="bento-button bg-bento-peach px-4 py-2 text-sm font-bold uppercase"
				onclick={syncNow}
				disabled={syncing}
			>
				{syncing ? 'Syncing...' : 'Sync now'}
			</button>
		</div>
		<p class="text-xs font-bold uppercase tracking-widest opacity-80">
			SSID: {ssid || '—'} · Status: {status || '—'}
		</p>
	</div>
</div>
```

- [ ] **Step 2: Integrate in +page.svelte**

Add the import after the `HardwareStatusPanel` import:

```svelte
	import WifiSettings from '$lib/components/WifiSettings.svelte';
```

Render it inside the system settings section, after `<HardwareStatusPanel />`:

```svelte
				<WifiSettings />
```

- [ ] **Step 3: Build + check**

Run `npm run build` (expected: All good!) and `npm run check` (expected: no new errors/warnings).

- [ ] **Step 4: Commit**

```bash
git add web/src/lib/components/WifiSettings.svelte web/src/routes/+page.svelte
git commit -m "feat: add WiFi time-sync settings panel to dashboard"
```

---

### Task 6: API.md documentation

**Files:**
- Modify: `API.md`

- [ ] **Step 1: Document the new commands**

In the `## System` section (after the `hw:status` line), add:

```markdown
- `wifi:ssid=<ssid>` — stores the WiFi SSID in NVS (used for time sync)
- `wifi:pass=<password>` — stores the WiFi password in NVS
- `wifi:sync` — one-shot WiFi connect + NTP sync (pool.ntp.org), sets the clock, then disconnects WiFi
```

- [ ] **Step 2: Commit**

```bash
git add API.md
git commit -m "docs: document wifi time-sync commands"
```

---

## Self-Review

**Spec coverage:** boot stage (Task 2), manual trigger (Tasks 3+5), web-configured credentials (Tasks 3+5), pool.ntp.org (Task 1), disconnect after sync (Task 1: `esp_wifi_stop`+`esp_wifi_deinit`+`esp_netif_deinit` in every path), web UI panel (Task 5), sync JSON status (Task 3), docs (Task 6). All covered.

**Placeholder scan:** no TBD/TODO; every code step has full code.

**Type consistency:** `WifiTimeSyncService` signatures identical across Tasks 1-3; `draw_boot_screen` new params `wifi_waiting`/`wifi_ok` used consistently in Task 2; web getters/senders names match Task 5 usage.
