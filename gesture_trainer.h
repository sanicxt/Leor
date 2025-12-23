/*
 * gesture_trainer.h - Gesture Recognition with Neural Network
 * 
 * Architecture: Input(150) -> Dense(16) -> ReLU -> Dense(N) -> Softmax
 * 
 * Flow:
 * 1. Browser sends "gs" to start streaming gyro data
 * 2. ESP32 streams "gd:x,y,z" samples via BLE (50 samples @ 40ms = 2 sec window)
 * 3. Browser trains TensorFlow.js model
 * 4. Browser sends weights via "gw+" chunks + "gw!" to finalize
 * 5. ESP32 runs inference using simple matrix multiply
 */

#ifndef GESTURE_TRAINER_H
#define GESTURE_TRAINER_H

#include <Arduino.h>
#include <Preferences.h>

// ==================== Configuration ====================
#define SAMPLE_COUNT 50          // Samples per gesture (2 sec window)
#define SAMPLE_INTERVAL_MS 40    // Time between samples (25 Hz)
#define INPUT_SIZE 150           // 50 samples * 3 axes
#define HIDDEN_SIZE 16           // Hidden layer neurons
#define OUTPUT_SIZE 5            // Max gesture classes
#define MATCH_COOLDOWN_MS 1000   // Cooldown between detections

// Weight sizes
// W1: INPUT_SIZE * HIDDEN_SIZE = 150 * 16 = 2400
// B1: HIDDEN_SIZE = 16
// W2: HIDDEN_SIZE * OUTPUT_SIZE = 16 * 5 = 80 (max)
// B2: OUTPUT_SIZE = 5 (max)
// Total max: 2400 + 16 + 80 + 5 = 2501 floats

#define MAX_WEIGHTS (INPUT_SIZE * HIDDEN_SIZE + HIDDEN_SIZE + HIDDEN_SIZE * OUTPUT_SIZE + OUTPUT_SIZE)

// ==================== Neural Network Weights ====================
float nn_weights[MAX_WEIGHTS];
bool weights_loaded = false;
int num_gestures = 0;

// ==================== Gesture Labels ====================
char gesture_labels[OUTPUT_SIZE][16] = {
  "gesture0", "gesture1", "gesture2", "gesture3", "gesture4"
};
char gesture_actions[OUTPUT_SIZE][32] = {
  "", "", "", "", ""
};

// ==================== Streaming State ====================
bool is_streaming = false;
unsigned long last_stream_time = 0;
int stream_sample_count = 0;

// ==================== Inference State ====================
bool matching_enabled = false;
float sample_buffer[INPUT_SIZE];  // Rolling buffer for inference
int sample_index = 0;
unsigned long last_sample_time = 0;
unsigned long last_match_time = 0;

// ==================== Weight Transfer ====================
String weight_buffer = "";
bool weight_transfer_active = false;

// ==================== Preferences Storage ====================
Preferences gesturePrefs;

// ==================== Forward Declarations ====================
extern void sendBLEStatus(const String& status);
extern int readGyroData(uint8_t addr, float &gx, float &gy, float &gz);
extern int rawGyroToDPS(float gx, float gy, float gz, float &dpsX, float &dpsY, float &dpsZ);
extern float rawGX, rawGY, rawGZ;
extern float dpsGX, dpsGY, dpsGZ;
extern String handleCommand(String cmd);
#define MPU_ADDRESS 0x68

// ==================== Calibration ====================
float gyroXoffset = 0, gyroYoffset = 0, gyroZoffset = 0;

void calibrateGyro() {
  Serial.println(F("Calibrating Gyro..."));
  float gx_sum = 0, gy_sum = 0, gz_sum = 0;
  const int samples = 100;
  
  for (int i = 0; i < samples; i++) {
    readGyroData(MPU_ADDRESS, rawGX, rawGY, rawGZ);
    rawGyroToDPS(rawGX, rawGY, rawGZ, dpsGX, dpsGY, dpsGZ);
    gx_sum += dpsGX; gy_sum += dpsGY; gz_sum += dpsGZ;
    delay(10);
  }
  
  gyroXoffset = (gx_sum / (float)samples) * (PI / 180.0f);
  gyroYoffset = (gy_sum / (float)samples) * (PI / 180.0f);
  gyroZoffset = (gz_sum / (float)samples) * (PI / 180.0f);
  
  Serial.println(F("Gyro calibration done."));
}

