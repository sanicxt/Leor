/*
 * commands.h - Command handler for Leora
 * Handles serial and web commands for expressions, mouth, actions, etc.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "RoboEyes/src/FluxGarage_RoboEyes.h"
#include "gesture_trainer.h"

// Forward declaration - will be set in main sketch
extern RoboEyes<Adafruit_SH1106G>* pRoboEyes;
// Tuning/logging options set in main sketch
extern bool tofVerbose;
extern bool gestVerbose;

// Preferences for WiFi storage
Preferences wifiPrefs;

// Reset all effects to default
void resetEffects() {
  pRoboEyes->setCuriosity(OFF);
  pRoboEyes->setHFlicker(OFF);
  pRoboEyes->setVFlicker(OFF);
  pRoboEyes->setSweat(OFF);
  pRoboEyes->setIdleMode(OFF);
  pRoboEyes->setEyebrows(false);
  pRoboEyes->setKnocked(OFF);
}

// Print help menu to Serial
void printHelp() {
  Serial.println(F("\n=== Leora Serial Commands ==="));
  Serial.println(F("EXPRESSIONS:"));
  Serial.println(F("  happy, sad, angry, love, surprised"));
  Serial.println(F("  confused, sleepy, curious, nervous"));
  Serial.println(F("  knocked, neutral, idle, raised"));
  Serial.println(F("\nMOUTH:"));
  Serial.println(F("  smile, frown, open, ooo, flat"));
  Serial.println(F("  talk, chew, wobble"));
  Serial.println(F("\nACTIONS:"));
  Serial.println(F("  blink, wink, winkr, laugh, cry"));
  Serial.println(F("\nPOSITIONS:"));
  Serial.println(F("  center, n, ne, e, se, s, sw, w, nw"));
  Serial.println(F("\nTOGGLES:"));
  Serial.println(F("  sweat, cyclops"));
  Serial.println(F("  toflog - toggle raw TOF debug output"));
  Serial.println(F("  gestlog - toggle gesture processing debug output"));
  Serial.println(F("================================\n"));
}

// Handle a command string, returns response for web UI
String handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  
  if (cmd.length() == 0) return "Empty command";
  
  Serial.print(F("> "));
  Serial.println(cmd);

  // ==================== EXPRESSIONS ====================
  if (cmd == "happy") {
    resetEffects();
    pRoboEyes->setMood(HAPPY);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->anim_laugh();
    pRoboEyes->setMouthType(1);
    Serial.println(F("Expression: Happy"));
  }
  else if (cmd == "sad") {
    resetEffects();
    pRoboEyes->setMood(TIRED);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->setMouthType(2);
    Serial.println(F("Expression: Sad"));
  }
  else if (cmd == "angry") {
    resetEffects();
    pRoboEyes->setMood(ANGRY);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->setMouthType(5);
    Serial.println(F("Expression: Angry"));
  }
  else if (cmd == "love") {
    resetEffects();
    pRoboEyes->anim_love();
    pRoboEyes->setMood(HAPPY);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->setMouthType(3);
    Serial.println(F("Expression: Love"));
  }
  else if (cmd == "surprised") {
    resetEffects();
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->setCuriosity(ON);
    pRoboEyes->setPosition(N);
    pRoboEyes->blink();
    pRoboEyes->setMouthType(3);
    Serial.println(F("Expression: Surprised"));
  }
  else if (cmd == "confused") {
    resetEffects();
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->anim_confused();
    pRoboEyes->setMouthType(4);
    Serial.println(F("Expression: Confused"));
  }
  else if (cmd == "sleepy") {
    resetEffects();
    pRoboEyes->setMood(TIRED);
    pRoboEyes->setPosition(SW);
    pRoboEyes->setMouthType(5);
    Serial.println(F("Expression: Sleepy"));
  }
  else if (cmd == "curious") {
    resetEffects();
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->setCuriosity(ON);
    pRoboEyes->setPosition(E);
    pRoboEyes->setMouthType(4);
    Serial.println(F("Expression: Curious"));
  }
  else if (cmd == "nervous") {
    resetEffects();
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->setSweat(ON);
    pRoboEyes->setCuriosity(ON);
    pRoboEyes->setPosition(N);  // Use N instead of NW - left eye gets bigger with curious mode
    pRoboEyes->setMouthType(2);
    Serial.println(F("Expression: Nervous"));
  }
  else if (cmd == "knocked" || cmd == "dizzy") {
    resetEffects();
    pRoboEyes->setKnocked(ON);
    Serial.println(F("Expression: Knocked"));
  }
  else if (cmd == "neutral" || cmd == "normal" || cmd == "reset") {
    resetEffects();
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->setMouthType(1);
    Serial.println(F("Expression: Neutral"));
  }
  else if (cmd == "idle") {
    resetEffects();
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->setIdleMode(ON, 1, 2);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->setMouthType(1);
    Serial.println(F("Mode: Idle"));
  }
  else if (cmd == "raised") {
    resetEffects();
    pRoboEyes->setEyebrows(true);
    pRoboEyes->setMood(DEFAULT);
    pRoboEyes->setPosition(DEFAULT);
    pRoboEyes->setMouthType(4);
    Serial.println(F("Expression: Raised eyebrows"));
  }

  // ==================== MOUTH ====================
  else if (cmd == "smile") {
    pRoboEyes->setMouthType(1);
    Serial.println(F("Mouth: Smile"));
  }
  else if (cmd == "frown") {
    pRoboEyes->setMouthType(2);
    Serial.println(F("Mouth: Frown"));
  }
  else if (cmd == "open") {
    pRoboEyes->setMouthType(3);
    Serial.println(F("Mouth: Open"));
  }
  else if (cmd == "ooo") {
    pRoboEyes->setMouthType(4);
    Serial.println(F("Mouth: Ooo"));
  }
  else if (cmd == "flat") {
    pRoboEyes->setMouthType(5);
    Serial.println(F("Mouth: Flat"));
  }
  else if (cmd == "talk") {
    pRoboEyes->startMouthAnim(1, 3000);
    Serial.println(F("Mouth: Talking"));
  }
  else if (cmd == "chew") {
    pRoboEyes->startMouthAnim(2, 2000);
    Serial.println(F("Mouth: Chewing"));
  }
  else if (cmd == "wobble") {
    pRoboEyes->startMouthAnim(3, 2000);
    Serial.println(F("Mouth: Wobbling"));
  }

  // ==================== ACTIONS ====================
  else if (cmd == "blink") {
    pRoboEyes->blink();
    Serial.println(F("Action: Blink"));
  }
  else if (cmd == "wink") {
    pRoboEyes->wink(true);
    pRoboEyes->setMouthType(1);
    Serial.println(F("Action: Wink"));
  }
  else if (cmd == "winkr") {
    pRoboEyes->wink(false);
    pRoboEyes->setMouthType(1);
    Serial.println(F("Action: Wink Right"));
  }
  else if (cmd == "laugh") {
    pRoboEyes->anim_laugh();
    Serial.println(F("Action: Laugh"));
  }
  else if (cmd == "cry") {
    pRoboEyes->anim_cry();
    Serial.println(F("Action: Cry"));
  }
  // knocked moved to expressions

  // ==================== POSITIONS ====================
  else if (cmd == "center") {
    pRoboEyes->setPosition(DEFAULT);
    Serial.println(F("Position: Center"));
  }
  else if (cmd == "n" || cmd == "up") {
    pRoboEyes->setPosition(N);
    Serial.println(F("Position: North"));
  }
  else if (cmd == "ne") {
    pRoboEyes->setPosition(NE);
    Serial.println(F("Position: North-East"));
  }
  else if (cmd == "e" || cmd == "right") {
    pRoboEyes->setPosition(E);
    Serial.println(F("Position: East"));
  }
  else if (cmd == "se") {
    pRoboEyes->setPosition(SE);
    Serial.println(F("Position: South-East"));
  }
  else if (cmd == "s" || cmd == "down") {
    pRoboEyes->setPosition(S);
    Serial.println(F("Position: South"));
  }
  else if (cmd == "sw") {
    pRoboEyes->setPosition(SW);
    Serial.println(F("Position: South-West"));
  }
  else if (cmd == "w" || cmd == "left") {
    pRoboEyes->setPosition(W);
    Serial.println(F("Position: West"));
  }
  else if (cmd == "nw") {
    pRoboEyes->setPosition(NW);
    Serial.println(F("Position: North-West"));
  }

  // ==================== TOGGLES ====================
  else if (cmd == "sweat") {
    static bool sweatOn = false;
    sweatOn = !sweatOn;
    pRoboEyes->setSweat(sweatOn);
    Serial.print(F("Sweat: "));
    Serial.println(sweatOn ? F("ON") : F("OFF"));
  }
  else if (cmd == "cyclops") {
    static bool cyclopsOn = false;
    cyclopsOn = !cyclopsOn;
    pRoboEyes->setCyclops(cyclopsOn);
    Serial.print(F("Cyclops: "));
    Serial.println(cyclopsOn ? F("ON") : F("OFF"));
  }
  else if (cmd == "mouth") {
    static bool mouthOn = true;
    mouthOn = !mouthOn;
    pRoboEyes->setMouthEnabled(mouthOn);
    Serial.print(F("Mouth: "));
    Serial.println(mouthOn ? F("ON") : F("OFF"));
  }

  // ==================== LOW-LEVEL DEBUG ====================
  else if (cmd == "toflog") {
    tofVerbose = !tofVerbose;
    Serial.print(F("TOF verbose logging: "));
    Serial.println(tofVerbose ? F("ON") : F("OFF"));
    return String(tofVerbose ? "TOF verbose ON" : "TOF verbose OFF");
  }
  else if (cmd == "gestlog") {
    gestVerbose = !gestVerbose;
    Serial.print(F("Gesture verbose logging: "));
    Serial.println(gestVerbose ? F("ON") : F("OFF"));
    return String(gestVerbose ? "Gesture verbose ON" : "Gesture verbose OFF");
  }

  // ==================== SETTINGS ====================
  else if (cmd.startsWith("set:")) {
    // Parse settings string: set:ew=36,eh=36,es=10,er=8,mw=20,lt=1000,vt=2000,bi=3
    String params = cmd.substring(4);
    
    int idx = 0;
    while (idx < params.length()) {
      int eqPos = params.indexOf('=', idx);
      int commaPos = params.indexOf(',', idx);
      if (commaPos == -1) commaPos = params.length();
      
      if (eqPos > idx && eqPos < commaPos) {
        String key = params.substring(idx, eqPos);
        int value = params.substring(eqPos + 1, commaPos).toInt();
        
        if (key == "ew") {
          pRoboEyes->setWidth(value, value);
        } else if (key == "eh") {
          pRoboEyes->setHeight(value, value);
        } else if (key == "es") {
          pRoboEyes->setSpacebetween(value);
        } else if (key == "er") {
          pRoboEyes->setBorderradius(value, value);
        } else if (key == "mw") {
          pRoboEyes->setMouthSize(value, 6);
        } else if (key == "lt") {
          pRoboEyes->setLaughDuration(value);
        } else if (key == "vt") {
          pRoboEyes->setLoveDuration(value);
        } else if (key == "bi") {
          pRoboEyes->setAutoblinker(true, value, 3);
        }
      }
      idx = commaPos + 1;
    }
    Serial.println(F("Settings applied"));
    return "Settings applied";
  }

  // ==================== WIFI SETTINGS ====================
  else if (cmd.startsWith("wifi:")) {
    String params = cmd.substring(5);
    
    if (params == "info") {
      // Return WiFi info
      String info = "Mode: ";
      if (WiFi.getMode() == WIFI_AP) {
        info += "Access Point\n";
        info += "SSID: " + String(WiFi.softAPSSID()) + "\n";
        info += "IP: " + WiFi.softAPIP().toString() + "\n";
        info += "Clients: " + String(WiFi.softAPgetStationNum());
      } else {
        info += "Station\n";
        info += "SSID: " + WiFi.SSID() + "\n";
        info += "IP: " + WiFi.localIP().toString() + "\n";
        info += "RSSI: " + String(WiFi.RSSI()) + " dBm";
      }
      Serial.println(info);
      return info;
    }
    else if (params == "force_ap") {
      // Set flag to force AP mode on next boot
      wifiPrefs.begin("leora", false);
      wifiPrefs.putBool("force_ap", true);
      wifiPrefs.end();
      Serial.println(F("Rebooting to AP mode..."));
      delay(500);
      ESP.restart();
    }
    else if (params == "reset") {
      // Clear all saved WiFi settings, revert to defaults
      wifiPrefs.begin("leora", false);
      wifiPrefs.clear();
      wifiPrefs.end();
      Serial.println(F("WiFi settings reset to defaults. Rebooting..."));
      delay(500);
      ESP.restart();
    }
    else {
      // Parse wifi settings: wifi:ssid=xxx,pass=xxx or wifi:ap_ssid=xxx,ap_pass=xxx
      wifiPrefs.begin("leora", false);
      
      int idx = 0;
      bool hasStaCreds = false;
      bool hasApCreds = false;
      String newSsid, newPass, newApSsid, newApPass;
      
      while (idx < params.length()) {
        int eqPos = params.indexOf('=', idx);
        int commaPos = params.indexOf(',', idx);
        if (commaPos == -1) commaPos = params.length();
        
        if (eqPos > idx && eqPos < commaPos) {
          String key = params.substring(idx, eqPos);
          String value = params.substring(eqPos + 1, commaPos);
          
          if (key == "ssid") {
            newSsid = value;
            wifiPrefs.putString("wifi_ssid", value);
            hasStaCreds = true;
          } else if (key == "pass") {
            newPass = value;
            wifiPrefs.putString("wifi_pass", value);
          } else if (key == "ap_ssid") {
            newApSsid = value;
            wifiPrefs.putString("ap_ssid", value);
            hasApCreds = true;
          } else if (key == "ap_pass") {
            newApPass = value;
            wifiPrefs.putString("ap_pass", value);
          }
        }
        idx = commaPos + 1;
      }
      
      // If AP creds set, force AP mode
      if (hasApCreds) {
        wifiPrefs.putBool("force_ap", true);
      } else {
        wifiPrefs.putBool("force_ap", false);
      }
      
      wifiPrefs.end();
      Serial.println(F("WiFi settings saved. Rebooting..."));
      delay(500);
      ESP.restart();
      return "Rebooting...";  // Never reached
    }
    return "WiFi command processed";
  }

  // ==================== GESTURE TRAINING ====================
  else if (cmd.startsWith("gesture:")) {
    String params = cmd.substring(8);
    
    if (params.startsWith("train=")) {
      // Start training: gesture:train=name,action
      String trainParams = params.substring(6);
      int commaPos = trainParams.indexOf(',');
      if (commaPos > 0) {
        String name = trainParams.substring(0, commaPos);
        String action = trainParams.substring(commaPos + 1);
        if (startGestureRecording(name, action)) {
          return "Training started: " + name;
        } else {
          return "Failed to start training";
        }
      }
      return "Invalid format. Use: gesture:train=name,action";
    }
    else if (params == "cancel") {
      cancelGestureRecording();
      return "Training cancelled";
    }
    else if (params == "status") {
      // Return training status for web UI polling
      String state;
      switch (trainState) {
        case TRAIN_WAITING: state = "waiting"; break;
        case TRAIN_COUNTDOWN: state = "countdown," + String(countdownNum); break;
        case TRAIN_RECORDING: state = "recording," + String(getTrainingProgress()); break;
        case TRAIN_DONE: state = "done"; break;
        default: state = "idle"; break;
      }
      return state;
    }
    else if (params == "list") {
      return listGestures();
    }
    else if (params.startsWith("delete=")) {
      String name = params.substring(7);
      if (deleteGesture(name)) {
        return "Deleted: " + name;
      }
      return "Gesture not found: " + name;
    }
    else if (params == "clear") {
      clearAllGestures();
      return "All gestures cleared";
    }
    else if (params.startsWith("match=")) {
      bool enable = params.substring(6) == "1";
      setMatchingEnabled(enable);
      return String("Matching: ") + (enable ? "ON" : "OFF");
    }
    return "Unknown gesture command";
  }

  // ==================== HELP ====================
  else if (cmd == "help" || cmd == "?") {
    printHelp();
    return "Help printed to serial";
  }
  else {
    Serial.print(F("Unknown: "));
    Serial.println(cmd);
    return "Unknown: " + cmd;
  }
  
  return "OK: " + cmd;
}

#endif // COMMANDS_H
