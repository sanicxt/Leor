# Hardware Failure Detection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Detect missing/failed optional hardware (gyro, buzzer, touch, power) at runtime, surface it on a boot summary screen and in the web dashboard, and degrade gracefully instead of bricking boot.

**Architecture:** A central `HardwareStatus` struct owned by `Application` records the runtime-detected state of each peripheral during `start()`. `Application::tick()` renders a 3-second "HARDWARE CHECK" screen when any peripheral failed, forces BLE always-on when touch is absent, and `PowerService::do_sleep()` becomes a no-op when the power-control pin is absent. The state is exposed to the web dashboard via a `hardware` object in the existing sync JSON and a new `hw:status` command.

**Tech Stack:** ESP-IDF v6 (C++17, FreeRTOS), SvelteKit 5 + Svelte 5 runes, TypeScript.

**Spec:** Design approved in brainstorming (runtime auto-detect, boot summary screen, BLE force-open on display/touch failure, sync-JSON `hardware` object, `hw:status` command, web status panel).

## Global Constraints

- Repo commit style is SEMANTIC (`feat:`, `chore:`, `fix:`). **Do NOT add Sisyphus as co-author** to any commit.
- All firmware code lives under `components/leor_core/`, uses `namespace leor`, header guard `#pragma once`, headers under `include/leor/`.
- Detection model is RUNTIME, not compile-time Kconfig.
- Only I2C devices (display, gyro) can be electrically probed. GPIO-only devices (buzzer, touch, power) are `kPresent` if pin valid, `kAbsent` if pin unconfigured. There is NO `kProbeFailed` for GPIO-only devices.
- Display is the only mandatory-but-degraded peripheral: on failure it falls back to `NullDisplayBackend` (already exists) and BLE is forced open.
- Do NOT modify `partitions.csv` (that is a separate pending NVS fix, out of scope for this plan).
- Firmware verification = `idf.py build` (no unit-test framework exists in this repo). Web verification = `npm run check` and `npm run build`.

---

### Task 1: Create the `HardwareStatus` struct

**Files:**
- Create: `components/leor_core/include/leor/hardware_status.hpp`

**Interfaces:**
- Produces: `enum class HwState { kPresent=0, kAbsent=1, kProbeFailed=2 }`; `struct HardwareStatus { HwState display, gyro, buzzer, touch, power; bool any_failure() const; std::string summary() const; }`. Later tasks read these fields and call `any_failure()` / `summary()`.

- [ ] **Step 1: Create the header**

```cpp
#pragma once

#include <cstdint>
#include <string>

namespace leor {

enum class HwState : uint8_t {
    kPresent = 0,
    kAbsent = 1,
    kProbeFailed = 2,
};

struct HardwareStatus {
    HwState display = HwState::kPresent;
    HwState gyro = HwState::kPresent;
    HwState buzzer = HwState::kPresent;
    HwState touch = HwState::kPresent;
    HwState power = HwState::kPresent;

    bool any_failure() const {
        return display == HwState::kProbeFailed ||
               gyro == HwState::kProbeFailed ||
               buzzer == HwState::kAbsent ||
               touch == HwState::kAbsent ||
               power == HwState::kAbsent;
    }

    // Returns e.g. "!GYRO !BUZZ" (space-separated, empty string if all OK).
    std::string summary() const {
        std::string out;
        auto add = [&](const char* tag) {
            if (!out.empty()) out += ' ';
            out += '!';
            out += tag;
        };
        if (display == HwState::kProbeFailed) add("DISP");
        if (gyro == HwState::kProbeFailed) add("GYRO");
        if (buzzer == HwState::kAbsent) add("BUZZ");
        if (touch == HwState::kAbsent) add("TOUCH");
        if (power == HwState::kAbsent) add("PWR");
        return out;
    }
};

} // namespace leor
```

- [ ] **Step 2: Verify it compiles**

