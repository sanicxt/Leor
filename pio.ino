/*
 * Leora - Animated Robot Eyes for ESP32-C3
 * 
 * Features:
 * - Animated OLED robot eyes with expressions
 * - WiFi web interface control
 * - Serial command interface
 * - TOF distance sensor for gesture detection
 * - Gesture training and recognition
 * - Access Point fallback if WiFi fails
 * 
 * Hardware:
 * - ESP32-C3 Super Mini
 * - SH1106G 128x64 OLED display (I2C)
 * - VL53L0X TOF distance sensor
 */

// ==================== Libraries ====================
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Adafruit_VL53L0X.h"
#include <WiFi.h>
#include <WebServer.h>

// ==================== Configuration ====================
#include "config.h"

// ==================== Hardware Objects ====================
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_VL53L0X tofSensor = Adafruit_VL53L0X();
WebServer server(80);

// ==================== RoboEyes ====================
#include "RoboEyes/src/FluxGarage_RoboEyes.h"
RoboEyes<Adafruit_SH1106G> roboEyes(display);
RoboEyes<Adafruit_SH1106G>* pRoboEyes = &roboEyes;  // Pointer for commands.h

// ==================== Gesture Training Overlay ====================
// Called by gesture_trainer.h to draw training status on OLED
void drawTrainingOverlay(const char* status, int progress);

// ==================== Modules ====================
#include "gesture_trainer.h"
#include "commands.h"
#include "wifi_manager.h"
#include "web_ui.h"

// ==================== State Variables ====================
String inputBuffer = "";
bool tofAvailable = false;
unsigned long lastTofRead = 0;
unsigned long reactionStartTime = 0;
bool isReacting = false;

// Random expression shuffle
bool shuffleEnabled = false;
uint32_t shuffleIntervalMs = 10000;  // default 10s
int lastShuffleIndex = -1;
bool shuffleNeedsInit = false;

enum ShufflePhase {
  SHUFFLE_NEUTRAL,
  SHUFFLE_EXPRESSION
};
ShufflePhase shufflePhase = SHUFFLE_NEUTRAL;
unsigned long shuffleNextChangeMs = 0;

// Gesture matching state
int matchSamples[SAMPLE_COUNT];
int matchSampleIndex = 0;
unsigned long lastMatchSampleTime = 0;
bool collectingForMatch = false;

// ==================== Gesture Detection ====================
int lastDistance = 0;           // Last measured distance in mm
int baseDistance = 0;           // Reference distance for rub detection
int gestureDirection = 0;       // 1 = moving away, -1 = moving closer, 0 = steady
int patCount = 0;               // Count of direction changes
unsigned long handDetectedTime = 0;  // When hand was first detected
unsigned long lastMovementTime = 0;  // Last time significant movement detected
unsigned long patStartTime = 0;      // When first pat movement detected
bool handPresent = false;       // Is a hand currently in range?
enum GestureState { GESTURE_NONE, GESTURE_DETECTING, GESTURE_PAT, GESTURE_RUB };
GestureState gestureState = GESTURE_NONE;
// Verbose debug logging for TOF sensor
// Verbose debug logging for TOF sensor
bool tofVerbose = false;
// Verbose debug logging for gesture processing
bool gestVerbose = false;

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  delay(250);

  randomSeed((uint32_t)micros());
  
  Serial.println(F("\n============================="));
  Serial.println(F("      Leora Starting..."));
  Serial.println(F("=============================\n"));
  
  // Initialize display
  display.begin(I2C_ADDRESS, true);
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
  roboEyes.setAutoblinker(ON, BLINK_INTERVAL, BLINK_VARIATION);
  Serial.println(F("✓ Display initialized"));
  
  // Connect to WiFi (with animation)
  WifiStatus wifiStatus = connectWiFiWithAnimation();
  
  // Setup web server
  setupWebServerRoutes();
  
  // Initialize TOF sensor in continuous mode (non-blocking)
  if (tofSensor.begin()) {
    tofAvailable = true;
    tofSensor.startRangeContinuous(50);  // 50ms measurement period
    Serial.println(F("✓ TOF sensor ready (continuous mode)"));
  } else {
    Serial.println(F("✗ TOF sensor not found"));
  }
  
  // Initialize gesture trainer
  initGestureTrainer();
  
  // Ready!
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  
  Serial.println(F("\n============================="));
  Serial.println(F("    Leora Ready!"));
  Serial.println(F("============================="));
  Serial.println(F("Type 'help' for commands\n"));
  printConnectionInfo();
}

