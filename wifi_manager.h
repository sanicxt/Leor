/*
 * wifi_manager.h - WiFi connection manager for Leora
 * Handles STA mode and AP fallback with persistent settings
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

// WiFi connection result
enum WifiStatus {
  WIFI_CONNECTED_STA,   // Connected to existing network
  WIFI_CONNECTED_AP,    // Running as Access Point
  WIFI_FAILED           // Failed to connect
};

// Stored WiFi credentials
String storedSsid;
String storedPass;
String storedApSsid;
String storedApPass;
bool forceApMode = false;

// Load WiFi settings from flash
void loadWifiSettings(const char* defaultSsid, const char* defaultPass,
                      const char* defaultApSsid, const char* defaultApPass) {
  Preferences prefs;
  prefs.begin("leora", true);  // Read-only
  
  storedSsid = prefs.getString("wifi_ssid", defaultSsid);
  storedPass = prefs.getString("wifi_pass", defaultPass);
  storedApSsid = prefs.getString("ap_ssid", defaultApSsid);
  storedApPass = prefs.getString("ap_pass", defaultApPass);
  forceApMode = prefs.getBool("force_ap", false);
  
  prefs.end();
  
  // Clear force_ap flag after reading (one-time use)
  if (forceApMode) {
    Preferences writePrefs;
    writePrefs.begin("leora", false);
    writePrefs.putBool("force_ap", false);
    writePrefs.end();
  }
  
  Serial.println(F("WiFi settings loaded from flash"));
  if (forceApMode) {
    Serial.println(F("Force AP mode enabled"));
  }
}

// Start Access Point
WifiStatus startAccessPoint() {
  Serial.println(F("Starting Access Point..."));
  
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  
  const char* apPass = storedApPass.length() >= 8 ? storedApPass.c_str() : NULL;
  bool apStarted = WiFi.softAP(storedApSsid.c_str(), apPass);
  
  if (apStarted) {
    delay(500);
    Serial.println(F("✓ Access Point Started!"));
    Serial.print(F("  SSID: "));
    Serial.println(storedApSsid);
    if (apPass) {
      Serial.print(F("  Password: "));
      Serial.println(storedApPass);
    } else {
      Serial.println(F("  (Open network)"));
    }
    Serial.print(F("  IP Address: "));
    Serial.println(WiFi.softAPIP());
    return WIFI_CONNECTED_AP;
  }
  
  Serial.println(F("✗ Failed to start Access Point!"));
  return WIFI_FAILED;
}

// Connect to WiFi with AP fallback
// Returns the connection status
WifiStatus connectWiFi(const char* ssid, const char* password, 
                       const char* ap_ssid, const char* ap_password,
                       int maxAttempts = 30) {
  
  // Load saved settings (may override defaults)
  loadWifiSettings(ssid, password, ap_ssid, ap_password);
  
  // Check if forced AP mode
  if (forceApMode) {
    return startAccessPoint();
  }
  
  Serial.println(F("\nConnecting to WiFi..."));
  Serial.print(F("SSID: "));
  Serial.println(storedSsid);

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);

#ifdef HOSTNAME
  WiFi.setHostname(HOSTNAME);
#endif
  
  WiFi.begin(storedSsid.c_str(), storedPass.c_str());
  // ESP32-C3 Super Mini has antenna flaw - reduce TX power
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n✓ WiFi connected!"));
    Serial.print(F("  IP Address: "));
    Serial.println(WiFi.localIP());
    return WIFI_CONNECTED_STA;
  }
  
  // WiFi failed, start Access Point
  Serial.println(F("\n✗ WiFi connection failed!"));
  return startAccessPoint();
}

// Print connection info
void printConnectionInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Web control: http://"));
    Serial.println(WiFi.localIP());
  } else if (WiFi.getMode() == WIFI_AP) {
    Serial.print(F("Connect to WiFi '"));
    Serial.print(storedApSsid);
    Serial.print(F("' then go to: http://"));
    Serial.println(WiFi.softAPIP());
  }
}

#endif // WIFI_MANAGER_H
