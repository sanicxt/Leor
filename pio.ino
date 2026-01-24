/*
 * leor - Animated Robot Eyes for ESP32-C3
 * 
 * Copyright (C) 2026 sanicxt
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// ==================== Libraries ====================
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "FastIMU.h"
#include <NimBLEDevice.h>
#include <Preferences.h>

// ==================== Configuration ====================
#include "config.h"

// ==================== Display Type Selection ====================
// Display type is loaded from preferences at boot (requires restart to change)
// Both libraries are included to support runtime selection via preferences
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>

// Display objects - one will be initialized based on preferences
Adafruit_SSD1306* display_ssd1306 = nullptr;
Adafruit_SH1106G* display_sh1106 = nullptr;

// Unified display interface
Adafruit_GFX* display = nullptr;
const char* DISPLAY_TYPE = "Unknown";

// Runtime display type flag
enum DisplayType { DISP_SSD1306, DISP_SH1106 };
DisplayType activeDisplayType = DISP_SH1106;

// ==================== Hardware Objects ====================
Preferences preferences;

// ==================== FastIMU ====================
#define IMU_ADDRESS 0x68

// Use IMU_Generic for counterfeit sensors
IMU_Generic IMU;
calData calib = { 0 };
AccelData accelData;
GyroData gyroData;
bool mpuAvailable = false;
bool mpuCalibrated = false;

// For compatibility with existing code
float dpsGX, dpsGY, dpsGZ;
float gForceAX, gForceAY, gForceAZ;

#define FREQUENCY_HZ 50
#define INTERVAL_MS (1000 / (FREQUENCY_HZ))

static unsigned long last_interval_ms = 0;

// Draw calibration screen on OLED
void drawCalibrationScreen(int progress, const char* status) {
    uint16_t WHITE_COLOR = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;
    
    if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
        display_ssd1306->clearDisplay();
        display_ssd1306->setTextSize(1);
        display_ssd1306->setTextColor(WHITE_COLOR);
        display_ssd1306->setCursor(20, 5);
        display_ssd1306->print(F("IMU CALIBRATION"));
        display_ssd1306->setCursor(10, 25);
        display_ssd1306->print(status);
        display_ssd1306->drawRect(10, 42, 108, 12, WHITE_COLOR);
        int fillWidth = map(progress, 0, 100, 0, 104);
        display_ssd1306->fillRect(12, 44, fillWidth, 8, WHITE_COLOR);
        display_ssd1306->setCursor(50, 56);
        display_ssd1306->print(progress);
        display_ssd1306->print(F("%"));
        display_ssd1306->display();
    } else if (display_sh1106) {
        display_sh1106->clearDisplay();
        display_sh1106->setTextSize(1);
        display_sh1106->setTextColor(WHITE_COLOR);
        display_sh1106->setCursor(20, 5);
        display_sh1106->print(F("IMU CALIBRATION"));
        display_sh1106->setCursor(10, 25);
        display_sh1106->print(status);
        display_sh1106->drawRect(10, 42, 108, 12, WHITE_COLOR);
        int fillWidth = map(progress, 0, 100, 0, 104);
        display_sh1106->fillRect(12, 44, fillWidth, 8, WHITE_COLOR);
        display_sh1106->setCursor(50, 56);
        display_sh1106->print(progress);
        display_sh1106->print(F("%"));
        display_sh1106->display();
    }
}

// Calibrate IMU with OLED display
void calibrateIMU() {
    Serial.println(F("  Calibrating IMU (keep still)..."));
    
    drawCalibrationScreen(0, "Keep device STILL");
    delay(500);
    
    // FastIMU handles calibration internally
    drawCalibrationScreen(20, "Sampling gyro...");
    delay(200);
    
    IMU.calibrateAccelGyro(&calib);
    
    drawCalibrationScreen(80, "Calculating bias...");
    delay(200);
    
    // Re-init with calibration
    IMU.init(calib, IMU_ADDRESS);
    
    mpuCalibrated = true;
    
    drawCalibrationScreen(100, "Calibration done!");
    delay(500);
    
    Serial.println(F(" Done!"));
    Serial.print(F("  Gyro bias: "));
    Serial.print(calib.gyroBias[0], 2); Serial.print(", ");
    Serial.print(calib.gyroBias[1], 2); Serial.print(", ");
    Serial.println(calib.gyroBias[2], 2);
    Serial.print(F("  Accel bias: "));
    Serial.print(calib.accelBias[0], 2); Serial.print(", ");
    Serial.print(calib.accelBias[1], 2); Serial.print(", ");
    Serial.println(calib.accelBias[2], 2);
}

// ==================== MochiEyes ====================
#include "MochiEyes.h"
// Separate MochiEyes instances for each display type
MochiEyes<Adafruit_SSD1306>* pMochiEyes_ssd1306 = nullptr;
MochiEyes<Adafruit_SH1106G>* pMochiEyes_sh1106 = nullptr;

// Helper macro to get the active MochiEyes instance
#define GET_MOCHI() (activeDisplayType == DISP_SSD1306 ? (void*)pMochiEyes_ssd1306 : (void*)pMochiEyes_sh1106)

// ==================== Gesture Training Overlay ====================
// Called by gesture_trainer.h to draw training status on OLED
void drawTrainingOverlay(const char* status, int progress);

// ==================== Modules ====================
#include "ble_manager.h"
#include "commands.h"

// ==================== State Variables ====================
String inputBuffer = "";
unsigned long lastMpuRead = 0;
unsigned long GESTURE_REACTION_MS = 1500;  // Modifiable reaction time (default 1.5s)
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
  
  // Check display preferences and create appropriate object
  String savedDispType = preferences.getString("disp_type", "sh1106");
  uint8_t savedDispAddr = preferences.getUInt("disp_addr", I2C_ADDRESS);
  
  // Determine which display to use and create object
  if (savedDispType == "ssd1306") {
    activeDisplayType = DISP_SSD1306;
    DISPLAY_TYPE = "SSD1306";
    display_ssd1306 = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    display = display_ssd1306;
  } else {
    activeDisplayType = DISP_SH1106;
    DISPLAY_TYPE = "SH1106";
    display_sh1106 = new Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    display = display_sh1106;
  }
  
  Serial.print(F("Display Type: "));
  Serial.print(DISPLAY_TYPE);
  Serial.print(F(" @ 0x"));
  Serial.println(savedDispAddr, HEX);
  
  // Initialize I2C bus first (for display + MPU6050)
  Wire.begin();
  
  // Initialize the selected display
  bool displayInitSuccess = false;
  if (activeDisplayType == DISP_SSD1306) {
    displayInitSuccess = display_ssd1306->begin(SSD1306_SWITCHCAPVCC, savedDispAddr);
  } else {
    displayInitSuccess = display_sh1106->begin(savedDispAddr, true);
  }
  
  if (!displayInitSuccess) {
    Serial.print(F("⚠️  Display init failed at 0x"));
    Serial.println(savedDispAddr, HEX);
    Serial.println(F("Try 0x3C or 0x3D via display:addr command"));
    while(1) { delay(1000); }
  }
  
  // Initialize MochiEyes with the active display
  if (activeDisplayType == DISP_SSD1306) {
    static MochiEyes<Adafruit_SSD1306> mochiEyes(*display_ssd1306);
    pMochiEyes_ssd1306 = &mochiEyes;
    pMochiEyes_ssd1306->begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
  } else {
    static MochiEyes<Adafruit_SH1106G> mochiEyes(*display_sh1106);
    pMochiEyes_sh1106 = &mochiEyes;
    pMochiEyes_sh1106->begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
  }
  
  Serial.print(F("✓ Display initialized ("));
  Serial.print(DISPLAY_TYPE);
  Serial.print(F(" @ 0x"));
  Serial.print(savedDispAddr, HEX);
  Serial.println(F(")"));
  
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
  
  MOCHI_CALL_VOID(setWidth, ew, ew);
  MOCHI_CALL_VOID(setHeight, eh, eh);
  MOCHI_CALL_VOID(setSpacebetween, es);
  MOCHI_CALL_VOID(setBorderradius, er, er);
  MOCHI_CALL_VOID(setMouthSize, mw, 6);
  MOCHI_CALL_VOID(setLaughDuration, lt);
  MOCHI_CALL_VOID(setLoveDuration, vt);
  MOCHI_CALL_VOID(setAutoblinker, true, (float)bi, (float)BLINK_VARIATION);
  
  // Load shuffle settings
  shuffleEnabled = preferences.getBool("shuf_en", true);
  shuffleExprMinMs = preferences.getUInt("shuf_emin", 2000);
  shuffleExprMaxMs = preferences.getUInt("shuf_emax", 5000);
  shuffleNeutralMinMs = preferences.getUInt("shuf_nmin", 2000);
  shuffleNeutralMaxMs = preferences.getUInt("shuf_nmax", 5000);
  
  // Initialize BLE (low power alternative to WiFi)
  initBLE(BLE_DEVICE_NAME);
  
  // Restore BLE power mode
  bool bleLP = preferences.getBool("ble_lp", false);
  setBLELowPowerMode(bleLP);
  
  // Initialize IMU with FastIMU
  Wire.setClock(400000);  // 400kHz I2C for faster IMU reads
  int err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0) {
    Serial.print(F("IMU init error: "));
    Serial.println(err);
    mpuAvailable = false;
  } else {
    mpuAvailable = true;
    Serial.println(F("✓ IMU ready (FastIMU)"));
    
    // Calibrate with OLED display
    calibrateIMU();
  }
  
  // Initialize Edge Impulse gesture recognition
  initEIGesture();
  
  // Ready!
  MOCHI_CALL_VOID(setMood, 0);  // DEFAULT
  MOCHI_CALL_VOID(setPosition, 0);  // CENTER
  
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
  
  // Update display - skip mochiEyes when training
  if (!training) {
    if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) {
      pMochiEyes_ssd1306->update();
    } else if (pMochiEyes_sh1106) {
      pMochiEyes_sh1106->update();
    }
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
  uint16_t WHITE_COLOR = (activeDisplayType == DISP_SSD1306) ? SSD1306_WHITE : SH110X_WHITE;
  
  if (activeDisplayType == DISP_SSD1306 && display_ssd1306) {
    display_ssd1306->clearDisplay();
    display_ssd1306->setTextColor(WHITE_COLOR);
    display_ssd1306->setTextSize(2);
    display_ssd1306->setCursor(10, 2);
    display_ssd1306->print(F("STREAMING"));
    display_ssd1306->drawLine(0, 20, SCREEN_WIDTH, 20, WHITE_COLOR);
    display_ssd1306->setTextSize(1);
    if ((millis() / 300) % 2 == 0) {
      display_ssd1306->fillCircle(64, 40, 8, WHITE_COLOR);
    } else {
      display_ssd1306->drawCircle(64, 40, 8, WHITE_COLOR);
    }
    display_ssd1306->setCursor(30, 54);
    display_ssd1306->print(F("Recording..."));
    display_ssd1306->display();
  } else if (display_sh1106) {
    display_sh1106->clearDisplay();
    display_sh1106->setTextColor(WHITE_COLOR);
    display_sh1106->setTextSize(2);
    display_sh1106->setCursor(10, 2);
    display_sh1106->print(F("STREAMING"));
    display_sh1106->drawLine(0, 20, SCREEN_WIDTH, 20, WHITE_COLOR);
    display_sh1106->setTextSize(1);
    if ((millis() / 300) % 2 == 0) {
      display_sh1106->fillCircle(64, 40, 8, WHITE_COLOR);
    } else {
      display_sh1106->drawCircle(64, 40, 8, WHITE_COLOR);
    }
    display_sh1106->setCursor(30, 54);
    display_sh1106->print(F("Recording..."));
    display_sh1106->display();
  }
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
    if (activeDisplayType == DISP_SSD1306 && pMochiEyes_ssd1306) {
      pMochiEyes_ssd1306->setMood(DEFAULT);
      pMochiEyes_ssd1306->setPosition(DEFAULT);
      pMochiEyes_ssd1306->setMouthType(1);
    } else if (pMochiEyes_sh1106) {
      pMochiEyes_sh1106->setMood(DEFAULT);
      pMochiEyes_sh1106->setPosition(DEFAULT);
      pMochiEyes_sh1106->setMouthType(1);
    }
    Serial.println(F("[GESTURE] Reaction ended, ready to detect"));
  }
  
  // Skip gesture detection while showing reaction
  if (isReacting) return;
  
  // Default values if no MPU
  float magnitude = 0;
  int magnitudeInt = 0;
  float gxf = 0, gyf = 0, gzf = 0;
  
  // ==================== Preprocessing State (persistent) ====================
  
  // EMA low-pass filter for gyro (noise reduction)
  static float filteredGX = 0, filteredGY = 0, filteredGZ = 0;
  static const float EMA_ALPHA = 0.3f;  // 0.1 = very smooth, 0.5 = more responsive
  
  // Moving average buffers for gyro (alternative smoothing)
  static const int GYRO_MA_SIZE = 5;
  static float maBufferX[GYRO_MA_SIZE] = {0};
  static float maBufferY[GYRO_MA_SIZE] = {0};
  static float maBufferZ[GYRO_MA_SIZE] = {0};
  static int maIndex = 0;
  
  // High-pass filter for accelerometer (gravity removal)
  static float hpAccelX = 0, hpAccelY = 0, hpAccelZ = 0;
  static float prevAccelX = 0, prevAccelY = 0, prevAccelZ = 0;
  static const float ACCEL_HP_ALPHA = 0.8f;  // High-pass: 0.8 = strong gravity removal
  
  // Preprocessed accelerometer output (gravity-free)
  static float linearAccelX = 0, linearAccelY = 0, linearAccelZ = 0;
  
  // Read MPU if available
  if (mpuAvailable) {
    // Rate limit reads
    if (now - lastMpuRead >= MPU_SAMPLE_RATE_MS) {
      lastMpuRead = now;
      
      // Read gyro and accel data using FastIMU
      IMU.update();
      IMU.getGyro(&gyroData);
      IMU.getAccel(&accelData);
      
      // FastIMU already applies calibration, just copy to compatibility vars
      dpsGX = gyroData.gyroX;
      dpsGY = gyroData.gyroY;
      dpsGZ = gyroData.gyroZ;
      gForceAX = accelData.accelX;
      gForceAY = accelData.accelY;
      gForceAZ = accelData.accelZ;
      
      // ==================== Gyro Preprocessing ====================
      
      // Convert dps to rad/s (raw values)
      float rawGXf = dpsGX * (PI / 180.0f);
      float rawGYf = dpsGY * (PI / 180.0f);
      float rawGZf = dpsGZ * (PI / 180.0f);
      
      // Method 1: EMA low-pass filter (current default)
      filteredGX = EMA_ALPHA * rawGXf + (1.0f - EMA_ALPHA) * filteredGX;
      filteredGY = EMA_ALPHA * rawGYf + (1.0f - EMA_ALPHA) * filteredGY;
      filteredGZ = EMA_ALPHA * rawGZf + (1.0f - EMA_ALPHA) * filteredGZ;
      
      // Method 2: Moving average (alternative - uncomment to use)
      // maBufferX[maIndex] = rawGXf;
      // maBufferY[maIndex] = rawGYf;
      // maBufferZ[maIndex] = rawGZf;
      // maIndex = (maIndex + 1) % GYRO_MA_SIZE;
      // float maGX = 0, maGY = 0, maGZ = 0;
      // for (int i = 0; i < GYRO_MA_SIZE; i++) {
      //   maGX += maBufferX[i]; maGY += maBufferY[i]; maGZ += maBufferZ[i];
      // }
      // filteredGX = maGX / GYRO_MA_SIZE;
      // filteredGY = maGY / GYRO_MA_SIZE;
      // filteredGZ = maGZ / GYRO_MA_SIZE;
      
      // Use filtered gyro values
      gxf = filteredGX;
      gyf = filteredGY;
      gzf = filteredGZ;
      
      // ==================== Accelerometer High-Pass Filter ====================
      // Removes gravity (DC component), leaving only dynamic acceleration (movement)
      // Formula: hp[n] = alpha * (hp[n-1] + accel[n] - accel[n-1])
      
      hpAccelX = ACCEL_HP_ALPHA * (hpAccelX + gForceAX - prevAccelX);
      hpAccelY = ACCEL_HP_ALPHA * (hpAccelY + gForceAY - prevAccelY);
      hpAccelZ = ACCEL_HP_ALPHA * (hpAccelZ + gForceAZ - prevAccelZ);
      
      prevAccelX = gForceAX;
      prevAccelY = gForceAY;
      prevAccelZ = gForceAZ;
      
      // Linear acceleration (gravity removed) - available for gesture detection
      linearAccelX = hpAccelX;
      linearAccelY = hpAccelY;
      linearAccelZ = hpAccelZ;
      
      // Calculate magnitude of angular velocity (from filtered gyro data)
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
        Serial.print(F(",laX="));
        Serial.print(linearAccelX, 3);
        Serial.print(F(",laY="));
        Serial.print(linearAccelY, 3);
        Serial.print(F(",laZ="));
        Serial.print(linearAccelZ, 3);
        Serial.print(F(",streaming="));
        Serial.println(isStreaming() ? 1 : 0);
      }
      
      // Handle streaming mode (sends data to browser) - INSIDE the read block
      if (isEIStreaming()) {
        // Use °/s gyro and raw g-force accel (same as data forwarder training format)
        streamEISample(dpsGX, dpsGY, dpsGZ, gForceAX, gForceAY, gForceAZ);
      }
      
      // Handle inference mode (runs Edge Impulse NN) - INSIDE the read block
      if (isEIMatchingEnabled()) {
        // IMPORTANT: Must match training data format!
        // Gyro: degrees per second (°/s) - NOT rad/s
        // Accel: raw g-force - NOT high-pass filtered
        processEISample(dpsGX, dpsGY, dpsGZ, gForceAX, gForceAY, gForceAZ);
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
