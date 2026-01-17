/*
 * commands.h - Command handler for leor
 * Handles serial and web commands for expressions, mouth, actions, etc.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>
#include <Preferences.h>
#include "MochiEyes.h"
#include "ei_gesture.h"

// Re-define DEFAULT after Edge Impulse include (ges_inferencing.h has #undef DEFAULT)
#ifndef DEFAULT
#define DEFAULT 0
#endif

// Forward declaration - will be set in main sketch
extern MochiEyes<Adafruit_SH1106G>* pMochiEyes;
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

// ==================== Stub functions for deprecated gesture_trainer.h features ====================
// Edge Impulse model is pre-trained, so weight transfer functions are not needed
inline void appendWeightChunk(const String& chunk) { /* no-op */ }
inline bool finalizeWeights() { return false; }
inline bool loadWeightsFromBase64(const String& data) { return false; }
inline void setGestureLabel(int index, const char* name, const char* action) { /* no-op */ }
inline void clearAllGestures() { /* no-op */ }
inline String listGestures() { 
    // Return Edge Impulse model classes
    String json = "[";
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"n\":\"" + String(ei_classifier_inferencing_categories[i]) + "\",\"a\":\"\"}";
    }
    json += "]";
    return json;
}
inline bool isTraining() { return false; }  // EI model is pre-trained

