/*
 * gesture_trainer.h - Gesture recognition with browser-based ML training
 * 
 * Flow:
 * 1. Browser sends "gs" to start streaming gyro data
 * 2. ESP32 streams "gd:x,y,z" samples via BLE
 * 3. Browser trains TensorFlow.js model
 * 4. Browser sends "gw=base64weights" to upload model
 * 5. ESP32 runs inference locally
 */

#ifndef GESTURE_TRAINER_H
#define GESTURE_TRAINER_H

#include <Arduino.h>
#include <Preferences.h>

// ==================== Configuration ====================
#define SAMPLE_COUNT 25          // Samples per gesture (~2 sec at 80ms)
#define SAMPLE_INTERVAL_MS 40    // Time between samples (40ms * 25 = 1s)
#define INPUT_SIZE 75            // 25 samples * 3 axes
#define HIDDEN_SIZE 24           // Hidden layer neurons
#define OUTPUT_SIZE 5            // Max gesture classes (5)
#define MATCH_THRESHOLD 0.7f     // Confidence threshold

// ==================== Neural Network Weights ====================
// Model: 75 inputs -> 24 hidden (ReLU) -> 5 outputs (Softmax)
// Weights: (75*24) + 24 + (24*5) + 5 = 1800 + 24 + 120 + 5 = 1949 floats
#define TOTAL_WEIGHTS 1949

float nn_weights[TOTAL_WEIGHTS];  // Stored weights from browser
bool weights_loaded = false;

// Weight layout indices
#define W1_START 0              // 75*24 = 1800 weights
#define B1_START 1800           // 24 biases
#define W2_START 1824           // 24*5 = 120 weights  
#define B2_START 1944           // 5 biases

// ==================== Gesture Labels ====================
char gesture_labels[OUTPUT_SIZE][16] = {
  "gesture0", "gesture1", "gesture2", "gesture3", "gesture4"
};
char gesture_actions[OUTPUT_SIZE][32] = {
  "", "", "", "", ""
};
int num_gestures = 0;

// ==================== Streaming State ====================
bool is_streaming = false;
unsigned long last_stream_time = 0;

// ==================== Chunked Weight Transfer ====================
// Weights arrive as multiple base64 chunks via BLE
String weight_buffer = "";  // Accumulates base64 chunks
bool weight_transfer_active = false;

// ==================== Inference State ====================
bool matching_enabled = false;
float sample_buffer_x[SAMPLE_COUNT];
float sample_buffer_y[SAMPLE_COUNT];
float sample_buffer_z[SAMPLE_COUNT];
int sample_index = 0;
bool buffer_ready = false;
unsigned long last_sample_time = 0;
unsigned long last_match_time = 0;
#define MATCH_COOLDOWN_MS 500

// ==================== Preferences Storage ====================
Preferences gesturePrefs;

// ==================== Forward Declarations ====================
extern void sendBLEStatus(const String& status);

// ==================== Neural Network Inference ====================

// ReLU activation
float relu(float x) {
  return x > 0 ? x : 0;
}

