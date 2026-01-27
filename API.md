# Leor — CLI & BLE API

This document lists the serial and BLE commands supported by Leor (derived from `commands.h`). Commands can be sent via Serial or over BLE. Format: `command` or `command=value`.

For quick help on a connected device, send the `help` command (prints to serial).

---

## Expressions
- happy — Set happy expression (laugh animation, mouth type).
- sad — Set sad expression.
- angry — Set angry expression.
- love — Set love expression (heart eyes / love animation).
- surprised — Set surprised expression.
- confused — Set confused expression.
- sleepy — Set sleepy expression.
- curious — Set curious expression.
- nervous — Turn on sweat + curiosity.
- knocked / dizzy — Turn on knocked/dizzy effect.
- neutral / normal / reset — Reset to neutral default expression.
- idle — Enter idle mode (gentle idle motions).
- raised — Raised eyebrows expression.

## Mouth
- smile / frown / open / ooo / flat — Set mouth shape.
- uwum / xdm — Alternate mouth shapes (UwU/XD).
- talk [ms] — Start talk animation for N ms (default 3000). Range: 100–10000.
- chew [ms] — Start chew animation for N ms (default 2000). Range: 100–10000.
- wobble [ms] — Start wobble animation for N ms (default 2000). Range: 100–10000.

## Actions
- blink — Blink once.
- wink — Wink (left).
- winkr — Wink right.
- laugh — Laugh animation.
- cry — Cry animation.
- uwu — Trigger UwU expression (3s default).
- xd — Trigger XD expression (3s default).

## Positions (gaze)
- center — Gaze to center.
- n / up — North.
- ne — North-East.
- e / right — East.
- se — South-East.
- s / down — South.
- sw — South-West.
- w / left — West.
- nw — North-West.

## Toggles & Misc
- sweat — Toggle sweat effect.
- cyclops — Toggle cyclops mode.
- mouth — Toggle mouth on/off.
- mpulog — Toggle MPU6050 verbose logging (returns "MPU verbose ON/OFF").

## Breathing controls
- br: — Report breathing status (returns JSON-like line for web UI).
- br=1 / br=0 — Enable/disable breathing explicitly.
- br — Toggle breathing on/off.
- bri=FLOAT — Set breathing intensity (e.g., `bri=0.05`).
- brs=FLOAT — Set breathing speed.

## Settings (s: or set:)
- s: or set: with comma-separated key=value pairs — apply settings and save to preferences.
  - If called with no params (just `s:`) returns a JSON sync payload describing current settings.
- Supported keys (examples):
  - `ew` eye width (px)
  - `eh` eye height (px)
  - `es` eye spacing
  - `er` eye roundness
  - `mw` mouth width
  - `lt` laugh duration (ms)
  - `vt` love duration (ms)
  - `bi` blink interval
  - `gs` gaze speed
  - `os` openness speed
  - `ss` squish speed

Examples:
- `s:ew=36,eh=36`
- `s:` (returns settings JSON)

## Gesture commands
- gs — Start streaming gyro data to browser. Returns: `gs:1`.
- gx — Stop streaming. Returns: `gs:0`.
- gl=index:name:action — Set gesture label (index, name, action). Returns `gl:ok` / `gl:err`.
- ga=index:action — Set gesture action mapping. Returns `ga:ok` / `ga:err`.
- gm=1 / gm=0 — Enable/disable gesture matching. Returns `gm=1`/`gm=0`.
- gc — Clear all gestures.
- gi — Get gesture info (returns JSON list of gestures and actions).
- gs: — Get gesture settings (returns the gesture settings string).
- grt=MS — Set reaction time in milliseconds. Allowed range: 500–10000. Returns `rt=<ms>` or `grt:err`.
- gcf=PERCENT — Set confidence threshold (30–99%). Returns `cf=<val>` or `gcf:err`.
- gcd=MS — Set cooldown in ms (500–10000). Returns `cd=<ms>` or `gcd:err`.

## BLE power
- ble: — Get BLE power/low-power status. Example response: `ble:lp=1`.
- ble:lp=1 / ble:lp=0 — Set BLE low power mode. Returns current setting.

## Expression shuffle (sh: or shuffle:)
- sh: / shuffle: with comma-separated tokens:
  - `on` / `off` — enable/disable shuffle
  - `quick` / `slow` — presets for timings
  - `expr=MIN-MAX` or `expr=N` — expression duration in seconds
  - `neutral=MIN-MAX` or `neutral=N` — neutral duration in seconds
- Example: `sh:on`, `sh:expr=2-5,neutral=1-3`
- Query with `sh:` returns current shuffle state and ranges.

## Display settings
- display:type=<sh1106|ssd1306> — Set display type (saves preference; restart required to apply if changed).
- display:addr=<0x3C|0x3D> — Set I2C address (saves preference; restart required).
- display:test — Run display test animation.
- display:clear — Clear the display.
- display:info — Return current display info (type, address, resolution, saved prefs).

## Weight transfer / Model upload
- `gw+<BASE64_CHUNK>` — Append a base64-encoded weight chunk for weight transfer (used by model upload tools). Returns `gw+ok` on success.
- `gw!` — Finalize weight transfer. Returns `gw:ok` or `gw:err`.
- `gw=<BASE64_DATA>` — Single-chunk weight load for small models. Returns `gw:ok` or `gw:err`.

## Notes about payloads & behavior
- Multiple commands may be sent in a single BLE write separated by **newlines** or **semicolons**; the device will process them sequentially.
- Some commands have short vs long forms that differ by a trailing colon. Example: `gs` starts accel/gyro streaming, whereas `gs:` returns gesture settings.
- Some mouth/face commands exist in two forms:
  - `uwum` / `xdm` (set mouth shapes)
  - `uwu` / `xd` (trigger full UwU/XD expressions)

## Restart & Help
- restart / reboot — Restart the ESP32.
- help / ? — Print help (see serial output).

---

Notes:
- Many commands print status to Serial and return short response strings when invoked via the web UI or BLE bridge.
- For BLE integration details, refer to `ble_manager.h` (uses NimBLE-Arduino) and the web UI implementation `leora/src/lib/ble.svelte.ts`.
- This document was generated from `commands.h`. If you add new commands to the source, update this file accordingly.
