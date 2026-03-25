# Leor CLI + BLE Command API (ESP-IDF)

This is the active command surface for the ESP-IDF firmware in this repository.

- Commands can be sent over BLE command characteristic or serial.
- Format: `command` or `command=value`
- Some groups use prefixes like `s:`, `sh:`, `display:`, `clock:`.

## Expressions

- `happy`
- `sad`
- `angry`
- `love`
- `surprised`
- `confused`
- `sleepy`
- `curious`
- `nervous`
- `knocked` / `dizzy`
- `neutral` / `normal` / `reset`
- `idle`
- `raised`

## Mouth + Actions

- Mouth shapes: `smile`, `frown`, `open`, `ooo`, `flat`, `uwum`, `xdm`
- Timed animations: `talk [ms]`, `chew [ms]`, `wobble [ms]`
- Actions: `blink`, `wink`, `winkr`, `laugh`, `cry`

## Gaze

- `center`
- `n`/`up`, `ne`, `e`/`right`, `se`, `s`/`down`, `sw`, `w`/`left`, `nw`

## Breathing + Effects

- `sweat`
- `cyclops`
- `mouth`
- `mpulog`
- `br:` status
- `br=1` / `br=0`
- `br` toggle
- `bri=<float>` intensity
- `brs=<float>` speed

## Settings Sync (`s:` / `set:`)

- `s:` returns full sync JSON
- `s:key=value,...` applies and persists settings

Common keys:

- `ew`, `eh`, `es`, `er`, `mw`
- `td` touch hold ms
- `bi`, `gs`, `os`, `ss`
- `wp` wake pin, `pp` power pin

## Gesture Commands

- `gs` / `gx`
- `ga=index:action`
- `gm=1|0`
- `gc`
- `gi`
- `gs:` gesture settings payload
- `grt=<ms>`
- `gcf=<percent>`
- `gcd=<ms>`

## BLE Commands

- `ble:` -> low-power status
- `ble:lp=1|0`
- `ble:name`
- `ble:name=<new_name>`

## Touch / Power

- `tw:` -> touch wake status

## Shuffle

- `sh:` status
- `sh:on`, `sh:off`, `sh:quick`, `sh:slow`
- `sh:expr=min-max`, `sh:neutral=min-max`

## Display

- `display:type=sh1106|ssd1306`
- `display:addr=0x3C|0x3D`
- `display:test`
- `display:clear`
- `display:info`

## Clock

- `clock:` status
- `clock:on` / `clock:off`
- `clock:set=HH:MM[:SS]`
- `clock:sync=EPOCH_MS[,TZ]`
- `clock:fmt=12|24`

## System

- `restart` / `reboot`
- `help` / `?`

## OTA Notes

- OTA transport is BLE chunked full-image update (`.bin`)
- Credit notifications are used for throughput/backpressure
- Web OTA panel streams chunks and finalizes with DONE/ACK control sequence