// Run inference on input buffer
// Returns class index or -1 if below threshold
int runInference(float* input) {
  if (!weights_loaded) return -1;
  
  // Layer 1: Input (75) -> Hidden (24) with ReLU
  float hidden[HIDDEN_SIZE];
  for (int j = 0; j < HIDDEN_SIZE; j++) {
    float sum = nn_weights[B1_START + j];  // Bias
    for (int i = 0; i < INPUT_SIZE; i++) {
      sum += input[i] * nn_weights[W1_START + i * HIDDEN_SIZE + j];
    }
    hidden[j] = relu(sum);
  }
  
  // Layer 2: Hidden (24) -> Output (8) with Softmax
  float output[OUTPUT_SIZE];
  float max_val = -1e9;
  for (int j = 0; j < OUTPUT_SIZE; j++) {
    float sum = nn_weights[B2_START + j];  // Bias
    for (int i = 0; i < HIDDEN_SIZE; i++) {
      sum += hidden[i] * nn_weights[W2_START + i * OUTPUT_SIZE + j];
    }
    output[j] = sum;
    if (sum > max_val) max_val = sum;
  }
  
  // Softmax normalization
  float exp_sum = 0;
  for (int j = 0; j < OUTPUT_SIZE; j++) {
    output[j] = exp(output[j] - max_val);  // Subtract max for numerical stability
    exp_sum += output[j];
  }
  for (int j = 0; j < OUTPUT_SIZE; j++) {
    output[j] /= exp_sum;
  }
  
  // Find best class
  int best_class = -1;
  float best_prob = 0;
  for (int j = 0; j < num_gestures && j < OUTPUT_SIZE; j++) {
    if (output[j] > best_prob) {
      best_prob = output[j];
      best_class = j;
    }
  }
  
  // Debug
  Serial.print(F("Inference: "));
  for (int j = 0; j < num_gestures; j++) {
    Serial.print(output[j]); Serial.print(F(" "));
  }
  Serial.println();
  
  if (best_prob >= MATCH_THRESHOLD) {
    return best_class;
  }
  return -1;
}

// ==================== Streaming Functions ====================

void startStreaming() {
  is_streaming = true;
  last_stream_time = millis();
  Serial.println(F("Gyro streaming started"));
}

void stopStreaming() {
  is_streaming = false;
  Serial.println(F("Gyro streaming stopped"));
}

bool isStreaming() {
  return is_streaming;
}

// Called from main loop with gyro data
void processGyroForStreaming(float gx, float gy, float gz) {
  if (!is_streaming) return;
  
  unsigned long now = millis();
  if (now - last_stream_time >= SAMPLE_INTERVAL_MS) {
    // Send sample via BLE: gd:x,y,z
    String msg = "gd:" + String(gx, 3) + "," + String(gy, 3) + "," + String(gz, 3);
    sendBLEStatus(msg);
    last_stream_time = now;
  }
}

// ==================== Inference Collection ====================

void processGyroForInference(float gx, float gy, float gz) {
  if (!matching_enabled || !weights_loaded || num_gestures == 0) return;
  if (millis() - last_match_time < MATCH_COOLDOWN_MS) return;
  
  unsigned long now = millis();
  if (now - last_sample_time >= SAMPLE_INTERVAL_MS) {
    sample_buffer_x[sample_index] = gx;
    sample_buffer_y[sample_index] = gy;
    sample_buffer_z[sample_index] = gz;
    sample_index++;
    last_sample_time = now;
    
    if (sample_index >= SAMPLE_COUNT) {
      // Buffer full - run inference
      float input[INPUT_SIZE];
      int idx = 0;
      for (int i = 0; i < SAMPLE_COUNT; i++) input[idx++] = sample_buffer_x[i];
      for (int i = 0; i < SAMPLE_COUNT; i++) input[idx++] = sample_buffer_y[i];
      for (int i = 0; i < SAMPLE_COUNT; i++) input[idx++] = sample_buffer_z[i];
      
// Forward declaration
extern String handleCommand(String cmd);

// ...

      int result = runInference(input);
      if (result >= 0) {
        Serial.print(F("✓ Matched: ")); Serial.println(gesture_labels[result]);
        last_match_time = millis();
        
        // Execute action if defined
        if (strlen(gesture_actions[result]) > 0) {
          String action = gesture_actions[result];
          sendBLEStatus("gm:" + String(gesture_labels[result]));
          
          // Actually execute the action!
          handleCommand(action); 
        }
      }
      
      sample_index = 0;  // Reset buffer
    }
  }
}

// ==================== Weight Loading ====================

