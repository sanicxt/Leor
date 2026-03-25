# Leor Architecture (Current)

This document describes the current architecture after migrating fully to ESP-IDF at repository root.

## Firmware Stack

- Platform: ESP-IDF v6
- Target: ESP32-C3 (primary)
- Language: C++17
- Build system: CMake + idf.py

## Firmware Structure

```
components/leor_core/
├── include/leor/
│   ├── application.hpp
│   ├── ble_service.hpp
│   ├── clock_service.hpp
│   ├── command_router.hpp
│   ├── config.hpp
│   ├── display_backend.hpp
│   ├── gesture_service.hpp
│   ├── mochi_eyes_engine.hpp
│   ├── ota_service.hpp
│   ├── power_service.hpp
│   └── ...
└── src/
    ├── application.cpp
    ├── ble_service.cpp
    ├── clock_service.cpp
    ├── command_router.cpp
    ├── display_backend.cpp
    ├── gesture_service.cpp
    ├── mochi_eyes_engine.cpp
    ├── ota_service.cpp
    ├── power_service.cpp
    └── ...
```

## Core Services

- `Application`: boot orchestration, runtime tick, mode switching
- `DisplayBackend`: U8g2-backed OLED abstraction (128x64)
- `MochiEyesEngine`: procedural face rendering and animation system
- `ClockService`: stable centered clock rendering with blink-safe colon
- `GestureService`: IMU gesture polling/inference integration
- `BleService`: NimBLE GATT, command bridge, status/gesture notifications
- `OtaService`: BLE OTA full-image write/finalize flow
- `PowerService`: touch-hold sleep entry and wake handling
- `CommandRouter`: command parsing + settings persistence

## BLE Topology

- Main service: commands + status + gesture notify
- OTA service:
  - control characteristic (request/done/credit/ack)
  - data characteristic (chunk stream)

## OTA Design

- Mode: full firmware image (`.bin`), not delta patch currently
- Data plane: chunked `writeValueWithoutResponse`
- Flow control: device credit notifications every batch
- Control plane: REQUEST -> ACK, DONE -> ACK/NAK
- Progress: bytes + packet counters, with metadata hint from web sender

## Display / UI Runtime

- Screen: 128x64 OLED
- Face mode uses optimized dirty-region updates
- Application forces full clear on face/clock mode transitions to avoid artifacts
- Clock layout anchored to center to avoid horizontal jitter while colon blinks

## Web Dashboard

- Path: `web/`
- SvelteKit + Web Bluetooth
- Controls: mood/action/gaze/settings/clock/OTA
- OTA upload panel currently expects `.bin` firmware image

## Build + Flash

```bash
idf.py build
idf.py flash
```
