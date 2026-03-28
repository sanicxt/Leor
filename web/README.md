# Leor Web Dashboard

SvelteKit dashboard for controlling the Leor ESP32 companion over Web Bluetooth.

## Features

- Connect to Leor from a BLE-supported browser
- Tune eye, breathing, gesture, and display settings
- Control the OLED clock mode
- Sync the clock from browser time
- Switch between 12-hour and 24-hour format
- Send BLE OTA full-image firmware updates (`.bin`)

## Development

```sh
bun install
bun run dev
```

Open the local app in Chrome, Edge, or another browser with Web Bluetooth support.

## Build

```sh
bun run build
```

## Pages build

```sh
bun run build:pages
```

## Notes

- The dashboard talks to the firmware through the BLE command API in [`../API.md`](../API.md).
- Clock sync uses browser time and timezone offset, and the firmware persists the synced epoch.
- BLE is only advertised in a configurable active window after boot or touch wake.
- OTA flow is chunked + credit-based for faster transfer while preserving reliability.
- The repo root README contains the current ESP-IDF firmware/hardware overview.