// Decode base64 weights from browser
bool loadWeightsFromBase64(const String& base64data) {
  // Simple base64 decode
  static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
  int len = base64data.length();
  if (len < 100) {
    Serial.println(F("Weight data too short"));
    return false;
  }
  
  // Decode to bytes first
  memset(nn_weights, 0, sizeof(nn_weights)); // Clear buffer first
  
  int byte_count = 0;
  uint8_t* bytes = (uint8_t*)nn_weights;  // Reuse buffer temporarily
  int max_bytes = TOTAL_WEIGHTS * 4;
  
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
  

  
  // Now bytes contain the float data (little-endian)
  // The data is already in nn_weights buffer since we decoded in-place
  
  int expected = TOTAL_WEIGHTS * 4;
  if (byte_count < expected) {
    Serial.print(F("Got ")); Serial.print(byte_count);
    Serial.print(F(" bytes, expected ")); Serial.println(expected);
    // Still try to use what we got
  }
  
  weights_loaded = true;
  Serial.println(F("Weights loaded successfully"));
  

  
  // Save to flash
  gesturePrefs.begin("gestures", false);
  gesturePrefs.putBytes("weights", nn_weights, sizeof(nn_weights));
  gesturePrefs.putInt("num_gestures", num_gestures);
  // Persist labels and actions
  gesturePrefs.putBytes("labels", gesture_labels, sizeof(gesture_labels));
  gesturePrefs.putBytes("actions", gesture_actions, sizeof(gesture_actions));
  gesturePrefs.end();
  
  return true;
}

// Set gesture label and action
void setGestureLabel(int index, const char* label, const char* action) {
  if (index < 0 || index >= OUTPUT_SIZE) return;
  strncpy(gesture_labels[index], label, 15);
  strncpy(gesture_actions[index], action, 31);
  if (index >= num_gestures) num_gestures = index + 1;
  
  // Save metadata update immediately (optional, or rely on deploy)
  gesturePrefs.begin("gestures", false);
  gesturePrefs.putBytes("labels", gesture_labels, sizeof(gesture_labels));
  gesturePrefs.putBytes("actions", gesture_actions, sizeof(gesture_actions));
  gesturePrefs.putInt("num_gestures", num_gestures);
  gesturePrefs.end();
}

// ==================== Chunked Weight Transfer ====================

// Start a new weight transfer (clears buffer)
void startWeightTransfer() {
  weights_loaded = false; // Disable inference explicitly
  weight_buffer = "";
  weight_buffer.reserve(11000); // Pre-allocate to prevent fragmentation
  weight_transfer_active = true;
  Serial.println(F("Weight transfer started"));
}

// Append a chunk of base64 weight data
void appendWeightChunk(const String& chunk) {
  if (!weight_transfer_active) {
    startWeightTransfer();
  }
  weight_buffer += chunk;
  // Reduce verbose logging
  if (weight_buffer.length() % 1000 < 50) {
     Serial.print(F("."));
  }
}

// Finalize weight transfer - decode and load
bool finalizeWeights() {
  if (weight_buffer.length() < 100) {
    Serial.println(F("Weight buffer too small"));
    weight_transfer_active = false;
    return false;
  }
  
  Serial.print(F("Finalizing ")); Serial.print(weight_buffer.length()); Serial.println(F(" chars..."));
  
  bool result = loadWeightsFromBase64(weight_buffer);
  weight_buffer = "";  // Free memory
  weight_transfer_active = false;
  
  return result;
}

// ==================== Initialization ====================

void initGestureTrainer() {
  // Load weights from flash
  gesturePrefs.begin("gestures", true);
  size_t len = gesturePrefs.getBytesLength("weights");
  if (len == sizeof(nn_weights)) {
    gesturePrefs.getBytes("weights", nn_weights, sizeof(nn_weights));
    weights_loaded = true;
    num_gestures = gesturePrefs.getInt("num_gestures", 0);
    // Load labels and actions
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
}

// ==================== Control Functions ====================

void setMatchingEnabled(bool enabled) {
  matching_enabled = enabled;
  sample_index = 0;
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
  gesturePrefs.begin("gestures", false);
  gesturePrefs.clear();
  gesturePrefs.end();
  Serial.println(F("All gestures cleared"));
}

#endif // GESTURE_TRAINER_H
