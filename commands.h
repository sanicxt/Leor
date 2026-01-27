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
// Unified display interface supporting both SSD1306 and SH1106
extern Adafruit_GFX* display;
extern Adafruit_SSD1306* display_ssd1306;
extern Adafruit_SH1106G* display_sh1106;

extern DisplayType activeDisplayType;
extern const char* DISPLAY_TYPE;
extern Preferences preferences;

// MochiEyes instances
extern MochiEyes<Adafruit_SSD1306>* pMochiEyes_ssd1306;
extern MochiEyes<Adafruit_SH1106G>* pMochiEyes_sh1106;

// Helper functions for display-specific methods
inline void displayClear() {
  if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
    display_ssd1306->clearDisplay();
  } else if (display_sh1106) {
    display_sh1106->clearDisplay();
  }
}

inline void displayShow() {
  if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
    display_ssd1306->display();
  } else if (display_sh1106) {
    display_sh1106->display();
  }
}

// Helper macros for MochiEyes method calls
#define MOCHI_CALL_VOID(method, ...) \
  do { \
    if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) { \
      pMochiEyes_ssd1306->method(__VA_ARGS__); \
    } else if (pMochiEyes_sh1106) { \
      pMochiEyes_sh1106->method(__VA_ARGS__); \
    } \
  } while(0)

#define MOCHI_CALL_VOID_NOARGS(method) \
  do { \
    if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) { \
      pMochiEyes_ssd1306->method(); \
    } else if (pMochiEyes_sh1106) { \
      pMochiEyes_sh1106->method(); \
    } \
  } while(0)

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
    // Return Edge Impulse model classes WITH action mappings
    String json = "[";
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"n\":\"" + String(ei_classifier_inferencing_categories[i]) + "\",\"a\":\"" + getGestureAction(i) + "\"}";
    }
    json += "]";
    return json;
}
inline bool isTraining() { return false; }  // EI model is pre-trained

