/*
 * ble_manager.h - Robust BLE manager for leor using NimBLE
 * Replaces WiFi with low-power Bluetooth Low Energy
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"

// Implemented in commands.h
String handleCommand(String cmd);

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define COMMAND_CHAR_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_CHAR_UUID    "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define GESTURE_CHAR_UUID   "d1e5f0a1-2b3c-4d5e-6f7a-8b9c0d1e2f3a"

// ==================== State Variables ====================
static NimBLEServer* pServer = NULL;
static NimBLECharacteristic* pCommandChar = NULL;
static NimBLECharacteristic* pStatusChar = NULL;
static NimBLECharacteristic* pGestureChar = NULL;

// Connection state
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static uint32_t advRestartAtMs = 0;
static uint32_t advStartedAtMs = 0;

// Power saving state
static bool bleLowPowerMode = false;  // Low power mode flag

// Robustness tracking
static uint32_t lastActivityMs = 0;
static uint32_t lastNotifyMs = 0;
static uint32_t connectionCount = 0;
static uint32_t disconnectionCount = 0;
static uint16_t currentMTU = 23;  // Default BLE MTU

// Forward declarations
void sendBLEStatus(const String& status);
void sendBLEGesture(const String& gesture);

// ==================== Helper Functions ====================
static String bleTrimOneLine(String s) {
  s.replace("\r", "");
  s.trim();
  return s;
}

static String bleProcessPayload(const String& payload) {
  // Allow sending multiple commands at once separated by newlines or semicolons.
  String lastResponse;

  String work = payload;
  work.replace("\r", "");
  work.trim();
  if (work.length() == 0) return lastResponse;

  int start = 0;
  while (start < work.length()) {
    int nl = work.indexOf('\n', start);
    int sc = work.indexOf(';', start);
    int end = -1;
    if (nl == -1) end = sc;
    else if (sc == -1) end = nl;
    else end = (nl < sc) ? nl : sc;

    String cmd = (end == -1) ? work.substring(start) : work.substring(start, end);
    cmd = bleTrimOneLine(cmd);
    if (cmd.length() > 0) {
      lastResponse = handleCommand(cmd);
    }

    if (end == -1) break;
    start = end + 1;
  }

  return lastResponse;
}

// ==================== Notification Functions ====================

// Send a large string in chunks if it exceeds MTU
void sendBLEChunked(NimBLECharacteristic* pChar, const String& data) {
  if (!deviceConnected || !pChar) return;

  const int maxChunk = currentMTU - 3;
  if (data.length() <= maxChunk) {
    pChar->setValue(data.c_str());
    pChar->notify();
    return;
  }

  // Send in chunks
  int sent = 0;
  int total = data.length();
  
  // Optional: send a header or use a specific protocol
  // For simplicity, we just send raw chunks. The frontend should reassemble or handle partials.
  while (sent < total) {
    int take = min(maxChunk, total - sent);
    String chunk = data.substring(sent, sent + take);
    
    pChar->setValue(chunk.c_str());
    pChar->notify();
    
    sent += take;
    
    // Tiny delay to allow the stack to process the notification queue
    // but much smaller than the previous 20ms and non-blocking if possible
    // In NimBLE, we can check if the queue is full, but a 5ms delay is usually safe.
    if (sent < total) delay(5); 
  }
}

// Send status notification
void sendBLEStatus(const String& status) {
  sendBLEChunked(pStatusChar, status);
  lastActivityMs = millis();
}

// Send gesture notification
void sendBLEGesture(const String& gesture) {
  sendBLEChunked(pGestureChar, gesture);
  lastActivityMs = millis();
}

// ==================== BLE Callbacks ====================

// BLE Server Callbacks
class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
      deviceConnected = true;
      advRestartAtMs = 0;
      lastActivityMs = millis();
      connectionCount++;
      Serial.println(F("✓ BLE Client connected!"));
      
      // Update connection parameters based on power mode
      if (bleLowPowerMode) {
        // Low power: slower connection (saves battery, ~100-200ms)
        pServer->updateConnParams(connInfo.getConnHandle(), 80, 160, 0, 400);
      } else {
        // Performance: faster connection (lower latency, ~30-60ms)
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
      }
      
      sendBLEStatus("connected");
    };

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
      deviceConnected = false;
      disconnectionCount++;
      Serial.printf("✗ BLE Client disconnected - reason: %d\n", reason);
      // Restart advertising shortly after disconnect (non-blocking)
      advRestartAtMs = millis() + 250;
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) override {
      currentMTU = MTU;
      Serial.printf("MTU updated: %u\n", MTU);
    }
};

// Command Characteristic Callback
class CommandCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo& connInfo) override {
      if (!pCharacteristic) return;
      
      String payload = String(pCharacteristic->getValue().c_str());
      payload = bleTrimOneLine(payload);
      if (payload.length() == 0) return;

      lastActivityMs = millis();
      Serial.print(F("[BLE] RX: "));
      Serial.println(payload);

      String response = bleProcessPayload(payload);
      if (response.length() > 0) {
        // Send the entire response, letting chunking handle MTU limits
        sendBLEStatus(response);
      }
    }
};

// ==================== Public API ====================

// Check if BLE client is connected
bool isBLEConnected() {
  return deviceConnected;
}

// Get BLE status info string
String getBLEStatusInfo() {
  String info = "BLE Status:\n";
  info += "  Connected: " + String(deviceConnected ? "Yes" : "No") + "\n";
  info += "  MTU: " + String(currentMTU) + "\n";
  info += "  Connections: " + String(connectionCount) + "\n";
  info += "  Disconnects: " + String(disconnectionCount) + "\n";
  if (deviceConnected) {
    uint32_t connectedSecs = (millis() - (lastActivityMs > 0 ? lastActivityMs : millis())) / 1000;
    info += "  Activity: " + String(connectedSecs) + "s ago\n";
  }
  info += "  Power Mode: " + String(bleLowPowerMode ? "Low" : "High");
  return info;
}

// ==================== Power Management ====================

// Set BLE low power mode
void setBLELowPowerMode(bool enabled) {
  bleLowPowerMode = enabled;
  
  if (enabled) {
    // Low power: reduce TX power to -3dBm
    NimBLEDevice::setPower(BLE_TX_POWER_LOW);
    Serial.println(F("[BLE] Low power mode ON (-3dBm)"));
  } else {
    // High power: increase TX power to +9dBm
    NimBLEDevice::setPower(BLE_TX_POWER_HIGH);
    Serial.println(F("[BLE] Low power mode OFF (+9dBm)"));
  }
  
  // Update advertising interval based on power mode
  NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
  if (pAdv) {
    pAdv->stop();
    if (enabled) {
      pAdv->setMinInterval(BLE_ADV_INTERVAL_SLOW * 1.6);  // Convert ms to 0.625ms units
      pAdv->setMaxInterval(BLE_ADV_INTERVAL_SLOW * 1.6 * 2);
    } else {
      pAdv->setMinInterval(BLE_ADV_INTERVAL_FAST * 1.6);
      pAdv->setMaxInterval(BLE_ADV_INTERVAL_FAST * 1.6 * 2);
    }
    if (!deviceConnected) {
      pAdv->start();
    }
  }
}

// Get BLE low power mode status
bool getBLELowPowerMode() {
  return bleLowPowerMode;
}

// Restart BLE stack (for recovery)
void restartBLE() {
  Serial.println(F("[BLE] Restarting advertising..."));
  NimBLEDevice::getAdvertising()->stop();
  delay(100);
  if (NimBLEDevice::getAdvertising()->start()) {
    advStartedAtMs = millis();
  }
}

// Initialize BLE
void initBLE(const char* deviceName) {
  Serial.println(F("Initializing NimBLE..."));
  
  NimBLEDevice::init(deviceName);
  
  pServer = NimBLEDevice::createServer();
  if (!pServer) {
    Serial.println(F("✗ Failed to create BLE server!"));
    return;
  }
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  if (!pService) {
    Serial.println(F("✗ Failed to create BLE service!"));
    return;
  }

  pCommandChar = pService->createCharacteristic(
                      COMMAND_CHAR_UUID,
                      NIMBLE_PROPERTY::WRITE |
                      NIMBLE_PROPERTY::WRITE_NR
                    );
  if (pCommandChar) {
    pCommandChar->setCallbacks(new CommandCallbacks());
  }

  pStatusChar = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      NIMBLE_PROPERTY::READ |
                      NIMBLE_PROPERTY::NOTIFY
                    );
  if (pStatusChar) {
    pStatusChar->setValue("ready");
  }

  pGestureChar = pService->createCharacteristic(
                      GESTURE_CHAR_UUID,
                      NIMBLE_PROPERTY::READ |
                      NIMBLE_PROPERTY::NOTIFY
                    );
  if (pGestureChar) {
    pGestureChar->setValue("idle");
  }

  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(deviceName);
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  
  pAdvertising->start();
  advStartedAtMs = millis();
  
  Serial.println(F("✓ NimBLE initialized!"));
}

// Handle BLE connection state (call from loop)
void handleBLEConnection() {
  uint32_t now = millis();
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }

  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = false;
  }

  if (!deviceConnected && advRestartAtMs != 0 && (int32_t)(now - advRestartAtMs) >= 0) {
    advRestartAtMs = 0;
    if (NimBLEDevice::getAdvertising()->start()) {
      advStartedAtMs = now;
      Serial.println(F("[BLE] Advertising restarted"));
    }
  }
  
  if (deviceConnected && lastActivityMs > 0) {
    if ((now - lastActivityMs) > BLE_CONNECTION_TIMEOUT_MS) {
      Serial.println(F("[BLE] Connection timeout warning"));
      lastActivityMs = now;
    }
  }
  
  if (!deviceConnected && advStartedAtMs > 0) {
    if ((now - advStartedAtMs) > BLE_ADV_WATCHDOG_MS) {
      Serial.println(F("[BLE] Watchdog restart"));
      restartBLE();
    }
  }
}

#endif // BLE_MANAGER_H