// ==================== Main Loop ====================
void loop() {
  // Handle web requests
  server.handleClient();
  
  // Update display
  roboEyes.update();
  
  // Draw training overlay if in training mode
  if (isTraining()) {
    drawTrainingOverlay(getTrainingStatus().c_str(), getTrainingProgress());
  }
  
  // Handle TOF sensor reactions
  handleTofSensor();

  // Randomly shuffle expressions
  maybeShuffleExpression();
  
  // Handle serial commands
  handleSerialInput();
}

// Pick a random expression and apply it via existing command handler
void maybeShuffleExpression() {
  if (!shuffleEnabled) return;
  if (isReacting) return;
  if (isTraining()) return;

  unsigned long now = millis();

  // When shuffle gets enabled, force neutral first for 2 seconds
  if (shuffleNeedsInit) {
    shuffleNeedsInit = false;
    shufflePhase = SHUFFLE_NEUTRAL;
    handleCommand(String("neutral"));
    shuffleNextChangeMs = now + 2000;
    return;
  }

  if (shuffleNextChangeMs != 0 && now < shuffleNextChangeMs) return;

  if (shufflePhase == SHUFFLE_EXPRESSION) {
    // After showing an expression, go neutral for 2–5 seconds
    handleCommand(String("neutral"));
    shufflePhase = SHUFFLE_NEUTRAL;
    shuffleNextChangeMs = now + (unsigned long)random(2000, 5001);
    return;
  }

  static const char* kExpressions[] = {
    "happy", "sad", "angry", "love", "surprised", "confused",
    "sleepy", "curious", "nervous", "knocked", "neutral", "idle"
  };
  const int count = (int)(sizeof(kExpressions) / sizeof(kExpressions[0]));
  int idx = random(count);
  if (count > 1 && idx == lastShuffleIndex) {
    idx = (idx + 1 + random(count - 1)) % count;
  }
  lastShuffleIndex = idx;

  handleCommand(String(kExpressions[idx]));

  // After picking an expression, hold it for the configured shuffle interval
  shufflePhase = SHUFFLE_EXPRESSION;
  shuffleNextChangeMs = now + shuffleIntervalMs;
}

// ==================== Helper Functions ====================

// Connect to WiFi with eye animation
WifiStatus connectWiFiWithAnimation() {
  Serial.println(F("Connecting to WiFi..."));
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);

#ifdef HOSTNAME
  WiFi.setHostname(HOSTNAME);
#endif
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    // Eye animation while connecting
    roboEyes.setPosition(attempts % 2 == 0 ? E : W);
    roboEyes.update();
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n✓ WiFi connected!"));
    Serial.print(F("  IP: "));
    Serial.println(WiFi.localIP());
    roboEyes.setMood(HAPPY);
    return WIFI_CONNECTED_STA;
  }
  
  // Start Access Point
  Serial.println(F("\n✗ WiFi failed, starting AP..."));
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  
  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    delay(500);
    Serial.println(F("✓ Access Point started!"));
    Serial.print(F("  SSID: "));
    Serial.println(AP_SSID);
    Serial.print(F("  IP: "));
    Serial.println(WiFi.softAPIP());
    roboEyes.setCuriosity(ON);
    return WIFI_CONNECTED_AP;
  }
  
  Serial.println(F("✗ AP failed!"));
  roboEyes.setMood(TIRED);
  return WIFI_FAILED;
}