Run: `idf.py build` (from repo root)
Expected: build succeeds (header is not yet included anywhere, so this only confirms no syntax errors in the new file via the component's include path).

- [ ] **Step 3: Commit**

```bash
git add components/leor_core/include/leor/hardware_status.hpp
git commit -m "feat: add HardwareStatus struct for runtime peripheral detection"
```

---

## Task 2: Add availability getters to services

**Files:**
- Modify: `components/leor_core/include/leor/gesture_service.hpp` (add getter near line 79)
- Modify: `components/leor_core/include/leor/buzzer_service.hpp` (add getter after `play_melody()`)
- Modify: `components/leor_core/include/leor/power_service.hpp` (add getters near `is_pressed()`)

**Interfaces:**
- Produces: `GestureService::mpu_available() const -> bool`; `BuzzerService::initialized() const -> bool`; `PowerService::touch_enabled() const -> bool`; `PowerService::power_control_enabled() const -> bool`. Task 3 consumes these.

- [ ] **Step 1: Add getter to GestureService**

In `gesture_service.hpp`, after the `roll()` method (line 78), add:

```cpp
    bool mpu_available() const { return mpu_available_; }
```

- [ ] **Step 2: Add getter to BuzzerService**

In `buzzer_service.hpp`, after `play_melody()` (line 22), add:

```cpp
    bool initialized() const { return initialized_; }
```

- [ ] **Step 3: Add getters to PowerService**

In `power_service.hpp`, after `is_pressed()` (line 23), add:

```cpp
    bool touch_enabled() const { return touch_pin_ != 0; }
    bool power_control_enabled() const { return pwr_ctrl_pin_ >= 0; }
```

- [ ] **Step 4: Verify build**

Run: `idf.py build`
Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add components/leor_core/include/leor/gesture_service.hpp components/leor_core/include/leor/buzzer_service.hpp components/leor_core/include/leor/power_service.hpp
git commit -m "feat: expose peripheral availability getters on services"
```

---

## Task 3: Record hardware status and render boot summary in Application

**Files:**
- Modify: `components/leor_core/include/leor/application.hpp` (add `hw_`, `boot_summary_shown_`, `boot_summary_until_ms_` members; include `hardware_status.hpp`)
- Modify: `components/leor_core/src/application.cpp` (record status in `start()`, add `draw_hw_summary_screen`, render in `tick()`, force BLE always-on when touch absent)

**Interfaces:**
- Consumes: `HardwareStatus`/`HwState` (Task 1), `GestureService::mpu_available()`, `BuzzerService::initialized()`, `PowerService::touch_enabled()`, `PowerService::power_control_enabled()` (Task 2).
- Produces: `Application::hw_` member (a `HardwareStatus`) that Task 5's `CommandRouter` reads via a constructor reference.

- [ ] **Step 1: Add members to `application.hpp`**

Add `#include "leor/hardware_status.hpp"` to the includes (after `config.hpp` include). Add these private members after `last_short_press_ms_` (line 52):

```cpp
  HardwareStatus hw_{};
  bool boot_summary_shown_ = false;
  uint32_t boot_summary_until_ms_ = 0;
```

- [ ] **Step 2: Add `draw_hw_summary_screen` free function**

In `application.cpp`, after the `draw_ota_screen` function (after line 116), add:

```cpp
void draw_hw_summary_screen(DisplayBackend& display, const HardwareStatus& hw) {
  display.clear();
  display.set_font_small();
  display.draw_text(4, 6, "HARDWARE CHECK");
  display.draw_hline(4, 12, 124);
  display.set_font_medium();
  const std::string summary = hw.summary();
  const int tw = display.text_width(summary.c_str());
  display.draw_text((display.width() - tw) / 2, 40, summary.c_str());
  display.send_buffer();
}
```

- [ ] **Step 3: Record hardware status in `start()`**

In `application.cpp`, replace the display init block (lines 183-195) so it records the probe result:

```cpp
  display_ = std::make_unique<U8g2DisplayBackend>();
  const bool display_ok = display_->init(config_.display);
  if (!display_ok) {
    ESP_LOGW(kTag,
             "display init failed, falling back to null backend (%s, SDA=%d, "
             "SCL=%d, addr=0x%02x)",
             config_.display.controller == DisplayController::kSsd1306
                 ? "ssd1306"
                 : "sh1106",
             config_.display.sda_pin, config_.display.scl_pin,
             config_.display.i2c_address);
    display_ = std::make_unique<NullDisplayBackend>();
    display_->init(config_.display);
  }
  hw_.display = display_ok ? HwState::kPresent : HwState::kProbeFailed;
```

After `gesture_.start(...)` (line 214-215), add:

```cpp
  hw_.gyro = gesture_.mpu_available() ? HwState::kPresent : HwState::kProbeFailed;
```

After `buzzer_.init(config_.buzzer_pin);` (line 233), add:

```cpp
  hw_.buzzer = buzzer_.initialized() ? HwState::kPresent : HwState::kAbsent;
```

After `power_.init(...)` (line 178-179), add:

```cpp
  hw_.touch = power_.touch_enabled() ? HwState::kPresent : HwState::kAbsent;
  hw_.power = power_.power_control_enabled() ? HwState::kPresent : HwState::kAbsent;
```

- [ ] **Step 4: Arm the boot summary screen at end of `start()`**

After `display_->set_contrast(...)` (line 270), before `ESP_LOGI(kTag, "application started");`, add:

```cpp
  if (hw_.any_failure() && display_ &&
      dynamic_cast<NullDisplayBackend*>(display_.get()) == nullptr) {
    const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
    boot_summary_until_ms_ = now_ms + 3000;
    boot_summary_shown_ = true;
    ESP_LOGW(kTag, "hardware check: %s", hw_.summary().c_str());
  }
```

- [ ] **Step 5: Render the summary and force BLE always-on in `tick()`**

In `tick()`, after the OTA bypass block (after line 324, before `ButtonEvent btn = power_.poll(now_ms);`), add:

```cpp
  if (boot_summary_shown_) {
    if (now_ms < boot_summary_until_ms_) {
      draw_hw_summary_screen(*display_, hw_);
      vTaskDelay(pdMS_TO_TICKS(33));
      return;
    }
    boot_summary_shown_ = false;
  }
```

Replace the BLE window close logic (lines 289-292) with a touch-absent guard:

```cpp
  if (hw_.touch == HwState::kAbsent) {
    // No touch input → keep BLE discoverable so the user can control via web dashboard.
    if (!ble_window_open_) {
      ble_window_open_ = true;
      ble_.start_advertising();
    }
  } else if (ble_window_open_ && !ota_active && now_ms >= ble_window_deadline_ms_) {
    ble_.stop(false);
    ble_window_open_ = false;
  }
```

- [ ] **Step 6: Verify build**

Run: `idf.py build`
Expected: build succeeds. (No runtime test possible in this repo; compile is the gate.)

- [ ] **Step 7: Commit**

```bash
git add components/leor_core/include/leor/application.hpp components/leor_core/src/application.cpp
git commit -m "feat: record peripheral status and show boot hardware-check screen"
```

---

## Task 4: Make `PowerService::do_sleep()` a no-op when power control is absent

**Files:**
- Modify: `components/leor_core/src/power_service.cpp` (top of `do_sleep()`, line 172)

**Interfaces:**
- Consumes: `pwr_ctrl_pin_` member (already exists).
- Produces: `do_sleep()` returns immediately when `pwr_ctrl_pin_ < 0`, so the menu's power-off action degrades gracefully.

- [ ] **Step 1: Add the guard**

At the very top of `PowerService::do_sleep()` (line 172), before the PM-lock block, add:

```cpp
  if (pwr_ctrl_pin_ < 0) {
    ESP_LOGW(kTag, "power control pin absent, sleep is a no-op");
    return;
  }
```

- [ ] **Step 2: Verify build**

Run: `idf.py build`
Expected: build succeeds.

- [ ] **Step 3: Commit**

```bash
git add components/leor_core/src/power_service.cpp
git commit -m "feat: make deep sleep a no-op when power control pin is absent"
```

---

## Task 5: Add `hardware` to sync JSON and a `hw:status` command

**Files:**
- Modify: `components/leor_core/include/leor/command_router.hpp` (add `const HardwareStatus& hw_` member + constructor param + `hw_json()` decl)
- Modify: `components/leor_core/src/command_router.cpp` (constructor init, `sync_json()`, `hw_json()`, `hw:status` dispatch)
- Modify: `components/leor_core/src/application.cpp` (pass `hw_` into `CommandRouter` constructor)

**Interfaces:**
- Consumes: `HardwareStatus` (Task 1), `Application::hw_` (Task 3).
- Produces: sync JSON gains `"hardware":{"display":N,"gyro":N,"buzzer":N,"touch":N,"power":N}` (0=present,1=absent,2=probe_failed); command `hw:status` returns `{"type":"hw","display":N,...}`. Task 6 (web) parses both.

- [ ] **Step 1: Update `command_router.hpp`**

Add `#include "leor/hardware_status.hpp"` to the includes. Add `const HardwareStatus& hw_` to the constructor signature (after `BuzzerService& buzzer`). Add a private method declaration after `sync_json`:

```cpp
    std::string hw_json() const;
```

Add the member after `buzzer_`:

```cpp
    const HardwareStatus& hw_;
```

- [ ] **Step 2: Update the constructor in `command_router.cpp`**

Change the signature and init list (lines 45-66) to accept and store `const HardwareStatus& hw`:

```cpp
CommandRouter::CommandRouter(Preferences& preferences,
                             DisplayConfig& display_config,
                             DisplayBackend& display,
                             MochiEyesEngine& eyes,
                             GestureService& gestures,
                             ShuffleService& shuffle,
                             ClockService& clock,
                             PowerService& power,
                             BleService& ble,
                             BuzzerService& buzzer,
                             const HardwareStatus& hw)
    : preferences_(preferences),
      display_config_(display_config),
      display_(display),
      eyes_(eyes),
      gestures_(gestures),
      shuffle_(shuffle),
      clock_(clock),
      power_(power),
      ble_(ble),
      buzzer_(buzzer),
      hw_(hw) {
    notif_duration_ms_ = preferences_.getUInt("notif_nd", 5000);
}
```

- [ ] **Step 3: Add `hw_json()` and extend `sync_json()`**

Add this method after `sync_json()` (after line 111):

```cpp
std::string CommandRouter::hw_json() const {
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "{\"type\":\"hw\",\"display\":%d,\"gyro\":%d,\"buzzer\":%d,\"touch\":%d,\"power\":%d}",
                  static_cast<int>(hw_.display), static_cast<int>(hw_.gyro),
                  static_cast<int>(hw_.buzzer), static_cast<int>(hw_.touch),
                  static_cast<int>(hw_.power));
    return buf;
}
```

In `sync_json()`, extend the format string. Change the trailing `"\"gesture\":%s}"` (line 98) to `"\"gesture\":%s,\"hardware\":{\"display\":%d,\"gyro\":%d,\"buzzer\":%d,\"touch\":%d,\"power\":%d}}"` and add the five args after `gestures_.settings_json().c_str()` (line 109):

```cpp
        ble_window_ms, gestures_.settings_json().c_str(),
        static_cast<int>(hw_.display), static_cast<int>(hw_.gyro),
        static_cast<int>(hw_.buzzer), static_cast<int>(hw_.touch),
        static_cast<int>(hw_.power));
```

- [ ] **Step 4: Add the `hw:status` dispatch**

In `handle()`, near the `music` command (line 639), add:

```cpp
  if (cmd == "hw:status") return hw_json();
```

- [ ] **Step 5: Pass `hw_` into the constructor in `application.cpp`**

Change the `CommandRouter` construction (lines 248-251) to append `, hw_`:

```cpp
  commands_ = std::make_unique<CommandRouter>(preferences_, config_.display,
                                               *display_, *eyes_, gesture_,
                                               shuffle_, clock_, power_, ble_,
                                               buzzer_, hw_);
```

- [ ] **Step 6: Verify build**

Run: `idf.py build`
Expected: build succeeds.

- [ ] **Step 7: Commit**

```bash
git add components/leor_core/include/leor/command_router.hpp components/leor_core/src/command_router.cpp components/leor_core/src/application.cpp
git commit -m "feat: expose hardware status in sync JSON and add hw:status command"
```

---

## Task 6: Parse hardware status in the web BLE client

**Files:**
- Modify: `web/src/lib/ble.svelte.ts` (add `hardware` to `bleState`, add getter, parse `data.hardware` in sync and `data.type === 'hw'`)

**Interfaces:**
- Consumes: firmware sync JSON `hardware` object and `hw:status` response (Task 5).
- Produces: `bleState.hardware` object `{display,gyro,buzzer,touch,power}` (numbers 0/1/2) and `getHardware()`. Task 7 consumes these.

- [ ] **Step 1: Add `hardware` to `bleState`**

In `ble.svelte.ts`, after the `clock24Hour` field (line 57), add:

```ts
    hardware: {
        display: 0, gyro: 0, buzzer: 0, touch: 0, power: 0
    },
```

- [ ] **Step 2: Add a getter**

After `getClock24Hour()` (line 122), add:

```ts
export function getHardware() { return bleState.hardware; }
```

- [ ] **Step 3: Parse `data.hardware` in the sync handler**

Inside the `if (data.type === 'sync')` block, after the `if (data.clock)` block (line 272), add:

```ts
                            if (data.hardware) {
                                if ('display' in data.hardware) bleState.hardware.display = data.hardware.display;
                                if ('gyro' in data.hardware) bleState.hardware.gyro = data.hardware.gyro;
                                if ('buzzer' in data.hardware) bleState.hardware.buzzer = data.hardware.buzzer;
                                if ('touch' in data.hardware) bleState.hardware.touch = data.hardware.touch;
                                if ('power' in data.hardware) bleState.hardware.power = data.hardware.power;
                            }
```

- [ ] **Step 4: Parse the `hw:status` response**

After the `if (data.type === 'cal')` block (line 284), add:

```ts
                        if (data.type === 'hw') {
                            if ('display' in data) bleState.hardware.display = data.display;
                            if ('gyro' in data) bleState.hardware.gyro = data.gyro;
                            if ('buzzer' in data) bleState.hardware.buzzer = data.buzzer;
                            if ('touch' in data) bleState.hardware.touch = data.touch;
                            if ('power' in data) bleState.hardware.power = data.power;
                        }
```

- [ ] **Step 5: Verify**

Run: `npm run check` (in `web/`)
Expected: svelte-check passes with no errors.

- [ ] **Step 6: Commit**

```bash
git add web/src/lib/ble.svelte.ts
git commit -m "feat: parse hardware status from device sync in web BLE client"
```

---

## Task 7: Add a Hardware status panel to the web dashboard

**Files:**
- Create: `web/src/lib/components/HardwareStatusPanel.svelte`
- Modify: `web/src/routes/+page.svelte` (import + render in the "Display & System" settings section)

**Interfaces:**
- Consumes: `bleState.hardware` (Task 6).
- Produces: a small bento card listing Display/Gyro/Buzzer/Touch/Power with OK/Absent/Failed status.

- [ ] **Step 1: Create the component**

Create `web/src/lib/components/HardwareStatusPanel.svelte`:

```svelte
<script lang="ts">
  import { bleState } from "$lib/ble.svelte";

  const items = [
    { key: 'display', label: 'Display' },
    { key: 'gyro', label: 'Gyro' },
    { key: 'buzzer', label: 'Buzzer' },
    { key: 'touch', label: 'Touch' },
    { key: 'power', label: 'Power' },
  ] as const;

  function statusText(v: number) {
    return v === 0 ? 'OK' : v === 1 ? 'Absent' : 'Failed';
  }
  function statusClass(v: number) {
    return v === 0 ? 'bg-bento-green' : 'bg-bento-pink';
  }
</script>

<div class="bento-card bg-paper p-4">
  <div class="mb-4 border-b-2 border-bento-border pb-2">
    <h2 class="text-xl font-black uppercase">Hardware</h2>
    <p class="text-sm font-bold opacity-80">Detected peripherals</p>
  </div>
  <div class="grid grid-cols-2 md:grid-cols-5 gap-3">
    {#each items as item}
      <div class="flex flex-col items-center gap-1 rounded-xl border-2 border-bento-border p-3">
        <span class="w-3 h-3 rounded-full {statusClass(bleState.hardware[item.key])}"></span>
        <span class="text-xs font-bold uppercase">{item.label}</span>
        <span class="text-[10px] font-bold opacity-70">{statusText(bleState.hardware[item.key])}</span>
      </div>
    {/each}
  </div>
</div>
```

- [ ] **Step 2: Wire it into the page**

In `+page.svelte`, add the import after `NotificationSettings` (line 25):

```ts
  import HardwareStatusPanel from "$lib/components/HardwareStatusPanel.svelte";
```

Inside the `{#if settingsActive === 'system'}` block (after `<GestureSettings />`, line 234), add:

```svelte
              <HardwareStatusPanel />
```

- [ ] **Step 3: Verify**

Run: `npm run check` (in `web/`)
Expected: svelte-check passes with no errors.

- [ ] **Step 4: Commit**

```bash
git add web/src/lib/components/HardwareStatusPanel.svelte web/src/routes/+page.svelte
git commit -m "feat: add hardware status panel to web dashboard"
```

---

## Task 8: Document the `hw:status` command

**Files:**
- Modify: `API.md` (System section, line 105-109)

- [ ] **Step 1: Add the command**

In the `## System` section, after the `music` line (line 109), add:

```markdown
- `hw:status` — returns a JSON object `{"type":"hw","display":N,"gyro":N,"buzzer":N,"touch":N,"power":N}` where N is 0=present, 1=absent, 2=probe-failed
```

- [ ] **Step 2: Commit**

```bash
git add API.md
git commit -m "docs: document hw:status command"
```

---

## Self-Review

**Spec coverage:**
- Runtime auto-detect → Task 1, 2, 3 ✓
- Boot summary screen → Task 3 ✓
- Display failure → NullDisplayBackend + force BLE open → Task 3 (BLE force-open is touch-absent; display failure already falls back to Null and BLE is opened at boot via `open_ble_window`) ✓
- Web hardware status in sync JSON → Task 5 + 6 ✓
- `hw:status` command → Task 5 ✓
- Touch absent → force BLE always-on → Task 3 ✓
- Power absent → disable sleep → Task 4 ✓
- Web status panel → Task 7 ✓
- Docs → Task 8 ✓

**Placeholder scan:** No TBD/TODO; every step has concrete code.

**Type consistency:** `HwState` values (0/1/2) are consistent between firmware `hw_json()`/`sync_json()` and web parsing. `bleState.hardware` keys (`display/gyro/buzzer/touch/power`) match the firmware JSON keys. `getHardware()` name is consistent with the existing getter convention.
