/*
 * leor - Animated Robot Eyes for ESP32-C3
 * 
 * Features:
 * - Animated OLED robot eyes with expressions
 * - BLE (Bluetooth Low Energy) control interface
 * - Serial command interface
 * - MPU6050 accelerometer for gesture detection
 * - Gesture training and recognition
 * - Low power consumption with BLE
 * 
 * Hardware:
 * - ESP32-C3 Super Mini
 * - SH1106G 128x64 OLED display (I2C)
 * - MPU6050 Accelerometer/Gyroscope
 */

// ==================== Libraries ====================
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <MPU6050.h>  // i2cdevlib MPU6050
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

// ==================== Configuration ====================
#include "config.h"

// ==================== Hardware Objects ====================
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MPU6050 mpu;  // i2cdevlib MPU6050
Preferences preferences;

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
#include "ble_manager.h"

// ==================== State Variables ====================
String inputBuffer = "";
bool mpuAvailable = false;
unsigned long lastMpuRead = 0;
unsigned long reactionStartTime = 0;
bool isReacting = false;

// Random expression shuffle
bool shuffleEnabled = true;  // ON by default
uint32_t shuffleExprMinMs = 2000;   // expression min 2s
uint32_t shuffleExprMaxMs = 5000;   // expression max 5s
uint32_t shuffleNeutralMinMs = 2000; // neutral min 2s
uint32_t shuffleNeutralMaxMs = 5000; // neutral max 5s
int lastShuffleIndex = -1;
bool shuffleNeedsInit = true;  // init on boot

enum ShufflePhase {
  SHUFFLE_NEUTRAL,
  SHUFFLE_EXPRESSION
};
ShufflePhase shufflePhase = SHUFFLE_NEUTRAL;
unsigned long shuffleNextChangeMs = 0;

// MPU6050 verbose logging
bool mpuVerbose = false;

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  delay(250);

  randomSeed((uint32_t)micros());
  
  // Initialize Preferences
  preferences.begin("leor", false);

  Serial.println(F("\n============================="));
  Serial.println(F("leor v2.4 (Action Fix)"));
  Serial.println(F("=============================\n"));
  
  // Initialize I2C bus first (for display + MPU6050)
  Wire.begin();
  
  // Initialize display
  display.begin(I2C_ADDRESS, true);
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
  
  // Load settings from Preferences
  int ew = preferences.getInt("ew", 36);
  int eh = preferences.getInt("eh", 36);
  int es = preferences.getInt("es", 10);
  int er = preferences.getInt("er", 8);
  int mw = preferences.getInt("mw", 20);
  int lt = preferences.getInt("lt", 1000);
  int vt = preferences.getInt("vt", 2000);
  
  // If blink interval (bi) was saved as int (seconds * 10 or similar? No, let's store as int for simplicity or float bytes, but simpler to use int for now if possible or just handle default).
  // pRoboEyes uses float for interval? No, setAutoblinker takes 'byte interval'.
  int bi = preferences.getInt("bi", 3);
  
  roboEyes.setWidth(ew, ew);
  roboEyes.setHeight(eh, eh);
  roboEyes.setSpacebetween(es);
  roboEyes.setBorderradius(er, er);
  roboEyes.setMouthSize(mw, 6);
  roboEyes.setLaughDuration(lt);
  roboEyes.setLoveDuration(vt);
  roboEyes.setAutoblinker(ON, bi, BLINK_VARIATION);
  
  Serial.println(F("✓ Display initialized & Settings loaded"));
  
  // Load shuffle settings
  shuffleEnabled = preferences.getBool("shuf_en", true);
  shuffleExprMinMs = preferences.getUInt("shuf_emin", 2000);
  shuffleExprMaxMs = preferences.getUInt("shuf_emax", 5000);
  shuffleNeutralMinMs = preferences.getUInt("shuf_nmin", 2000);
  shuffleNeutralMaxMs = preferences.getUInt("shuf_nmax", 5000);
  
  // Initialize BLE (low power alternative to WiFi)
  initBLE(BLE_DEVICE_NAME);
  
  // Initialize MPU6050 sensor (i2cdevlib)
  // Wire already initialized above
  
  // DEBUG: Manually check WHO_AM_I
  Wire.beginTransmission(0x68);
  Wire.write(0x75); // WHO_AM_I register
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  if (Wire.available()) {
    byte who = Wire.read();
    Serial.print(F("MPU6050 WHO_AM_I: 0x"));
    Serial.println(who, HEX);
  } else {
    Serial.println(F("MPU6050 WHO_AM_I read failed"));
  }

  mpu.initialize();
  
  // Strict check failed (0x72 != 0x68), but we might have a clone
  if (mpu.testConnection()) {
    mpuAvailable = true;
    Serial.println(F("✓ MPU6050 ready"));
  } else {
    // Retry: if we saw ANY valid WHO_AM_I, force enable
    Wire.beginTransmission(0x68);
    Wire.write(0x75);
    Wire.endTransmission();
    Wire.requestFrom(0x68, 1);
    if (Wire.available()) {
      byte who = Wire.read();
      if (who != 0x00 && who != 0xFF) {
        mpuAvailable = true;
        Serial.print(F("! MPU6050 ID mismatch (0x"));
        Serial.print(who, HEX);
        Serial.println(F(") but device responsive. Forcing ON."));
      } else {
        mpuAvailable = false;
        Serial.println(F("✗ MPU6050 not found"));
      }
    } else {
      mpuAvailable = false;
      Serial.println(F("✗ MPU6050 not found"));
    }
  }
  
  // Initialize gesture trainer
  initGestureTrainer();
  
  // Ready!
  roboEyes.setMood(DEFAULT);
  roboEyes.setPosition(DEFAULT);
  
  Serial.println(F("\n============================="));
  Serial.println(F("    leor Ready!"));
  Serial.println(F("============================="));
  Serial.println(F("Type 'help' for commands\n"));
}

