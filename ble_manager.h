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

// ==================== BLE Callbacks ====================

// BLE Server Callbacks
class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
      deviceConnected = true;
      advRestartAtMs = 0;
      lastActivityMs = millis();
      connectionCount++;
      Serial.println(F("✓ BLE Client connected!"));
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

      // Update activity timestamp
      lastActivityMs = millis();

      // Limit payload size to prevent memory issues
      const int kMaxPayload = min(240, (int)(currentMTU - 3));
      if (payload.length() > kMaxPayload) {
        payload = payload.substring(0, kMaxPayload);
      }

      Serial.print(F("[BLE] RX: "));
      Serial.println(payload);

      String response = bleProcessPayload(payload);
      if (response.length() > 0) {
        // Handle multi-line responses (split by \n)
        int start = 0;
        while (start < response.length()) {
           int nl = response.indexOf('\n', start);
           int end = (nl == -1) ? response.length() : nl;
           String chunk = response.substring(start, end);
           // bleTrimOneLine handles \r removal
           chunk.trim();
           if (chunk.length() > 0) {
             sendBLEStatus(chunk);
           }
           start = end + 1;
        }
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
    uint32_t connectedSecs = (millis() - lastActivityMs) / 1000;
    info += "  Idle: " + String(connectedSecs) + "s";
  }
  return info;
}

// Restart BLE stack (for recovery)
void restartBLE() {
  Serial.println(F("[BLE] Restarting BLE stack..."));
  
  // Stop advertising first
  NimBLEDevice::getAdvertising()->stop();
  
  delay(100);
  
  // Restart advertising
  if (NimBLEDevice::getAdvertising()->start()) {
    advStartedAtMs = millis();
    Serial.println(F("[BLE] Advertising restarted"));
  }
}

// Initialize BLE
void initBLE(const char* deviceName) {
  Serial.println(F("Initializing BLE (NimBLE)..."));
  
  // Create BLE Device
  NimBLEDevice::init(deviceName);
  
  // Create BLE Server
  pServer = NimBLEDevice::createServer();
  if (!pServer) {
    Serial.println(F("✗ Failed to create BLE server!"));
    return;
  }
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  if (!pService) {
    Serial.println(F("✗ Failed to create BLE service!"));
    return;
  }

  // Create Command Characteristic
  pCommandChar = pService->createCharacteristic(
                      COMMAND_CHAR_UUID,
                      NIMBLE_PROPERTY::WRITE |
                      NIMBLE_PROPERTY::WRITE_NR
                    );
  if (pCommandChar) {
    pCommandChar->setCallbacks(new CommandCallbacks());
  }

  // Create Status Characteristic (Read + Notify)
  pStatusChar = pService->createCharacteristic(
                      STATUS_CHAR_UUID,
                      NIMBLE_PROPERTY::READ |
                      NIMBLE_PROPERTY::NOTIFY
                    );
  if (pStatusChar) {
    pStatusChar->setValue("ready");
    // NimBLE handles 2902 automatically
  }

  // Create Gesture Characteristic (Read + Notify) 
  pGestureChar = pService->createCharacteristic(
                      GESTURE_CHAR_UUID,
                      NIMBLE_PROPERTY::READ |
                      NIMBLE_PROPERTY::NOTIFY
                    );
  if (pGestureChar) {
    pGestureChar->setValue("idle");
    // NimBLE handles 2902 automatically
  }

  // Start the service
  pService->start();

  // Configure and start advertising
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  
  // NimBLE simplifies connection parameters
  pAdvertising->start();
  advStartedAtMs = millis();
  
  Serial.println(F("✓ BLE initialized!"));
  Serial.print(F("  Device Name: "));
  Serial.println(deviceName);
  Serial.println(F("  Ready for connections"));
}

// Handle BLE connection state (call from loop)
void handleBLEConnection() {
  uint32_t now = millis();
  
  // Track connection state changes
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }

  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = false;
  }

  // Restart advertising after disconnect
  if (!deviceConnected && advRestartAtMs != 0 && (int32_t)(now - advRestartAtMs) >= 0) {
    advRestartAtMs = 0;
    if (NimBLEDevice::getAdvertising()->start()) {
      advStartedAtMs = now;
      Serial.println(F("[BLE] Advertising restarted"));
    }
  }
  
  // Connection timeout detection (client connected but went silent)
  if (deviceConnected && lastActivityMs > 0) {
    if ((now - lastActivityMs) > BLE_CONNECTION_TIMEOUT_MS) {
      Serial.println(F("[BLE] Connection timeout - client unresponsive"));
      lastActivityMs = now;  // Reset to avoid spam
    }
  }
  
  // Advertising watchdog - restart if no connection for too long
  if (!deviceConnected && advStartedAtMs > 0) {
    if ((now - advStartedAtMs) > BLE_ADV_WATCHDOG_MS) {
      Serial.println(F("[BLE] Advertising watchdog - restarting"));
      restartBLE();
    }
  }
}

// ==================== Notification Functions ====================

// Send status notification (rate-limited)
void sendBLEStatus(const String& status) {
  if (!deviceConnected || !pStatusChar) return;
  
  // Rate limiting to prevent flooding
  uint32_t now = millis();
  if ((now - lastNotifyMs) < BLE_NOTIFY_MIN_INTERVAL_MS) {
    delay(BLE_NOTIFY_MIN_INTERVAL_MS - (now - lastNotifyMs));
  }
  
  // Truncate to MTU
  String safeStatus = status;
  int maxLen = currentMTU - 3;
  if (safeStatus.length() > maxLen) {
    safeStatus = safeStatus.substring(0, maxLen);
  }
  
  pStatusChar->setValue(safeStatus.c_str());
  pStatusChar->notify();
  lastNotifyMs = millis();
  lastActivityMs = lastNotifyMs;
}

// Send gesture notification (rate-limited)
void sendBLEGesture(const String& gesture) {
  if (!deviceConnected || !pGestureChar) return;
  
  // Rate limiting
  uint32_t now = millis();
  if ((now - lastNotifyMs) < BLE_NOTIFY_MIN_INTERVAL_MS) {
    delay(BLE_NOTIFY_MIN_INTERVAL_MS - (now - lastNotifyMs));
  }
  
  // Truncate to MTU
  String safeGesture = gesture;
  int maxLen = currentMTU - 3;
  if (safeGesture.length() > maxLen) {
    safeGesture = safeGesture.substring(0, maxLen);
  }
  
  pGestureChar->setValue(safeGesture.c_str());
  pGestureChar->notify();
  lastNotifyMs = millis();
  lastActivityMs = lastNotifyMs;
}

#endif // BLE_MANAGER_H