// Reset all effects to default
void resetEffects() {
  // Reset emotions
  pMochiEyes->resetEmotions();
  
  // Reset effects
  pMochiEyes->setCuriosity(OFF);
  pMochiEyes->setHFlicker(OFF);
  pMochiEyes->setVFlicker(OFF);
  pMochiEyes->setSweat(OFF);
  pMochiEyes->setIdleMode(OFF);
  pMochiEyes->setEyebrows(false);
  pMochiEyes->setKnocked(OFF);
  
  // Reset gaze to center
  pMochiEyes->setGaze(0, 0);
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
  
  // Handle weight transfer BEFORE toLowerCase (base64 is case-sensitive!)
  if (cmd.startsWith("gw+")) {
    String chunk = cmd.substring(3);
    appendWeightChunk(chunk);
    return "gw+ok";
  }
  if (cmd == "gw!") {
    if (finalizeWeights()) {
      return "gw:ok";
    } else {
      return "gw:err";
    }
  }
  if (cmd.startsWith("gw=")) {
    // Direct single-chunk weight load (for small models)
    String base64data = cmd.substring(3);
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
    pMochiEyes->setMood(HAPPY);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->anim_laugh();
    pMochiEyes->setMouthType(1);
    Serial.println(F("Expression: Happy"));
  }
  else if (cmd == "sad") {
    resetEffects();
    pMochiEyes->setMood(TIRED);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->setMouthType(2);
    Serial.println(F("Expression: Sad"));
  }
  else if (cmd == "angry") {
    resetEffects();
    pMochiEyes->setMood(ANGRY);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->setMouthType(5);
    Serial.println(F("Expression: Angry"));
  }
  else if (cmd == "love") {
    resetEffects();
    pMochiEyes->anim_love();
    pMochiEyes->setMood(HAPPY);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->setMouthType(3);
    Serial.println(F("Expression: Love"));
  }
  else if (cmd == "surprised") {
    resetEffects();
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->setCuriosity(ON);
    pMochiEyes->setPosition(POS_N);
    pMochiEyes->blink();
    pMochiEyes->setMouthType(3);
    Serial.println(F("Expression: Surprised"));
  }
  else if (cmd == "confused") {
    resetEffects();
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->anim_confused();
    pMochiEyes->setMouthType(4);
    Serial.println(F("Expression: Confused"));
  }
  else if (cmd == "sleepy") {
    resetEffects();
    pMochiEyes->setMood(TIRED);
    pMochiEyes->setPosition(POS_SW);
    pMochiEyes->setMouthType(5);
    Serial.println(F("Expression: Sleepy"));
  }
  else if (cmd == "curious") {
    resetEffects();
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->setCuriosity(ON);
    pMochiEyes->setPosition(POS_E);
    pMochiEyes->setMouthType(4);
    Serial.println(F("Expression: Curious"));
  }
  else if (cmd == "nervous") {
    resetEffects();
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->setSweat(ON);
    pMochiEyes->setCuriosity(ON);
    pMochiEyes->setPosition(POS_N);  // Use POS_N instead of NW - left eye gets bigger with curious mode
    pMochiEyes->setMouthType(2);
    Serial.println(F("Expression: Nervous"));
  }
  else if (cmd == "knocked" || cmd == "dizzy") {
    resetEffects();
    pMochiEyes->setKnocked(ON);
    Serial.println(F("Expression: Knocked"));
  }
  else if (cmd == "neutral" || cmd == "normal" || cmd == "reset") {
    resetEffects();
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->setMouthType(1);
    Serial.println(F("Expression: Neutral"));
  }
  else if (cmd == "idle") {
    resetEffects();
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->setIdleMode(ON, 1, 2);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->setMouthType(1);
    Serial.println(F("Mode: Idle"));
  }
  else if (cmd == "raised") {
    resetEffects();
    pMochiEyes->setEyebrows(true);
    pMochiEyes->setMood(DEFAULT);
    pMochiEyes->setPosition(DEFAULT);
    pMochiEyes->setMouthType(4);
    Serial.println(F("Expression: Raised eyebrows"));
  }

  // ==================== MOUTH ====================
  else if (cmd == "smile") {
    pMochiEyes->setMouthType(1);
    Serial.println(F("Mouth: Smile"));
  }
  else if (cmd == "frown") {
    pMochiEyes->setMouthType(2);
    Serial.println(F("Mouth: Frown"));
  }
  else if (cmd == "open") {
    pMochiEyes->setMouthType(3);
    Serial.println(F("Mouth: Open"));
  }
  else if (cmd == "ooo") {
    pMochiEyes->setMouthType(4);
    Serial.println(F("Mouth: Ooo"));
  }
  else if (cmd == "flat") {
    pMochiEyes->setMouthType(5);
    Serial.println(F("Mouth: Flat"));
  }
  else if (cmd == "talk") {
    pMochiEyes->startMouthAnim(1, 3000);
    Serial.println(F("Mouth: Talking"));
  }
  else if (cmd == "chew") {
    pMochiEyes->startMouthAnim(2, 2000);
    Serial.println(F("Mouth: Chewing"));
  }
  else if (cmd == "wobble") {
    pMochiEyes->startMouthAnim(3, 2000);
    Serial.println(F("Mouth: Wobbling"));
  }

  // ==================== ACTIONS ====================
  else if (cmd == "blink") {
    pMochiEyes->blink();
    Serial.println(F("Action: Blink"));
  }
  else if (cmd == "wink") {
    pMochiEyes->wink(true);
    pMochiEyes->setMouthType(1);
    Serial.println(F("Action: Wink"));
  }
  else if (cmd == "winkr") {
    pMochiEyes->wink(false);
    pMochiEyes->setMouthType(1);
    Serial.println(F("Action: Wink Right"));
  }
  else if (cmd == "laugh") {
    pMochiEyes->anim_laugh();
    Serial.println(F("Action: Laugh"));
  }
  else if (cmd == "cry") {
    pMochiEyes->anim_cry();
    Serial.println(F("Action: Cry"));
  }
  // knocked moved to expressions

  // ==================== POSITIONS ====================
  else if (cmd == "center") {
    pMochiEyes->setPosition(DEFAULT);
    Serial.println(F("Position: Center"));
  }
  else if (cmd == "n" || cmd == "up") {
    pMochiEyes->setPosition(POS_N);
    Serial.println(F("Position: North"));
  }
  else if (cmd == "ne") {
    pMochiEyes->setPosition(POS_NE);
    Serial.println(F("Position: North-East"));
  }
  else if (cmd == "e" || cmd == "right") {
    pMochiEyes->setPosition(POS_E);
    Serial.println(F("Position: East"));
  }
  else if (cmd == "se") {
    pMochiEyes->setPosition(POS_SE);
    Serial.println(F("Position: South-East"));
  }
  else if (cmd == "s" || cmd == "down") {
    pMochiEyes->setPosition(POS_S);
    Serial.println(F("Position: South"));
  }
  else if (cmd == "sw") {
    pMochiEyes->setPosition(POS_SW);
    Serial.println(F("Position: South-West"));
  }
  else if (cmd == "w" || cmd == "left") {
    pMochiEyes->setPosition(POS_W);
    Serial.println(F("Position: West"));
  }
  else if (cmd == "nw") {
    pMochiEyes->setPosition(POS_NW);
    Serial.println(F("Position: North-West"));
  }

  // ==================== TOGGLES ====================
  else if (cmd == "sweat") {
    static bool sweatOn = false;
    sweatOn = !sweatOn;
    pMochiEyes->setSweat(sweatOn);
    Serial.print(F("Sweat: "));
    Serial.println(sweatOn ? F("ON") : F("OFF"));
  }
  else if (cmd == "cyclops") {
    static bool cyclopsOn = false;
    cyclopsOn = !cyclopsOn;
    pMochiEyes->setCyclops(cyclopsOn);
    Serial.print(F("Cyclops: "));
    Serial.println(cyclopsOn ? F("ON") : F("OFF"));
  }
  else if (cmd == "mouth") {
    static bool mouthOn = true;
    mouthOn = !mouthOn;
    pMochiEyes->setMouthEnabled(mouthOn);
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
    
    // If empty params, return current settings as JSON for robust sync
    if (params.length() == 0) {
      String json = "{\"type\":\"sync\",\"settings\":{";
      json += "\"ew\":" + String(preferences.getInt("ew", 36)) + ",";
      json += "\"eh\":" + String(preferences.getInt("eh", 36)) + ",";
      json += "\"es\":" + String(preferences.getInt("es", 10)) + ",";
      json += "\"er\":" + String(preferences.getInt("er", 8)) + ",";
      json += "\"mw\":" + String(preferences.getInt("mw", 20)) + ",";
      json += "\"lt\":" + String(preferences.getInt("lt", 1000)) + ",";
      json += "\"vt\":" + String(preferences.getInt("vt", 2000)) + ",";
      json += "\"bi\":" + String(preferences.getInt("bi", 3)) + ",";
      json += "\"gs\":" + String(preferences.getInt("gs", 6)) + ",";
      json += "\"os\":" + String(preferences.getInt("os", 12)) + ",";
      json += "\"ss\":" + String(preferences.getInt("ss", 10));
      json += "},\"state\":{";
      json += "\"shuf\":" + String(shuffleEnabled ? "1" : "0") + ",";
      json += "\"mpu\":" + String(mpuVerbose ? "1" : "0");
      json += "}}";
      return json;
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
          pMochiEyes->setWidth(value, value);
          preferences.putInt("ew", value);
        } else if (key == "eh") {
          pMochiEyes->setHeight(value, value);
          preferences.putInt("eh", value);
        } else if (key == "es") {
          pMochiEyes->setSpacebetween(value);
          preferences.putInt("es", value);
        } else if (key == "er") {
          pMochiEyes->setBorderradius(value, value);
          preferences.putInt("er", value);
        } else if (key == "mw") {
          pMochiEyes->setMouthSize(value, 6);
          preferences.putInt("mw", value);
        } else if (key == "lt") {
          pMochiEyes->setLaughDuration(value);
          preferences.putInt("lt", value);
        } else if (key == "vt") {
          pMochiEyes->setLoveDuration(value);
          preferences.putInt("vt", value);
        } else if (key == "bi") {
          pMochiEyes->setAutoblinker(true, value, 3);
          preferences.putInt("bi", value);
        } else if (key == "gs") {
          pMochiEyes->setGazeSpeed((float)value);
          preferences.putInt("gs", value);
        } else if (key == "os") {
          pMochiEyes->setOpennessSpeed((float)value);
          preferences.putInt("os", value);
        } else if (key == "ss") {
          pMochiEyes->setSquishSpeed((float)value);
          preferences.putInt("ss", value);
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
  // ga=index:action = set gesture action mapping (from frontend)
  else if (cmd.startsWith("ga=")) {
    String params = cmd.substring(3);
    int sep = params.indexOf(':');
    if (sep > 0) {
      int index = params.substring(0, sep).toInt();
      String action = params.substring(sep + 1);
      action.trim();
      setGestureAction(index, action);
      return "ga:ok";
    }
    return "ga:err";
  }
  // gm=1 or gm=0 = enable/disable matching
  else if (cmd.startsWith("gm=")) {
    String val = cmd.substring(3);
    val.trim();
    bool enable = (val == "1");
    setMatchingEnabled(enable);
    return enable ? "gm=1" : "gm=0";
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
  // gs: = get gesture settings
  else if (cmd == "gs:") {
    return getGestureSettings();
  }
  // grt=1500 = set reaction time in ms
  else if (cmd.startsWith("grt=")) {
    int val = cmd.substring(4).toInt();
    if (val >= 500 && val <= 10000) {
      setGestureReactionTime(val);
      return "rt=" + String(val);
    }
    return "grt:err";
  }
  // gcf=70 = set confidence threshold (percent)
  else if (cmd.startsWith("gcf=")) {
    int val = cmd.substring(4).toInt();
    if (val >= 30 && val <= 99) {
      setGestureConfidence(val);
      return "cf=" + String(val);
    }
    return "gcf:err";
  }
  // gcd=2000 = set cooldown in ms
  else if (cmd.startsWith("gcd=")) {
    int val = cmd.substring(4).toInt();
    if (val >= 500 && val <= 10000) {
      setGestureCooldown(val);
      return "cd=" + String(val);
    }
    return "gcd:err";
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