// ==================== Main Loop ====================
void loop() {
  // Handle BLE connection state
  handleBLEConnection();
  
  // Check if in training mode
  bool training = isTraining();
  
  // Update display - skip roboEyes when training
  if (!training) {
    roboEyes.update();
  }
  
  // Draw streaming overlay if in streaming mode
  if (isStreaming()) {
    drawStreamingOverlay();
  }
  
  // Handle MPU6050 sensor for gesture detection
  handleMPU6050();

  // Randomly shuffle expressions (skip during training)
  if (!training) {
    maybeShuffleExpression();
  }
  
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
    shuffleNextChangeMs = now + (unsigned long)random(shuffleNeutralMinMs, shuffleNeutralMaxMs + 1);
    return;
  }

  // Expressions that are not neutral/idle
  static const char* kExpressions[] = {
    "happy", "sad", "angry", "love", "surprised", "confused",
    "sleepy", "curious", "nervous", "knocked"
  };
  const int count = (int)(sizeof(kExpressions) / sizeof(kExpressions[0]));
  int idx = random(count);
  if (count > 1 && idx == lastShuffleIndex) {
    idx = (idx + 1 + random(count - 1)) % count;
  }
  lastShuffleIndex = idx;

  handleCommand(String(kExpressions[idx]));

  // Show expression briefly, then back to neutral
  shufflePhase = SHUFFLE_EXPRESSION;
  shuffleNextChangeMs = now + (unsigned long)random(shuffleExprMinMs, shuffleExprMaxMs + 1);
}

// ==================== Helper Functions ====================

// Draw streaming overlay on OLED - shows when streaming gyro to browser
void drawStreamingOverlay() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  
  // Header
  display.setTextSize(2);
  display.setCursor(10, 2);
  display.print(F("STREAMING"));
  
  // Line
  display.drawLine(0, 20, SCREEN_WIDTH, 20, SH110X_WHITE);
  
  // Blinking indicator
  display.setTextSize(1);
  if ((millis() / 300) % 2 == 0) {
    display.fillCircle(64, 40, 8, SH110X_WHITE);
  } else {
    display.drawCircle(64, 40, 8, SH110X_WHITE);
  }
  
  display.setCursor(30, 54);
  display.print(F("Recording..."));
  
  display.display();
}

// Execute action from matched gesture
void executeGestureAction(const String& action) {
  isReacting = true;
  reactionStartTime = millis();
  Serial.print(F("[GESTURE] Matched! Action: "));
  Serial.println(action);
  
  // Send BLE notification
  sendBLEGesture("matched:" + action);
  
  handleCommand(action);
}

// Handle MPU6050 sensor input with gesture detection
void handleMPU6050() {
  unsigned long now = millis();
  
  // Reset reaction after timeout
  if (isReacting && (now - reactionStartTime >= GESTURE_REACTION_MS)) {
    isReacting = false;
    resetEffects();
    roboEyes.setMood(DEFAULT);
    roboEyes.setPosition(DEFAULT);
    roboEyes.setMouthType(1);
    Serial.println(F("[GESTURE] Reaction ended, ready to detect"));
  }
  
  // Skip gesture detection while showing reaction
  if (isReacting) return;
  
  // Default values if no MPU
  float magnitude = 0;
  int magnitudeInt = 0;
  float gxf = 0, gyf = 0, gzf = 0;
  

  
  // Read MPU if available
  if (mpuAvailable) {
    // Rate limit reads
    if (now - lastMpuRead >= MPU_SAMPLE_RATE_MS) {
      lastMpuRead = now;
      
      // Read gyro data using i2cdevlib
      int16_t ax, ay, az, gx, gy, gz;
      mpu.getAcceleration(&ax, &ay, &az);
      mpu.getRotation(&gx, &gy, &gz);
      
      // Convert raw gyro values to rad/s (sensitivity: 131 LSB/(deg/s) at ±250°/s range)
      float gyroScale = (1.0f / 131.0f) * (3.14159265f / 180.0f);
      gxf = gx * gyroScale;
      gyf = gy * gyroScale;
      gzf = gz * gyroScale;
      
      // Calculate magnitude of angular velocity (gyroscope)
      magnitude = sqrt(gxf * gxf + gyf * gyf + gzf * gzf);
      magnitudeInt = (int)(magnitude * 100);
      
      // Debug output
      if (mpuVerbose) {
        Serial.print(now);
        Serial.print(F(",mag="));
        Serial.print(magnitude);
        Serial.print(F(",gx="));
        Serial.print(gxf);
        Serial.print(F(",gy="));
        Serial.print(gyf);
        Serial.print(F(",gz="));
        Serial.print(gzf);
        Serial.print(F(",streaming="));
        Serial.println(isStreaming() ? 1 : 0);
      }
      
      // Handle streaming mode (sends data to browser) - INSIDE the read block
      if (isStreaming()) {

        
        processGyroForStreaming(gxf, gyf, gzf);
      }
      
      // Handle inference mode (runs local NN) - INSIDE the read block
      if (isMatchingEnabled()) {
        processGyroForInference(gxf, gyf, gzf);
      }
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