// ==================== Neural Network Inference ====================

// ReLU activation
inline float relu(float x) {
  return x > 0 ? x : 0;
}

// Softmax activation (in-place)
void softmax(float* output, int size) {
  float maxVal = output[0];
  for (int i = 1; i < size; i++) {
    if (output[i] > maxVal) maxVal = output[i];
  }
  
  float sum = 0;
  for (int i = 0; i < size; i++) {
    output[i] = exp(output[i] - maxVal);
    sum += output[i];
  }
  
  for (int i = 0; i < size; i++) {
    output[i] /= sum;
  }
}

// Run neural network inference
// Returns class index with highest probability, or -1 if no match
int runInference(float* input) {
  if (!weights_loaded || num_gestures == 0) return -1;
  
  // Weight layout in nn_weights:
  // W1: [0 .. INPUT_SIZE*HIDDEN_SIZE-1]
  // B1: [INPUT_SIZE*HIDDEN_SIZE .. INPUT_SIZE*HIDDEN_SIZE+HIDDEN_SIZE-1]
  // W2: [INPUT_SIZE*HIDDEN_SIZE+HIDDEN_SIZE .. ...]
  // B2: [... to end]
  
  const int W1_offset = 0;
  const int B1_offset = INPUT_SIZE * HIDDEN_SIZE;
  const int W2_offset = B1_offset + HIDDEN_SIZE;
  const int B2_offset = W2_offset + HIDDEN_SIZE * num_gestures;
  
  // Layer 1: hidden = ReLU(input @ W1 + B1)
  float hidden[HIDDEN_SIZE];
  for (int h = 0; h < HIDDEN_SIZE; h++) {
    float sum = nn_weights[B1_offset + h];  // bias
    for (int i = 0; i < INPUT_SIZE; i++) {
      // W1 is stored as [INPUT_SIZE, HIDDEN_SIZE] row-major
      sum += input[i] * nn_weights[W1_offset + i * HIDDEN_SIZE + h];
    }
    hidden[h] = relu(sum);
  }
  
  // Layer 2: output = Softmax(hidden @ W2 + B2)
  float output[OUTPUT_SIZE];
  for (int o = 0; o < num_gestures; o++) {
    float sum = nn_weights[B2_offset + o];  // bias
    for (int h = 0; h < HIDDEN_SIZE; h++) {
      sum += hidden[h] * nn_weights[W2_offset + h * num_gestures + o];
    }
    output[o] = sum;
  }
  
  // Apply softmax
  softmax(output, num_gestures);
  
  // Find best class
  int best_class = 0;
  float best_prob = output[0];
  for (int i = 1; i < num_gestures; i++) {
    if (output[i] > best_prob) {
      best_prob = output[i];
      best_class = i;
    }
  }
  
  // Only return if confidence > 50%
  if (best_prob > 0.5f) {
    Serial.print(F("✓ Match: ")); Serial.print(gesture_labels[best_class]);
    Serial.print(F(" (conf: ")); Serial.print(best_prob * 100.0f, 1);
    Serial.println(F("%)"));
    return best_class;
  }
  
  return -1;  // No confident match
}

// ==================== Streaming Functions ====================

void startStreaming() {
  is_streaming = true;
  stream_sample_count = 0;
  last_stream_time = millis();
  Serial.println(F("Gyro streaming started (2s window, 50 samples)"));
}

void stopStreaming() {
  is_streaming = false;
  Serial.print(F("Gyro streaming stopped. Samples sent: "));
  Serial.println(stream_sample_count);
}

