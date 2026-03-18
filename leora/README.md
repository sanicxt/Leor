# Leor Web Dashboard

SvelteKit dashboard for controlling the Leor ESP32 companion over Web Bluetooth.

## Features

- Connect to Leor from a BLE-supported browser
- Tune eye, breathing, gesture, and display settings
- Control the OLED clock mode
- Sync the clock from browser time
- Switch between 12-hour and 24-hour format
- Send OTA updates when supported by the firmware

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
- The repo root README contains the full firmware and hardware overview.