// Reset all effects to default
void resetEffects() {
  // Reset emotions
  MOCHI_CALL_VOID_NOARGS(resetEmotions);
  
  // Reset effects
  MOCHI_CALL_VOID(setCuriosity, OFF);
  MOCHI_CALL_VOID(setHFlicker, OFF);
  MOCHI_CALL_VOID(setVFlicker, OFF);
  MOCHI_CALL_VOID(setSweat, OFF);
  MOCHI_CALL_VOID(setIdleMode, OFF);
  MOCHI_CALL_VOID(setBreathing, OFF);  // Turn off breathing too
  MOCHI_CALL_VOID(setEyebrows, false);
  MOCHI_CALL_VOID(setKnocked, OFF);
  
  // Reset gaze to center
  MOCHI_CALL_VOID(setGaze, 0, 0);
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
  Serial.println(F("\nDISPLAY:"));
  Serial.println(F("  display:type=<sh1106|ssd1306> - set display type"));
  Serial.println(F("  display:addr=<0x3C|0x3D> - set I2C address"));
  Serial.println(F("  display:test - test display animation"));
  Serial.println(F("  display:clear - clear display"));
  Serial.println(F("  display:info - show display info"));
  Serial.println(F("\nSYSTEM:"));
  Serial.println(F("  restart/reboot - restart ESP32"));
  Serial.println(F("\nTOGGLES:"));
  Serial.println(F("  sweat, cyclops, breathing"));
  Serial.println(F("  breathing:intensity=<0.01-0.2> - adjust breath depth"));
  Serial.println(F("  breathing:speed=<0.1-1.0> - adjust breath rate"));
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
    MOCHI_CALL_VOID(setMood, HAPPY);
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID_NOARGS(anim_laugh);
    MOCHI_CALL_VOID(setMouthType, 1);
    Serial.println(F("Expression: Happy"));
  }
  else if (cmd == "sad") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, TIRED);
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID(setMouthType, 2);
    Serial.println(F("Expression: Sad"));
  }
  else if (cmd == "angry") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, ANGRY);
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID(setMouthType, 5);
    Serial.println(F("Expression: Angry"));
  }
  else if (cmd == "love") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);  // Don't use HAPPY - it resets love animation
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID(setMouthType, 3);  // Open mouth (surprised/delighted)
    MOCHI_CALL_VOID_NOARGS(anim_love);  // Call LAST so resetEmotions() doesn't cancel it
    Serial.println(F("Expression: Love"));
  }
  else if (cmd == "surprised") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID(setCuriosity, ON);
    // No setPosition - curious motion creates the surprised look
    MOCHI_CALL_VOID_NOARGS(blink);
    MOCHI_CALL_VOID(setMouthType, 3);
    Serial.println(F("Expression: Surprised"));
  }
  else if (cmd == "confused") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID_NOARGS(anim_confused);
    MOCHI_CALL_VOID(setMouthType, 4);
    Serial.println(F("Expression: Confused"));
  }
  else if (cmd == "sleepy") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, TIRED);
    MOCHI_CALL_VOID(setPosition, POS_SW);
    MOCHI_CALL_VOID(setMouthType, 5);
    Serial.println(F("Expression: Sleepy"));
  }
  else if (cmd == "curious") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID(setCuriosity, ON);
    // No setPosition - curious mode creates left-right motion
    MOCHI_CALL_VOID(setMouthType, 4);
    Serial.println(F("Expression: Curious"));
  }
  else if (cmd == "nervous") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID(setSweat, ON);
    MOCHI_CALL_VOID(setCuriosity, ON);
    // No setPosition - nervous uses curious left-right motion
    MOCHI_CALL_VOID(setMouthType, 2);
    Serial.println(F("Expression: Nervous"));
  }
  else if (cmd == "knocked" || cmd == "dizzy") {
    resetEffects();
    MOCHI_CALL_VOID(setKnocked, ON);
    Serial.println(F("Expression: Knocked"));
  }
  else if (cmd == "neutral" || cmd == "normal" || cmd == "reset") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID(setMouthType, 1);
    Serial.println(F("Expression: Neutral"));
  }
  else if (cmd == "idle") {
    resetEffects();
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID(setIdleMode, ON, 1, 2);
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID(setMouthType, 1);
    Serial.println(F("Mode: Idle"));
  }
  else if (cmd == "raised") {
    resetEffects();
    MOCHI_CALL_VOID(setEyebrows, true);
    MOCHI_CALL_VOID(setMood, DEFAULT);
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    MOCHI_CALL_VOID(setMouthType, 4);
    Serial.println(F("Expression: Raised eyebrows"));
  }

  // ==================== MOUTH ====================
  else if (cmd == "smile") {
    MOCHI_CALL_VOID(setMouthType, 1);
    Serial.println(F("Mouth: Smile"));
  }
  else if (cmd == "frown") {
    MOCHI_CALL_VOID(setMouthType, 2);
    Serial.println(F("Mouth: Frown"));
  }
  else if (cmd == "open") {
    MOCHI_CALL_VOID(setMouthType, 3);
    Serial.println(F("Mouth: Open"));
  }
  else if (cmd == "ooo") {
    MOCHI_CALL_VOID(setMouthType, 4);
    Serial.println(F("Mouth: Ooo"));
  }
  else if (cmd == "flat") {
    MOCHI_CALL_VOID(setMouthType, 5);
    Serial.println(F("Mouth: Flat"));
  }
  else if (cmd == "uwum") {
    MOCHI_CALL_VOID(setMouthType, 6);
    Serial.println(F("Mouth: UwU"));
  }
  else if (cmd == "xdm") {
    MOCHI_CALL_VOID(setMouthType, 7);
    Serial.println(F("Mouth: XD"));
  }
  else if (cmd.startsWith("talk")) {
    int duration = 3000;
    int spacePos = cmd.indexOf(' ');
    if (spacePos > 0) {
      duration = cmd.substring(spacePos + 1).toInt();
      if (duration < 100) duration = 100;
      if (duration > 10000) duration = 10000;
    }
    MOCHI_CALL_VOID(startMouthAnim, 1, duration);
    Serial.print(F("Mouth: Talking for ")); Serial.print(duration); Serial.println(F("ms"));
  }
  else if (cmd.startsWith("chew")) {
    int duration = 2000;
    int spacePos = cmd.indexOf(' ');
    if (spacePos > 0) {
      duration = cmd.substring(spacePos + 1).toInt();
      if (duration < 100) duration = 100;
      if (duration > 10000) duration = 10000;
    }
    MOCHI_CALL_VOID(startMouthAnim, 2, duration);
    Serial.print(F("Mouth: Chewing for ")); Serial.print(duration); Serial.println(F("ms"));
  }
  else if (cmd.startsWith("wobble")) {
    int duration = 2000;
    int spacePos = cmd.indexOf(' ');
    if (spacePos > 0) {
      duration = cmd.substring(spacePos + 1).toInt();
      if (duration < 100) duration = 100;
      if (duration > 10000) duration = 10000;
    }
    MOCHI_CALL_VOID(startMouthAnim, 3, duration);
    Serial.print(F("Mouth: Wobbling for ")); Serial.print(duration); Serial.println(F("ms"));
  }

  // ==================== ACTIONS ====================
  else if (cmd == "blink") {
    MOCHI_CALL_VOID_NOARGS(blink);
    Serial.println(F("Action: Blink"));
  }
  else if (cmd == "wink") {
    MOCHI_CALL_VOID(wink, true);
    MOCHI_CALL_VOID(setMouthType, 1);
    Serial.println(F("Action: Wink"));
  }
  else if (cmd == "winkr") {
    MOCHI_CALL_VOID(wink, false);
    MOCHI_CALL_VOID(setMouthType, 1);
    Serial.println(F("Action: Wink Right"));
  }
  else if (cmd == "laugh") {
    MOCHI_CALL_VOID_NOARGS(anim_laugh);
    Serial.println(F("Action: Laugh"));
  }
  else if (cmd == "cry") {
    MOCHI_CALL_VOID_NOARGS(anim_cry);
    Serial.println(F("Action: Cry"));
  }
  else if (cmd == "uwu") {
    MOCHI_CALL_VOID(triggerUwU, 3.0f); // Default 3 seconds
    Serial.println(F("Expression: UwU"));
  }
  else if (cmd == "xd") {
    MOCHI_CALL_VOID(triggerXD, 3.0f);
    Serial.println(F("Expression: XD"));
  }
  // knocked moved to expressions

  // ==================== POSITIONS ====================
  else if (cmd == "center") {
    MOCHI_CALL_VOID(setPosition, DEFAULT);
    Serial.println(F("Position: Center"));
  }
  else if (cmd == "n" || cmd == "up") {
    MOCHI_CALL_VOID(setPosition, POS_N);
    Serial.println(F("Position: North"));
  }
  else if (cmd == "ne") {
    MOCHI_CALL_VOID(setPosition, POS_NE);
    Serial.println(F("Position: North-East"));
  }
  else if (cmd == "e" || cmd == "right") {
    MOCHI_CALL_VOID(setPosition, POS_E);
    Serial.println(F("Position: East"));
  }
  else if (cmd == "se") {
    MOCHI_CALL_VOID(setPosition, POS_SE);
    Serial.println(F("Position: South-East"));
  }
  else if (cmd == "s" || cmd == "down") {
    MOCHI_CALL_VOID(setPosition, POS_S);
    Serial.println(F("Position: South"));
  }
  else if (cmd == "sw") {
    MOCHI_CALL_VOID(setPosition, POS_SW);
    Serial.println(F("Position: South-West"));
  }
  else if (cmd == "w" || cmd == "left") {
    MOCHI_CALL_VOID(setPosition, POS_W);
    Serial.println(F("Position: West"));
  }
  else if (cmd == "nw") {
    MOCHI_CALL_VOID(setPosition, POS_NW);
    Serial.println(F("Position: North-West"));
  }

  // ==================== TOGGLES ====================
  else if (cmd == "sweat") {
    static bool sweatOn = false;
    sweatOn = !sweatOn;
    MOCHI_CALL_VOID(setSweat, sweatOn);
    Serial.print(F("Sweat: "));
    Serial.println(sweatOn ? F("ON") : F("OFF"));
  }
  else if (cmd == "cyclops") {
    static bool cyclopsOn = false;
    cyclopsOn = !cyclopsOn;
    MOCHI_CALL_VOID(setCyclops, cyclopsOn);
    Serial.print(F("Cyclops: "));
    Serial.println(cyclopsOn ? F("ON") : F("OFF"));
  }
  
  // ==================== BREATHING CONTROL ====================
  else if (cmd == "breathing") {
    static bool breathingOn = false;
    breathingOn = !breathingOn;
    MOCHI_CALL_VOID(setBreathing, breathingOn, 0.08f, 0.3f);  // 8% squish, 3.3s cycle
    Serial.print(F("Breathing: "));
    Serial.println(breathingOn ? F("ON") : F("OFF"));
  }
  else if (cmd.startsWith("breathing:intensity=")) {
    float intensity = cmd.substring(20).toFloat();
    MOCHI_CALL_VOID(setBreathingIntensity, intensity);
    Serial.print(F("Breathing intensity: "));
    Serial.println(intensity);
  }
  else if (cmd.startsWith("breathing:speed=")) {
    float speed = cmd.substring(16).toFloat();
    MOCHI_CALL_VOID(setBreathingSpeed, speed);
    Serial.print(F("Breathing speed: "));
    Serial.println(speed);
  }
  else if (cmd == "mouth") {
    static bool mouthOn = true;
    mouthOn = !mouthOn;
    MOCHI_CALL_VOID(setMouthEnabled, mouthOn);
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
      json += "},\"display\":{";
      json += "\"type\":\"" + preferences.getString("disp_type", "sh1106") + "\",";
      json += "\"addr\":\"0x" + String(preferences.getUInt("disp_addr", I2C_ADDRESS), HEX) + "\"";
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
          MOCHI_CALL_VOID(setWidth, value, value);
          preferences.putInt("ew", value);
        } else if (key == "eh") {
          MOCHI_CALL_VOID(setHeight, value, value);
          preferences.putInt("eh", value);
        } else if (key == "es") {
          MOCHI_CALL_VOID(setSpacebetween, value);
          preferences.putInt("es", value);
        } else if (key == "er") {
          MOCHI_CALL_VOID(setBorderradius, value, value);
          preferences.putInt("er", value);
        } else if (key == "mw") {
          MOCHI_CALL_VOID(setMouthSize, value, 6);
          preferences.putInt("mw", value);
        } else if (key == "lt") {
          MOCHI_CALL_VOID(setLaughDuration, value);
          preferences.putInt("lt", value);
        } else if (key == "vt") {
          MOCHI_CALL_VOID(setLoveDuration, value);
          preferences.putInt("vt", value);
        } else if (key == "bi") {
          MOCHI_CALL_VOID(setAutoblinker, true, value, 3);
          preferences.putInt("bi", value);
        } else if (key == "gs") {
          MOCHI_CALL_VOID(setGazeSpeed, (float)value);
          preferences.putInt("gs", value);
        } else if (key == "os") {
          MOCHI_CALL_VOID(setOpennessSpeed, (float)value);
          preferences.putInt("os", value);
        } else if (key == "ss") {
          MOCHI_CALL_VOID(setSquishSpeed, (float)value);
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

  // ==================== BLE POWER COMMANDS ====================
  // ble: = get BLE power status
  else if (cmd == "ble:") {
    String resp = "ble:lp=" + String(getBLELowPowerMode() ? "1" : "0");
    return resp;
  }
  // ble:lp=1/0 = set low power mode
  else if (cmd.startsWith("ble:lp=")) {
    int val = cmd.substring(7).toInt();
    setBLELowPowerMode(val == 1);
    preferences.putBool("ble_lp", val == 1);
    return "ble:lp=" + String(val == 1 ? "1" : "0");
  }

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
      } else if (token == "quick") {
        // Quick preset: fast expression cycling
        shuffleExprMinMs = 1000;
        shuffleExprMaxMs = 2000;
        shuffleNeutralMinMs = 500;
        shuffleNeutralMaxMs = 1500;
        shuffleEnabled = true;
        turnedOn = true;
        changed = true;
      } else if (token == "slow") {
        // Slow preset: calm, relaxed cycling
        shuffleExprMinMs = 4000;
        shuffleExprMaxMs = 8000;
        shuffleNeutralMinMs = 3000;
        shuffleNeutralMaxMs = 6000;
        shuffleEnabled = true;
        turnedOn = true;
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

  // ==================== DISPLAY SETTINGS ====================
  else if (cmd.startsWith("display:")) {
    String params = cmd.substring(8);
    params.trim();
    
    if (params.startsWith("type=")) {
      String type = params.substring(5);
      type.trim(); // Remove any whitespace
      type.toLowerCase();
      
      // Save display type preference
      if (type == "ssd1306" || type == "sh1106") {
        preferences.putString("disp_type", type);
        
        String current = String(DISPLAY_TYPE);
        current.toLowerCase();
        
        // Always return success if valid, noting if restart is needed
        if (type != current) {
          return "display:type=" + type + " saved. Restart required: send 'restart' command";
        } else {
          return "display:type=" + type + " (already active)";
        }
      }
      return "display:type invalid. Use: sh1106 or ssd1306";
    }
    else if (params.startsWith("addr=")) {
      // Save I2C address change
      String addr = params.substring(5);
      addr.trim();
      
      // Parse hex address
      long addrVal = strtol(addr.c_str(), NULL, 16);
      if (addrVal >= 0x00 && addrVal <= 0xFF) {
        preferences.putUInt("disp_addr", (uint8_t)addrVal);
        return "display:addr=" + addr + " saved. Restart required: send 'restart' command";
      }
      return "display:addr invalid. Use hex format: 0x3C or 0x3D";
    }
    else if (params == "test") {
      // Test display with animation - use display-specific objects for proper method calls
      if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        display_ssd1306->setTextSize(2);
        display_ssd1306->setTextColor(SSD1306_WHITE);
        display_ssd1306->setCursor(20, 20);
        display_ssd1306->println("DISPLAY");
        display_ssd1306->setCursor(35, 40);
        display_ssd1306->println("TEST");
        display_ssd1306->display();
        delay(1000);
        
        // Animate a bouncing box
        for (int i = 0; i < 30; i++) {
          display_ssd1306->clearDisplay();
          int x = (i * 4) % (SCREEN_WIDTH - 20);
          int y = 20 + (int)(15 * sin(i * 0.3));
          display_ssd1306->fillRect(x, y, 20, 20, SSD1306_WHITE);
          display_ssd1306->display();
          delay(50);
        }
      } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        display_sh1106->setTextSize(2);
        display_sh1106->setTextColor(SH110X_WHITE);
        display_sh1106->setCursor(20, 20);
        display_sh1106->println("DISPLAY");
        display_sh1106->setCursor(35, 40);
        display_sh1106->println("TEST");
        display_sh1106->display();
        delay(1000);
        
        // Animate a bouncing box
        for (int i = 0; i < 30; i++) {
          display_sh1106->clearDisplay();
          int x = (i * 4) % (SCREEN_WIDTH - 20);
          int y = 20 + (int)(15 * sin(i * 0.3));
          display_sh1106->fillRect(x, y, 20, 20, SH110X_WHITE);
          display_sh1106->display();
          delay(50);
        }
      }
      
      return "display:test complete";
    }
    else if (params == "clear") {
      displayClear();
      displayShow();
      return "display:clear";
    }
    else if (params == "info") {
      String info = "Display: ";
      info += DISPLAY_TYPE;
      
      // Get current I2C address from preferences
      uint8_t currentAddr = preferences.getUInt("disp_addr", I2C_ADDRESS);
      info += " @ 0x" + String(currentAddr, HEX);
      info += " (" + String(SCREEN_WIDTH) + "x" + String(SCREEN_HEIGHT) + ")";
      
      // Show saved preferences
      String savedType = preferences.getString("disp_type", "none");
      if (savedType != "none") {
        info += "\nSaved: " + savedType;
      }
      uint8_t savedAddr = preferences.getUInt("disp_addr", 0);
      if (savedAddr != 0) {
        info += " @ 0x" + String(savedAddr, HEX);
      }
      
      return info;
    }
    else {
      return "display: usage - type=<sh1106|ssd1306>, addr=<hex>, test, clear, info";
    }
  }

  // ==================== RESTART ====================
  else if (cmd == "restart" || cmd == "reboot") {
    Serial.println(F("Restarting ESP32..."));
    delay(500);
    ESP.restart();
    return "Restarting...";
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
