/*
 * commands.h - Command handler for leor
 * Handles serial and web commands for expressions, mouth, actions, etc.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>
#include <Preferences.h>
#include "RoboEyes/src/FluxGarage_RoboEyes.h"
#include "gesture_trainer.h"

// Forward declaration - will be set in main sketch
extern RoboEyes<Adafruit_SH1106G>* pRoboEyes;
extern Preferences preferences;

// MPU6050 debug logging set in main sketch
extern bool mpuVerbose;

// Random expression shuffle state set in main sketch
extern bool shuffleEnabled;
extern uint32_t shuffleExprMinMs;
extern uint32_t shuffleExprMaxMs;
extern uint32_t shuffleNeutralMinMs;
extern uint32_t shuffleNeutralMaxMs;
extern bool shuffleNeedsInit;




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
  Serial.println(F("\n=== leor Serial Commands ==="));
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
  Serial.println(F("  mpulog - toggle MPU6050 debug output"));
  Serial.println(F("================================\n"));
}

// Handle a command string, returns response for web UI
String handleCommand(String cmd) {
  cmd.trim();
  
  if (cmd.length() == 0) return "Empty command";
  
  // IMPORTANT: Handle weight transfer commands BEFORE toLowerCase
  // Base64 is case-sensitive!
  if (cmd.startsWith("gw+")) {
    String chunk = cmd.substring(3);
    appendWeightChunk(chunk);
    Serial.print(F("> gw+ chunk: ")); Serial.println(chunk.length());
    return "gw+ok";
  }
  if (cmd.startsWith("gw=")) {
    String base64data = cmd.substring(3);
    Serial.print(F("> gw= data: ")); Serial.println(base64data.length());
    if (loadWeightsFromBase64(base64data)) {
      return "gw:ok";
    } else {
      return "gw:err";
    }
  }
  
  // Lowercase for all other commands
  cmd.toLowerCase();
  
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
  else if (cmd == "mpulog") {
    mpuVerbose = !mpuVerbose;
    Serial.print(F("MPU6050 verbose logging: "));
    Serial.println(mpuVerbose ? F("ON") : F("OFF"));
    return String(mpuVerbose ? "MPU verbose ON" : "MPU verbose OFF");
  }


  // ==================== SETTINGS (short: s:, long: set:) ====================
  else if (cmd.startsWith("s:") || cmd.startsWith("set:")) {
    // Parse settings string: s:ew=36,eh=36 or set:ew=36,eh=36
    int prefixLen = cmd.startsWith("s:") ? 2 : 4;
    String params = cmd.substring(prefixLen);
    
    // If empty params, return current settings string for sync
    if (params.length() == 0) {
      String current = "";
      current += "ew=" + String(preferences.getInt("ew", 36)) + ",";
      current += "eh=" + String(preferences.getInt("eh", 36)) + "\n";
      
      current += "es=" + String(preferences.getInt("es", 10)) + ",";
      current += "er=" + String(preferences.getInt("er", 8)) + "\n";
      
      current += "mw=" + String(preferences.getInt("mw", 20)) + ",";
      current += "lt=" + String(preferences.getInt("lt", 1000)) + "\n";
      
      current += "vt=" + String(preferences.getInt("vt", 2000)) + ",";
      current += "bi=" + String(preferences.getInt("bi", 3));
      return current;
    }
    
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
          preferences.putInt("ew", value);
        } else if (key == "eh") {
          pRoboEyes->setHeight(value, value);
          preferences.putInt("eh", value);
        } else if (key == "es") {
          pRoboEyes->setSpacebetween(value);
          preferences.putInt("es", value);
        } else if (key == "er") {
          pRoboEyes->setBorderradius(value, value);
          preferences.putInt("er", value);
        } else if (key == "mw") {
          pRoboEyes->setMouthSize(value, 6);
          preferences.putInt("mw", value);
        } else if (key == "lt") {
          pRoboEyes->setLaughDuration(value);
          preferences.putInt("lt", value);
        } else if (key == "vt") {
          pRoboEyes->setLoveDuration(value);
          preferences.putInt("vt", value);
        } else if (key == "bi") {
          pRoboEyes->setAutoblinker(true, value, 3);
          preferences.putInt("bi", value);
        }
      }
      idx = commaPos + 1;
    }
    Serial.println(F("Settings applied & saved"));
    return "Settings applied & saved";
  }

  // ==================== GESTURE COMMANDS (streaming + ML) ====================
  // gs = start streaming gyro data to browser
  else if (cmd == "gs") {
    startStreaming();
    return "gs:1";
  }
  // gx = stop streaming
  else if (cmd == "gx") {
    stopStreaming();
    return "gs:0";
  }
  // gw! = finalize chunked weight transfer
  else if (cmd == "gw!") {
    if (finalizeWeights()) {
      return "gw:ok";
    } else {
      return "gw:err";
    }
  }
  // gl=index:name:action = set gesture label
  else if (cmd.startsWith("gl=")) {
    String params = cmd.substring(3);
    int sep1 = params.indexOf(':');
    int sep2 = params.indexOf(':', sep1 + 1);
    if (sep1 > 0 && sep2 > sep1) {
      int index = params.substring(0, sep1).toInt();
      String name = params.substring(sep1 + 1, sep2);
      String action = params.substring(sep2 + 1);
      setGestureLabel(index, name.c_str(), action.c_str());
      return "gl:ok";
    }
    return "gl:err";
  }
  // gm=1 or gm=0 = enable/disable matching
  else if (cmd.startsWith("gm=")) {
    String val = cmd.substring(3);
    val.trim();
    bool enable = (val == "1");
    setMatchingEnabled(enable);
    return enable ? "gm:1" : "gm:0";
  }
  // gc = clear all gestures
  else if (cmd == "gc") {
    clearAllGestures();
    return "gc:ok";
  }
  // gi = get gesture info
  else if (cmd == "gi") {
    return listGestures();
  }

  // ==================== BLE COMMANDS ====================
  // ... (omitted for brevity, assume matches original) ...

  // ==================== EXPRESSION SHUFFLE (short: sh:, long: shuffle:) ====================
  else if (cmd.startsWith("sh:") || cmd.startsWith("shuffle:")) {
    int prefixLen = cmd.startsWith("sh:") ? 3 : 8;
    String params = cmd.substring(prefixLen);

    if (params.length() == 0) {
      String resp = String("Shuffle: ") + (shuffleEnabled ? "ON" : "OFF");
      resp += "\nexpr=" + String(shuffleExprMinMs / 1000) + "-" + String(shuffleExprMaxMs / 1000) + "s";
      resp += "\nneutral=" + String(shuffleNeutralMinMs / 1000) + "-" + String(shuffleNeutralMaxMs / 1000) + "s";
      return resp;
    }

    bool turnedOn = false;
    bool changed = false;

    int idx = 0;
    while (idx < params.length()) {
      int commaPos = params.indexOf(',', idx);
      if (commaPos == -1) commaPos = params.length();
      String token = params.substring(idx, commaPos);
      token.trim();

      if (token == "on" || token == "1") {
        shuffleEnabled = true;
        turnedOn = true;
        changed = true;
      } else if (token == "off" || token == "0") {
        shuffleEnabled = false;
        changed = true;
      } else if (token.startsWith("expr=") || token.startsWith("e=")) {
        // expr=2-5 or expr=3 (fixed duration)
        int eqPos = token.indexOf('=');
        String val = token.substring(eqPos + 1);
        int dashPos = val.indexOf('-');
        if (dashPos > 0) {
          long minS = val.substring(0, dashPos).toInt();
          long maxS = val.substring(dashPos + 1).toInt();
          if (minS < 1) minS = 1;
          if (maxS < minS) maxS = minS;
          shuffleExprMinMs = (uint32_t)minS * 1000UL;
          shuffleExprMaxMs = (uint32_t)maxS * 1000UL;
        } else {
          long seconds = val.toInt();
          if (seconds < 1) seconds = 1;
          shuffleExprMinMs = shuffleExprMaxMs = (uint32_t)seconds * 1000UL;
        }
        changed = true;
      } else if (token.startsWith("neutral=") || token.startsWith("n=")) {
        // neutral=2-5 or neutral=3 (fixed duration)
        int eqPos = token.indexOf('=');
        String val = token.substring(eqPos + 1);
        int dashPos = val.indexOf('-');
        if (dashPos > 0) {
          long minS = val.substring(0, dashPos).toInt();
          long maxS = val.substring(dashPos + 1).toInt();
          if (minS < 1) minS = 1;
          if (maxS < minS) maxS = minS;
          shuffleNeutralMinMs = (uint32_t)minS * 1000UL;
          shuffleNeutralMaxMs = (uint32_t)maxS * 1000UL;
        } else {
          long seconds = val.toInt();
          if (seconds < 1) seconds = 1;
          shuffleNeutralMinMs = shuffleNeutralMaxMs = (uint32_t)seconds * 1000UL;
        }
        changed = true;
      }

      idx = commaPos + 1;
    }

    // Save changes to preferences
    if (changed) {
      preferences.putBool("shuf_en", shuffleEnabled);
      preferences.putUInt("shuf_emin", shuffleExprMinMs);
      preferences.putUInt("shuf_emax", shuffleExprMaxMs);
      preferences.putUInt("shuf_nmin", shuffleNeutralMinMs);
      preferences.putUInt("shuf_nmax", shuffleNeutralMaxMs);
    }

    // Kick the state machine so it starts with neutral
    if (turnedOn) {
      shuffleNeedsInit = true;
    }

    String resp = String("Shuffle: ") + (shuffleEnabled ? "ON" : "OFF");
    resp += "\nexpr=" + String(shuffleExprMinMs / 1000) + "-" + String(shuffleExprMaxMs / 1000) + "s";
    resp += "\nneutral=" + String(shuffleNeutralMinMs / 1000) + "-" + String(shuffleNeutralMaxMs / 1000) + "s";
    return resp;
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