// Setup web server routes
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleCmd() {
  if (server.hasArg("c")) {
    String cmd = server.arg("c");
    String response = handleCommand(cmd);
    server.send(200, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Missing command");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void setupWebServerRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/cmd", HTTP_GET, handleCmd);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println(F("✓ Web server started"));
}

// Draw training overlay on OLED
void drawTrainingOverlay(const char* status, int progress) {
  // Draw a banner at top of screen
  display.fillRect(0, 0, SCREEN_WIDTH, 20, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  
  // Center the status text
  int textWidth = strlen(status) * 6;  // Approx 6 pixels per char
  int x = (SCREEN_WIDTH - textWidth) / 2;
  display.setCursor(x, 2);
  display.print(status);
  
  // Draw progress bar if recording
  if (progress >= 0 && progress <= 100) {
    display.fillRect(4, 12, SCREEN_WIDTH - 8, 6, SH110X_BLACK);
    int barWidth = ((SCREEN_WIDTH - 10) * progress) / 100;
    display.fillRect(5, 13, barWidth, 4, SH110X_WHITE);
  }
  
  // Draw recording dot if recording
  if (trainState == TRAIN_RECORDING) {
    // Blinking dot
    if ((millis() / 300) % 2 == 0) {
      display.fillCircle(SCREEN_WIDTH - 10, 6, 4, SH110X_BLACK);
    }
  }
  
  display.setTextColor(SH110X_WHITE);
  display.display();
}

// Execute action from matched gesture
void executeGestureAction(const String& action) {
  isReacting = true;
  reactionStartTime = millis();
  Serial.print(F("[GESTURE] Matched! Action: "));
  Serial.println(action);
  handleCommand(action);
}

// React to detected gesture
void reactToGesture(GestureState gesture) {
  isReacting = true;
  reactionStartTime = millis();
  
  if (gesture == GESTURE_PAT) {
    // Pat = Happy reaction!
    Serial.println(F("[PAT] Detected! Happy!"));
    resetEffects();
    roboEyes.setMood(HAPPY);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setMouthType(1);  // Smile
    roboEyes.anim_laugh();     // Little laugh
  } else if (gesture == GESTURE_RUB) {
    // Rub = Love/content reaction
    Serial.println(F("[RUB] Detected! Love!"));
    resetEffects();
    roboEyes.setMood(HAPPY);
    roboEyes.setPosition(N);   // Look up at hand
    roboEyes.anim_love();      // Heart eyes
  }
}

// Handle TOF sensor input with gesture detection
void handleTofSensor() {
  unsigned long now = millis();
  
  // Reset reaction after timeout
  if (isReacting && (now - reactionStartTime >= GESTURE_REACTION_MS)) {
    isReacting = false;
    resetEffects();
    roboEyes.setMood(DEFAULT);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setMouthType(1);
    // Reset gesture state - force hand to leave before detecting again
    gestureState = GESTURE_NONE;
    handPresent = false;
    patCount = 0;
    patStartTime = 0;
    Serial.println(F("[GESTURE] Reaction ended, ready to detect"));
  }
  
  // Skip gesture detection while showing reaction
  if (isReacting) return;
  
  // Check if sensor available and has new data ready
  if (!tofAvailable) return;
  if (!tofSensor.isRangeComplete()) return;  // Non-blocking check
  
  // Get the reading (non-blocking since measurement is already done)
  int distanceMm = tofSensor.readRangeResult();
  int distanceCm = distanceMm / 10;
  uint8_t rangeStatus = tofSensor.readRangeStatus();
  bool timeout = tofSensor.timeoutOccurred();

  // Raw debug output for tuning (CSV-friendly)
  if (tofVerbose) {
    Serial.print(now);
    Serial.print(F(","));
    Serial.print(distanceMm);
    Serial.print(F(","));
    Serial.print(distanceCm);
    Serial.print(F(",status="));
    Serial.print(rangeStatus);
    Serial.print(F(",timeout="));
    Serial.print(timeout ? 1 : 0);
    Serial.print(F(",handPresent="));
    Serial.print(handPresent ? 1 : 0);
    Serial.print(F(",isReacting="));
    Serial.print(isReacting ? 1 : 0);
    Serial.print(F(",gstate="));
    Serial.print((int)gestureState);
    Serial.print(F(",patCount="));
    Serial.print(patCount);
    Serial.print(F(",lastDist="));
    Serial.print(lastDistance);
    Serial.print(F(",baseDist="));
    Serial.print(baseDistance);
    // Extra timing & delta fields
    Serial.print(F(",dtLast="));
    Serial.print(now - lastTofRead);
    Serial.print(F(",dtMove="));
    Serial.print(now - lastMovementTime);
    Serial.print(F(",dtHand="));
    Serial.print(now - handDetectedTime);
    Serial.print(F(",dtPatStart="));
    Serial.print(patStartTime ? (now - patStartTime) : -1);
    Serial.print(F(",collecting="));
    Serial.print(collectingForMatch ? 1 : 0);
    Serial.print(F(",matchIdx="));
    Serial.println(matchSampleIndex);
  }
  
  // Check for valid reading (out of range returns 8190+)
  if (distanceMm > 2000) {
    // Invalid reading - check for timeout
    if (handPresent && (now - handDetectedTime > GESTURE_TIMEOUT_MS)) {
      handPresent = false;
      gestureState = GESTURE_NONE;
      patCount = 0;
    }
    return;
  }
  
  // If training mode, pass to gesture trainer
  if (isTraining()) {
    processTrainingSample(distanceMm, distanceCm);
    return;
  }
  
  // If matching mode, collect samples and try to match
  if (isMatchingEnabled() && distanceCm <= GESTURE_DISTANCE_CM && distanceCm >= GESTURE_MIN_CM) {
    if (!collectingForMatch) {
      // Start collecting samples
      collectingForMatch = true;
      matchSampleIndex = 0;
      lastMatchSampleTime = now;
    }
    
    if (now - lastMatchSampleTime >= SAMPLE_INTERVAL_MS && matchSampleIndex < SAMPLE_COUNT) {
      matchSamples[matchSampleIndex++] = distanceMm;
      lastMatchSampleTime = now;
      
      if (matchSampleIndex >= SAMPLE_COUNT) {
        // Try to match
        String action = matchGesture(matchSamples);
        if (action.length() > 0) {
          executeGestureAction(action);
        }
        collectingForMatch = false;
        matchSampleIndex = 0;
      }
    }
    return;  // Don't do normal gesture detection when matching
  } else if (collectingForMatch && distanceCm > GESTURE_DISTANCE_CM) {
    // Hand left range, reset
    collectingForMatch = false;
    matchSampleIndex = 0;
  }
  
  // Check if hand is in gesture range (built-in pat/rub detection)
  if (distanceCm <= GESTURE_DISTANCE_CM && distanceCm >= GESTURE_MIN_CM) {
    
    if (!handPresent) {
      // Hand just entered range - initialize
      handPresent = true;
      handDetectedTime = now;
      lastMovementTime = now;
      lastDistance = distanceMm;
      baseDistance = distanceMm;  // Reference for rub
      patCount = 0;
      patStartTime = 0;
      gestureDirection = 0;
      gestureState = GESTURE_DETECTING;
      Serial.print(F("[GESTURE] Hand entered at "));
      Serial.print(distanceCm);
      Serial.println(F("cm - detecting"));
      if (gestVerbose) {
        Serial.print(F("[GESTURE-RAW] baseDist="));
        Serial.print(baseDistance);
        Serial.print(F(",lastDist="));
        Serial.println(lastDistance);
      }
    } else if (gestureState == GESTURE_DETECTING) {
      // Hand is present, analyze movement
      int deltaFromLast = distanceMm - lastDistance;
      int deltaFromBase = abs(distanceMm - baseDistance);
      if (gestVerbose) {
        Serial.print(F("[GESTURE-RAW] deltaFromLast="));
        Serial.print(deltaFromLast);
        Serial.print(F(",deltaFromBase="));
        Serial.print(deltaFromBase);
        Serial.print(F(",gdir="));
        Serial.println(gestureDirection);
      }
      
      // Check for significant movement (pat detection)
      if (abs(deltaFromLast) > PAT_THRESHOLD) {
        int newDirection = (deltaFromLast > 0) ? 1 : -1;  // 1 = away, -1 = closer
        
        // First movement - start pat window
        if (patCount == 0) {
          patStartTime = now;
          patCount = 1;
          gestureDirection = newDirection;
        }
        // Direction changed = pat movement
        else if (newDirection != gestureDirection) {
          // Check if within time window
          if (now - patStartTime <= PAT_WINDOW_MS) {
            patCount++;
            
              if (patCount >= PAT_COUNT_THRESHOLD) {
                gestureState = GESTURE_PAT;
                if (gestVerbose) {
                  Serial.print(F("[GESTURE] PAT triggered patCount="));
                  Serial.print(patCount);
                  Serial.print(F(",now-patStart="));
                  Serial.println(now - patStartTime);
                }
                reactToGesture(GESTURE_PAT);
                patCount = 0;
                patStartTime = 0;
              }
          } else {
            // Took too long, restart
            patCount = 1;
            patStartTime = now;
          }
          gestureDirection = newDirection;
        }
        
        lastMovementTime = now;
        lastDistance = distanceMm;
        baseDistance = distanceMm;  // Reset base after movement
      } 
      // Hand is relatively steady
      else if (deltaFromBase <= RUB_TOLERANCE) {
        // Check for rub (steady for long enough)
        if (now - lastMovementTime >= RUB_TIME_MS) {
          gestureState = GESTURE_RUB;
          if (gestVerbose) {
            Serial.print(F("[GESTURE] RUB triggered dt="));
            Serial.println(now - lastMovementTime);
          }
          reactToGesture(GESTURE_RUB);
        }
        // Don't update lastDistance when steady (prevents drift)
      }
      // Small movement - update base but don't count as pat
      else {
        baseDistance = distanceMm;
        lastDistance = distanceMm;
      }
    }
  } else {
    // Hand out of range
    if (handPresent) {
      handPresent = false;
      gestureState = GESTURE_NONE;
      patCount = 0;
      patStartTime = 0;
      Serial.println(F("[GESTURE] Hand left range"));
    }
  }
}

// Handle serial input
void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        handleCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}