bool isStreaming() {
  return is_streaming;
}

// Stream gyro data to browser for training
void processGyroForStreaming(float gx, float gy, float gz) {
  if (!is_streaming) return;
  
  unsigned long now = millis();
  if (now - last_stream_time >= SAMPLE_INTERVAL_MS) {
    String msg = "gd:" + String(gx - gyroXoffset, 3) + "," + 
                        String(gy - gyroYoffset, 3) + "," + 
                        String(gz - gyroZoffset, 3);
    sendBLEStatus(msg);
    last_stream_time = now;
    stream_sample_count++;
  }
}

// ==================== Inference Collection ====================

void processGyroForInference(float gx, float gy, float gz) {
  if (!matching_enabled || !weights_loaded || num_gestures == 0) return;
  if (millis() - last_match_time < MATCH_COOLDOWN_MS) return;
  
  unsigned long now = millis();
  if (now - last_sample_time >= SAMPLE_INTERVAL_MS) {
    // Add calibrated sample to buffer
    // Buffer format: [x0..x49, y0..y49, z0..z49]
    int xi = sample_index;
    int yi = SAMPLE_COUNT + sample_index;
    int zi = 2 * SAMPLE_COUNT + sample_index;
    
    sample_buffer[xi] = gx - gyroXoffset;
    sample_buffer[yi] = gy - gyroYoffset;
    sample_buffer[zi] = gz - gyroZoffset;
    
    sample_index++;
    last_sample_time = now;
    
    if (sample_index >= SAMPLE_COUNT) {
      // Buffer full - run inference
      int result = runInference(sample_buffer);
      if (result >= 0) {
        last_match_time = millis();
        
        // Execute action if defined
        if (strlen(gesture_actions[result]) > 0) {
          sendBLEStatus("gm:" + String(gesture_labels[result]));
          handleCommand(gesture_actions[result]);
        }
      }
      
      sample_index = 0;  // Reset buffer
    }
  }
}

// ==================== Weight Loading ====================

// Decode base64 weights from browser
bool loadWeightsFromBase64(const String& base64data) {
  static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
  int len = base64data.length();
  if (len < 100) {
    Serial.println(F("Weight data too short"));
    return false;
  }
  
  // Clear weights
  memset(nn_weights, 0, sizeof(nn_weights));
  
  int byte_count = 0;
  uint8_t* bytes = (uint8_t*)nn_weights;
  int max_bytes = MAX_WEIGHTS * 4;
  
  for (int i = 0; i < len && byte_count < max_bytes; i += 4) {
    uint32_t n = 0;
    for (int j = 0; j < 4 && i + j < len; j++) {
      char c = base64data[i + j];
      if (c == '=') break;
      const char* p = strchr(b64, c);
      if (p) n = (n << 6) | (p - b64);
    }
    
    if (i + 3 < len && base64data[i+3] != '=') {
      bytes[byte_count++] = (n >> 16) & 0xFF;
      bytes[byte_count++] = (n >> 8) & 0xFF;
      bytes[byte_count++] = n & 0xFF;
    } else if (base64data[i+3] == '=') {
      bytes[byte_count++] = (n >> 16) & 0xFF;
      if (base64data[i+2] != '=') bytes[byte_count++] = (n >> 8) & 0xFF;
    }
  }
  
  int float_count = byte_count / 4;
  Serial.print(F("Loaded ")); Serial.print(float_count); Serial.println(F(" weights"));
  
  weights_loaded = true;
  
  // Save to flash
  gesturePrefs.begin("gestures", false);
  gesturePrefs.putBytes("weights", nn_weights, sizeof(nn_weights));
  gesturePrefs.putInt("num_gestures", num_gestures);
  gesturePrefs.putBytes("labels", gesture_labels, sizeof(gesture_labels));
  gesturePrefs.putBytes("actions", gesture_actions, sizeof(gesture_actions));
  gesturePrefs.end();
  
  Serial.println(F("Weights saved to flash"));
  return true;
}

