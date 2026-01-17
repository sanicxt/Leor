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
#include "FastIMU.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>

// ==================== Configuration ====================
#include "config.h"

// ==================== Hardware Objects ====================
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
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
    display.clearDisplay();
    
    // Title
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(20, 5);
    display.print(F("IMU CALIBRATION"));
    
    // Status message
    display.setCursor(10, 25);
    display.print(status);
    
    // Progress bar background
    display.drawRect(10, 42, 108, 12, SH110X_WHITE);
    
    // Progress bar fill
    int fillWidth = map(progress, 0, 100, 0, 104);
    display.fillRect(12, 44, fillWidth, 8, SH110X_WHITE);
    
    // Percentage
    display.setCursor(50, 56);
    display.print(progress);
    display.print(F("%"));
    
    display.display();
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
MochiEyes<Adafruit_SH1106G> mochiEyes(display);
MochiEyes<Adafruit_SH1106G>* pMochiEyes = &mochiEyes;  // Pointer for commands.h

// ==================== Gesture Training Overlay ====================
// Called by gesture_trainer.h to draw training status on OLED
void drawTrainingOverlay(const char* status, int progress);

// ==================== Modules ====================
#include "commands.h"
#include "ble_manager.h"

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
  
  // Initialize I2C bus first (for display + MPU6050)
  Wire.begin();
  
  // Initialize display
  display.begin(I2C_ADDRESS, true);
  mochiEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, FRAME_RATE);
  
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
  
  mochiEyes.setWidth(ew, ew);
  mochiEyes.setHeight(eh, eh);
  mochiEyes.setSpacebetween(es);
  mochiEyes.setBorderradius(er, er);
  mochiEyes.setMouthSize(mw, 6);
  mochiEyes.setLaughDuration(lt);
  mochiEyes.setLoveDuration(vt);
  mochiEyes.setAutoblinker(true, (float)bi, (float)BLINK_VARIATION);
  
  Serial.println(F("✓ Display initialized & Settings loaded"));
  
  // Load shuffle settings
  shuffleEnabled = preferences.getBool("shuf_en", true);
  shuffleExprMinMs = preferences.getUInt("shuf_emin", 2000);
  shuffleExprMaxMs = preferences.getUInt("shuf_emax", 5000);
  shuffleNeutralMinMs = preferences.getUInt("shuf_nmin", 2000);
  shuffleNeutralMaxMs = preferences.getUInt("shuf_nmax", 5000);
  
  // Initialize BLE (low power alternative to WiFi)
  initBLE(BLE_DEVICE_NAME);
  
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
  mochiEyes.setMood(0);  // DEFAULT
  mochiEyes.setPosition(0);  // CENTER
  
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
    mochiEyes.update();
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
    mochiEyes.setMood(DEFAULT);
    mochiEyes.setPosition(DEFAULT);
    mochiEyes.setMouthType(1);
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
