/*
 * ota_manager.h - OTA firmware update via BLE for leor (NimBLE-Arduino)
 *
 * Protocol (same as michaelangerer.dev/esp32/ble/ota):
 *   1. Client writes packet_size (2B LE) to OTA Data char
 *   2. Client writes REQUEST (0x01) to OTA Control char
 *   3. ESP32 notifies REQUEST_ACK (0x02) or REQUEST_NAK (0x03)
 *   4. Client streams .bin chunks to OTA Data char
 *   5. Client writes DONE (0x04) to OTA Control char
 *   6. ESP32 notifies DONE_ACK (0x05) or DONE_NAK (0x06), then reboots
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "esp_ota_ops.h"

// ==================== UUIDs ====================
#define OTA_SERVICE_UUID  "d6f1d96d-594c-4c53-b1c6-244a1dfde6d8"
#define OTA_CONTROL_UUID  "7ad671aa-21c0-46a4-b722-270e3ae3d830"
#define OTA_DATA_UUID     "23408888-1f40-4cd8-9b89-ca8d45f8a5b0"

// ==================== Control opcodes ====================
#define OTA_CTRL_NOP         0x00
#define OTA_CTRL_REQUEST     0x01
#define OTA_CTRL_REQUEST_ACK 0x02
#define OTA_CTRL_REQUEST_NAK 0x03
#define OTA_CTRL_DONE        0x04
#define OTA_CTRL_DONE_ACK    0x05
#define OTA_CTRL_DONE_NAK    0x06
// Credit-based flow control: device notifies this after every OTA_CREDIT_BATCH packets
// so the browser can send the next burst without per-packet round-trips.
#define OTA_CTRL_CREDIT      0x07
#define OTA_CREDIT_BATCH     32 // packets per credit window (larger = faster, fewer round-trips)
#define OTA_CREDIT_NOTIFY_RETRIES 3  // retry notify if NimBLE queue is busy

// ==================== State ====================
static NimBLECharacteristic* pOtaControlChar = nullptr;
static NimBLECharacteristic* pOtaDataChar    = nullptr;

static esp_ota_handle_t        otaHandle         = 0;
static const esp_partition_t*  otaPartition      = nullptr;
static bool                    otaInProgress     = false;
static uint16_t                otaPacketSize     = 0;
static uint32_t                otaPacketsRx      = 0;
static uint32_t                otaBytesRx        = 0;
static uint32_t                otaExpectedSize   = 0;    // partition size for progress calc
static uint32_t                otaShowErrorUntilMs = 0;  // keep error on OLED until this time
static uint32_t                otaRebootAtMs     = 0;    // deferred reboot timestamp

// ==================== Display callback ====================
// Set by main sketch so OTA can update OLED without coupling to display code.
typedef void (*OtaDisplayCallback)(int percent, const char* line1, const char* line2);
static OtaDisplayCallback otaDisplayCb = nullptr;

void setOtaDisplayCallback(OtaDisplayCallback cb) { otaDisplayCb = cb; }

static void otaUpdateDisplay(int pct, const char* l1, const char* l2) {
  if (otaDisplayCb) otaDisplayCb(pct, l1, l2);
}

// Query from main loop — when true, suspend eye rendering / I2C access.
// Stays true for a few seconds after OTA error so the error message stays visible.
// Also stays true while waiting for deferred reboot.
inline bool isOtaInProgress() {
  if (otaInProgress) return true;
  if (otaRebootAtMs > 0) return true;
  if (otaShowErrorUntilMs > 0) {
    if (millis() < otaShowErrorUntilMs) return true;
    otaShowErrorUntilMs = 0;
  }
  return false;
}

// Call from main loop — reboots the device after OTA success.
// The reboot is deferred so the BLE callback can return normally and
// NimBLE can send the ATT Write Response + DONE_ACK notification.
inline void otaCheckReboot() {
  if (otaRebootAtMs > 0 && millis() >= otaRebootAtMs) {
    esp_restart();
  }
}

// ==================== Helpers ====================

static void otaNotify(uint8_t opcode) {
  if (!pOtaControlChar) return;
  pOtaControlChar->setValue(&opcode, 1);
  pOtaControlChar->notify();
}

static void otaAbort() {
  if (otaHandle) {
    esp_ota_abort(otaHandle);
    otaHandle = 0;
  }
  otaInProgress = false;
  otaPartition = nullptr;
  otaPacketSize = 0;
  otaPacketsRx = 0;
  otaBytesRx   = 0;
  otaExpectedSize = 0;
}

// ==================== OTA Control characteristic callback ====================

class OtaControlCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    if (pChar->getLength() < 1) return;
    uint8_t opcode = pChar->getValue().data()[0];

    switch (opcode) {

      case OTA_CTRL_REQUEST: {
        Serial.println(F("[OTA] Request received"));

        if (otaInProgress) {
          // Abort previous attempt
          otaAbort();
        }

        otaPartition = esp_ota_get_next_update_partition(NULL);
        if (!otaPartition) {
          Serial.println(F("[OTA] ERROR: No OTA partition available"));
          otaUpdateDisplay(0, "OTA FAILED", "No partition!");
          otaNotify(OTA_CTRL_REQUEST_NAK);
          return;
        }

        esp_err_t err = esp_ota_begin(otaPartition, OTA_WITH_SEQUENTIAL_WRITES, &otaHandle);
        if (err != ESP_OK) {
          Serial.printf("[OTA] esp_ota_begin failed: %s\n", esp_err_to_name(err));
          otaUpdateDisplay(0, "OTA FAILED", "Begin error");
          otaNotify(OTA_CTRL_REQUEST_NAK);
          otaAbort();
          return;
        }

        otaInProgress  = true;
        otaPacketsRx   = 0;
        otaBytesRx     = 0;
        otaExpectedSize = otaPartition->size;  // for progress bar
        Serial.printf("[OTA] Started. Writing to partition at 0x%08X (%lu KB). Packet size: %u bytes\n",
                      otaPartition->address, (unsigned long)(otaExpectedSize / 1024), otaPacketSize);
        otaUpdateDisplay(0, NULL, "Waiting for data...");
        otaNotify(OTA_CTRL_REQUEST_ACK);
        break;
      }

      case OTA_CTRL_DONE: {
        Serial.printf("[OTA] Done signal. Total packets: %lu (%lu bytes)\n",
                      (unsigned long)otaPacketsRx, (unsigned long)otaBytesRx);

        if (!otaInProgress) {
          otaUpdateDisplay(0, "OTA FAILED", "Not started");
          otaNotify(OTA_CTRL_DONE_NAK);
          return;
        }

        // Keep otaInProgress = true so loop() stays suspended during verify/display
        otaUpdateDisplay(99, "OTA UPDATE", "Verifying...");

        esp_err_t err = esp_ota_end(otaHandle);
        otaHandle = 0;

        if (err != ESP_OK) {
          if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            Serial.println(F("[OTA] Validation failed — firmware image corrupted!"));
            otaUpdateDisplay(0, "OTA FAILED", "Bad image!");
          } else {
            Serial.printf("[OTA] esp_ota_end failed: %s\n", esp_err_to_name(err));
            otaUpdateDisplay(0, "OTA FAILED", "End error");
          }
          otaInProgress = false;  // release display ownership on error
          otaNotify(OTA_CTRL_DONE_NAK);
          otaAbort();
          return;
        }

        err = esp_ota_set_boot_partition(otaPartition);
        if (err != ESP_OK) {
          Serial.printf("[OTA] set_boot_partition failed: %s\n", esp_err_to_name(err));
          otaUpdateDisplay(0, "OTA FAILED", "Boot set error");
          otaNotify(OTA_CTRL_DONE_NAK);
          return;
        }

        otaPartition = nullptr;
        Serial.println(F("[OTA] Success! Rebooting in 1s..."));
        otaUpdateDisplay(100, "OTA SUCCESS", "Rebooting...");
        otaNotify(OTA_CTRL_DONE_ACK);
        // Defer reboot so this callback can return and NimBLE sends
        // the ATT Write Response + DONE_ACK notification to the browser.
        otaRebootAtMs = millis() + 1000;
        break;
      }

      default:
        break;
    }
  }
};

// ==================== OTA Data characteristic callback ====================

class OtaDataCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    // IMPORTANT: Keep the NimBLEAttValue alive for the entire callback scope.
    // getValue() may return a temporary; storing .data() from it would be a
    // dangling pointer once the temporary is destroyed at the semicolon.
    NimBLEAttValue val = pChar->getValue();
    size_t         dataLen = val.size();
    const uint8_t* data    = val.data();

    if (dataLen == 0) return;

    // First write before OTA starts = packet size (2 bytes, little-endian)
    if (!otaInProgress && dataLen == 2) {
      otaPacketSize = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
      Serial.printf("[OTA] Packet size negotiated: %u bytes\n", otaPacketSize);
      return;
    }

    // Binary payload during active OTA
    if (otaInProgress) {
      // Validate first packet: ESP32 app images must start with magic byte 0xE9
      if (otaPacketsRx == 0 && dataLen > 0 && data[0] != 0xE9) {
        Serial.printf("[OTA] ERROR: Invalid firmware file! First byte 0x%02X, expected 0xE9.\n", data[0]);
        Serial.println(F("[OTA] Make sure you upload the .bin file (not .elf, .merged.bin, or other formats)"));
        otaUpdateDisplay(0, "OTA FAILED", "Wrong file type!");
        otaNotify(OTA_CTRL_DONE_NAK);
        otaShowErrorUntilMs = millis() + 3000;  // keep error visible for 3s
        otaAbort();
        return;
      }

      esp_err_t err = esp_ota_write(otaHandle, (const void*)data, dataLen);
      if (err != ESP_OK) {
        Serial.printf("[OTA] esp_ota_write failed: %s — aborting.\n", esp_err_to_name(err));
        otaUpdateDisplay(0, "OTA FAILED", "Write error!");
        otaNotify(OTA_CTRL_DONE_NAK);
        otaShowErrorUntilMs = millis() + 3000;  // keep error visible for 3s
        otaAbort();
        return;
      }

      otaPacketsRx++;
      otaBytesRx += dataLen;

      // Credit-based flow control: every OTA_CREDIT_BATCH packets, notify the browser
      // it may send the next batch. Yield here (not per-write) so NimBLE can process.
      if (otaPacketsRx % OTA_CREDIT_BATCH == 0) {
        uint8_t credit = OTA_CTRL_CREDIT;
        pOtaControlChar->setValue(&credit, 1);
        // Retry notify — NimBLE can silently drop notifications when its
        // TX queue is contended, causing the browser to never receive the
        // credit and timeout.
        for (int i = 0; i < OTA_CREDIT_NOTIFY_RETRIES; i++) {
          if (pOtaControlChar->notify()) break;
          vTaskDelay(pdMS_TO_TICKS(2));
        }
      }

      // Update OLED every 32 packets (less frequent = less I2C blocking)
      if (otaPacketsRx % 32 == 0) {
        // Compute real progress percentage from partition size
        int pct = 0;
        if (otaExpectedSize > 0) {
          pct = (int)((uint64_t)otaBytesRx * 100 / otaExpectedSize);
          if (pct > 99) pct = 99;  // cap at 99 until DONE confirms
        }
        char msg[32];
        uint32_t kbDone = otaBytesRx / 1024;
        uint32_t kbTotal = otaExpectedSize / 1024;
        snprintf(msg, sizeof(msg), "%lu / %lu KB", (unsigned long)kbDone, (unsigned long)kbTotal);
        if (otaPacketsRx % 200 == 0) {
          Serial.printf("[OTA] %d%% — %lu / %lu KB\n", pct, (unsigned long)kbDone, (unsigned long)kbTotal);
        }
        otaUpdateDisplay(pct, NULL, msg);
      }
    }
  }
};

// ==================== Public init ====================

void initOTAService(NimBLEServer* pServer) {
  NimBLEService* pOtaService = pServer->createService(OTA_SERVICE_UUID);
  if (!pOtaService) {
    Serial.println(F("[OTA] Failed to create OTA service!"));
    return;
  }

  pOtaControlChar = pOtaService->createCharacteristic(
    OTA_CONTROL_UUID,
    NIMBLE_PROPERTY::READ   |
    NIMBLE_PROPERTY::WRITE  |
    NIMBLE_PROPERTY::NOTIFY
  );
  if (pOtaControlChar) {
    uint8_t nop = OTA_CTRL_NOP;
    pOtaControlChar->setValue(&nop, 1);
    pOtaControlChar->setCallbacks(new OtaControlCallbacks());
  }

  pOtaDataChar = pOtaService->createCharacteristic(
    OTA_DATA_UUID,
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::WRITE_NR
  );
  if (pOtaDataChar) {
    pOtaDataChar->setCallbacks(new OtaDataCallbacks());
  }

  pOtaService->start();
  Serial.println(F("✓ OTA BLE service ready"));
}

// ==================== OTA boot verification ====================
// Call once at the very start of setup() to confirm a successful OTA boot.
// If the new image doesn't call this the bootloader will roll back automatically.
void otaVerifyBootAndMark() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
      // We booted successfully — mark the image as valid.
      Serial.println(F("[OTA] New image booted — marking valid."));
      esp_ota_mark_app_valid_cancel_rollback();
    }
  }
}

#endif // OTA_MANAGER_H