// Set gesture label and action
void setGestureLabel(int index, const char* label, const char* action) {
  if (index < 0 || index >= OUTPUT_SIZE) return;
  strncpy(gesture_labels[index], label, 15);
  strncpy(gesture_actions[index], action, 31);
  if (index >= num_gestures) num_gestures = index + 1;
  
  gesturePrefs.begin("gestures", false);
  gesturePrefs.putBytes("labels", gesture_labels, sizeof(gesture_labels));
  gesturePrefs.putBytes("actions", gesture_actions, sizeof(gesture_actions));
  gesturePrefs.putInt("num_gestures", num_gestures);
  gesturePrefs.end();
}

// ==================== Chunked Weight Transfer ====================

void startWeightTransfer() {
  weights_loaded = false;
  weight_buffer = "";
  weight_buffer.reserve(15000);
  weight_transfer_active = true;
  Serial.println(F("Weight transfer started"));
}

void appendWeightChunk(const String& chunk) {
  if (!weight_transfer_active) {
    startWeightTransfer();
  }
  weight_buffer += chunk;
  if (weight_buffer.length() % 1000 < 50) {
    Serial.print(F("."));
  }
}

bool finalizeWeights() {
  if (weight_buffer.length() < 100) {
    Serial.println(F("Weight buffer too small"));
    weight_transfer_active = false;
    return false;
  }
  
  Serial.print(F("\nFinalizing ")); Serial.print(weight_buffer.length()); Serial.println(F(" chars..."));
  
  bool result = loadWeightsFromBase64(weight_buffer);
  weight_buffer = "";
  weight_transfer_active = false;
  
  return result;
}

// ==================== Initialization ====================

void initGestureTrainer() {
  gesturePrefs.begin("gestures", true);
  size_t len = gesturePrefs.getBytesLength("weights");
  if (len == sizeof(nn_weights)) {
    gesturePrefs.getBytes("weights", nn_weights, sizeof(nn_weights));
    weights_loaded = true;
    num_gestures = gesturePrefs.getInt("num_gestures", 0);
    if (gesturePrefs.getBytesLength("labels") == sizeof(gesture_labels)) {
      gesturePrefs.getBytes("labels", gesture_labels, sizeof(gesture_labels));
      gesturePrefs.getBytes("actions", gesture_actions, sizeof(gesture_actions));
    }
    Serial.print(F("Loaded NN weights for "));
    Serial.print(num_gestures);
    Serial.println(F(" gestures"));
  } else {
    Serial.println(F("No saved weights found"));
  }
  gesturePrefs.end();
  
  Serial.println(F("Gesture trainer ready (50 samples @ 40ms = 2s window)"));
}

// ==================== Control Functions ====================

void setMatchingEnabled(bool enabled) {
  matching_enabled = enabled;
  sample_index = 0;
  memset(sample_buffer, 0, sizeof(sample_buffer));
  Serial.print(F("Matching: ")); Serial.println(enabled ? F("ON") : F("OFF"));
}

bool isMatchingEnabled() {
  return matching_enabled && weights_loaded;
}

bool isTraining() {
  return is_streaming;
}

// ==================== Utility Functions ====================

String listGestures() {
  String json = "[";
  for (int i = 0; i < num_gestures; i++) {
    if (i > 0) json += ",";
    json += "{\"n\":\"" + String(gesture_labels[i]) + "\",";
    json += "\"a\":\"" + String(gesture_actions[i]) + "\"}";
  }
  json += "]";
  return json;
}

void clearAllGestures() {
  weights_loaded = false;
  num_gestures = 0;
  memset(nn_weights, 0, sizeof(nn_weights));
  memset(gesture_labels, 0, sizeof(gesture_labels));
  memset(gesture_actions, 0, sizeof(gesture_actions));
  gesturePrefs.begin("gestures", false);
  gesturePrefs.clear();
  gesturePrefs.end();
  Serial.println(F("All gestures cleared"));
}

#endif // GESTURE_TRAINER_H
