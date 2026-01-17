/*
 * ei_gesture.h - Edge Impulse Gesture Recognition for leor
 * 
 * Uses the Edge Impulse SDK with a pre-trained gesture model.
 * Model: leor (5 classes: neutral, patpat, pickup, shake, swipe)
 * Input: 6 axes (gX, gY, gZ, aX, aY, aZ) @ 23Hz
 */

#ifndef EI_GESTURE_H
#define EI_GESTURE_H

#include <Arduino.h>

// Edge Impulse Arduino library (renamed from ges to leor)
#include <leor_inferencing.h>

// ==================== Configuration (modifiable via BLE) ====================
#define EI_SAMPLE_INTERVAL_MS   43       // ~23Hz (fixed, matches model)

// Tunable parameters (with defaults)
static float ei_confidence_threshold = 0.70f;   // 70% default
static unsigned long ei_cooldown_ms = 2000;     // 2s default

// Reaction time is defined in pio.ino as GESTURE_REACTION_MS, use extern
extern unsigned long GESTURE_REACTION_MS;  // make modifiable

// ==================== State ====================
static float ei_input_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static int ei_sample_index = 0;
static unsigned long ei_last_sample_time = 0;
static unsigned long ei_last_gesture_time = 0;
static bool ei_matching_enabled = false;
static bool ei_streaming = false;

// ==================== Tuning Setters ====================
void setGestureConfidence(int percent) {
    ei_confidence_threshold = (float)percent / 100.0f;
    preferences.putFloat("ges_cf", ei_confidence_threshold);
    ei_printf("Confidence: %d%%\n", percent);
}

void setGestureCooldown(int ms) {
    ei_cooldown_ms = ms;
    preferences.putULong("ges_cd", ei_cooldown_ms);
    ei_printf("Cooldown: %dms\n", ms);
}

void setGestureReactionTime(int ms) {
    GESTURE_REACTION_MS = ms;
    preferences.putULong("ges_rt", GESTURE_REACTION_MS);
    ei_printf("Reaction: %dms\n", ms);
}

// Get current settings (for BLE query)
String getGestureSettings() {
    String resp = "gs:rt=" + String(GESTURE_REACTION_MS) + "\n";
    resp += "cf=" + String((int)(ei_confidence_threshold * 100)) + "\n";
    resp += "cd=" + String(ei_cooldown_ms) + "\n";
    resp += "gm=" + String(ei_matching_enabled ? "1" : "0");
    return resp;
}

// ==================== Forward Declarations ====================
extern void sendBLEStatus(const String& status);
extern String handleCommand(String cmd);
extern Preferences preferences;  // Use existing preferences from pio.ino
extern bool isReacting;          // Reaction state from pio.ino
extern unsigned long reactionStartTime;  // Reaction timer from pio.ino

// ==================== Gesture Actions ====================
// Map gesture labels to commands (order matches model labels alphabetically)
// Labels: neutral, patpat, pickup, shake, swipe
// These can be modified at runtime via BLE commands

#define EI_GESTURE_COUNT 5
static String ei_gesture_actions[EI_GESTURE_COUNT] = {
    "",           // 0: neutral - no action
    "happy",      // 1: patpat - show happy expression  
    "curious",    // 2: pickup - show curious expression
    "confused",   // 3: shake - show confused expression
    "surprised"   // 4: swipe - show surprised expression
};

// Set gesture action mapping (called from commands.h)
void setGestureAction(int index, const String& action) {
    if (index >= 0 && index < EI_GESTURE_COUNT) {
        ei_gesture_actions[index] = action;
        ei_printf("Gesture %d action set to: %s\n", index, action.c_str());
        
        // Save to flash
        String key = "ges_" + String(index);
        preferences.putString(key.c_str(), action);
    }
}

// Get gesture action (for debugging)
String getGestureAction(int index) {
    if (index >= 0 && index < EI_GESTURE_COUNT) {
        return ei_gesture_actions[index];
    }
    return "";
}

// ==================== Signal Callback ====================
// Called by Edge Impulse to get raw input data
static int ei_get_signal_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, ei_input_buffer + offset, length * sizeof(float));
    return 0;
}

// ==================== Initialization ====================
void initEIGesture() {
    Serial.println(F("\n=== Edge Impulse Gesture Recognition ==="));
    Serial.print(F("Model: ")); Serial.println(EI_CLASSIFIER_PROJECT_NAME);
    Serial.print(F("Classes: ")); Serial.println(EI_CLASSIFIER_LABEL_COUNT);
    Serial.print(F("  "));
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.print(ei_classifier_inferencing_categories[i]);
        if (i < EI_CLASSIFIER_LABEL_COUNT - 1) Serial.print(", ");
    }
    Serial.println();
    Serial.print(F("Input: ")); Serial.print(EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
    Serial.print(F(" axes, ")); Serial.print(EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    Serial.print(F(" samples @ ")); Serial.print(EI_CLASSIFIER_FREQUENCY);
    Serial.println(F("Hz"));
    Serial.print(F("DSP frame size: ")); Serial.println(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    Serial.println(F("========================================\n"));
    
    // Clear buffer
    memset(ei_input_buffer, 0, sizeof(ei_input_buffer));
    ei_sample_index = 0;
    
    // Load saved gesture mappings from flash
    for (int i = 0; i < EI_GESTURE_COUNT; i++) {
        String key = "ges_" + String(i);
        if (preferences.isKey(key.c_str())) {
            ei_gesture_actions[i] = preferences.getString(key.c_str(), ei_gesture_actions[i]);
            Serial.print(F("  Loaded gesture ")); Serial.print(i);
            Serial.print(F(" -> ")); Serial.println(ei_gesture_actions[i]);
        }
    }
    
    // Load saved tuning parameters
    if (preferences.isKey("ges_cf")) {
        ei_confidence_threshold = preferences.getFloat("ges_cf", ei_confidence_threshold);
    }
    if (preferences.isKey("ges_cd")) {
        ei_cooldown_ms = preferences.getULong("ges_cd", ei_cooldown_ms);
    }
    if (preferences.isKey("ges_rt")) {
        GESTURE_REACTION_MS = preferences.getULong("ges_rt", GESTURE_REACTION_MS);
    }
    Serial.print(F("  Tuning: cf=")); Serial.print((int)(ei_confidence_threshold * 100));
    Serial.print(F("% cd=")); Serial.print(ei_cooldown_ms);
    Serial.print(F("ms rt=")); Serial.print(GESTURE_REACTION_MS); Serial.println(F("ms"));
    
    // Load saved matching state
    ei_matching_enabled = preferences.getBool("ges_match", false);
    if (ei_matching_enabled) {
        Serial.println(F("  Gesture matching: ON (restored)"));
    }
}

// ==================== Run Inference ====================
int runEIInference() {
    // Create signal from buffer
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &ei_get_signal_data;
    
    // Run classifier
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    
    if (res != EI_IMPULSE_OK) {
        ei_printf("ERR: run_classifier failed (%d)\n", res);
        return -1;
    }
    
    // Debug: print all probabilities
    ei_printf("Probs: ");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("%s=%.1f%% ", 
            ei_classifier_inferencing_categories[i],
            result.classification[i].value * 100.0f);
    }
    ei_printf("(DSP:%dms, inf:%dms)\n", 
        result.timing.dsp, result.timing.classification);
    
    // Find best class
    int best_class = -1;
    float best_prob = 0;
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > best_prob) {
            best_prob = result.classification[i].value;
            best_class = i;
        }
    }
    
    // Check threshold and skip "neutral" (class 0)
    if (best_prob >= ei_confidence_threshold && best_class > 0) {
        return best_class;
    }
    
    return -1;  // No confident gesture detection
}

// ==================== Process Sample ====================
// Call this with 6-axis sensor data at ~24Hz
// Input: gyro in °/s (from FastIMU), accel in g (from FastIMU)
void processEISample(float gx, float gy, float gz, float ax, float ay, float az) {
    if (!ei_matching_enabled) return;
    
    unsigned long now = millis();
    
    // Rate limit to match model frequency
    if (now - ei_last_sample_time < EI_SAMPLE_INTERVAL_MS) return;
    ei_last_sample_time = now;
    
    // Cooldown after gesture detection
    if (now - ei_last_gesture_time < ei_cooldown_ms) return;
    
    // Add sample to buffer (interleaved format: gx0,gy0,gz0,ax0,ay0,az0,gx1,...)
    int base_idx = ei_sample_index * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    if (base_idx + 5 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_input_buffer[base_idx + 0] = gx;
        ei_input_buffer[base_idx + 1] = gy;
        ei_input_buffer[base_idx + 2] = gz;
        ei_input_buffer[base_idx + 3] = ax;
        ei_input_buffer[base_idx + 4] = ay;
        ei_input_buffer[base_idx + 5] = az;
    }
    
    ei_sample_index++;
    
    // Buffer full - run inference
    if (ei_sample_index >= EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        int result = runEIInference();
        
        if (result > 0 && result < EI_CLASSIFIER_LABEL_COUNT) {
            ei_last_gesture_time = millis();
            
            const char* gesture = ei_classifier_inferencing_categories[result];
            String action = (result < EI_GESTURE_COUNT) ? ei_gesture_actions[result] : "";
            
            ei_printf(">>> Gesture: %s -> %s\n", gesture, action.c_str());
            
            // Send BLE notification
            sendBLEStatus("gm:" + String(gesture));
            
            // Execute action if defined
            if (action.length() > 0) {
                handleCommand(action);
                
                // Set reaction state so expression auto-resets after timeout
                isReacting = true;
                reactionStartTime = millis();
            }
        }
        
        // Reset buffer
        ei_sample_index = 0;
    }
}

// ==================== Streaming Mode (for data collection) ====================
void startEIStreaming() {
    ei_streaming = true;
    Serial.println(F("EI Streaming started"));
}

void stopEIStreaming() {
    ei_streaming = false;
    Serial.println(F("EI Streaming stopped"));
}

bool isEIStreaming() {
    return ei_streaming;
}

// Stream sample to browser (during data collection)
void streamEISample(float gx, float gy, float gz, float ax, float ay, float az) {
    if (!ei_streaming) return;
    
    unsigned long now = millis();
    static unsigned long last_stream = 0;
    if (now - last_stream < EI_SAMPLE_INTERVAL_MS) return;
    last_stream = now;
    
    String msg = "gd:" + String(gx, 3) + "," + String(gy, 3) + "," + String(gz, 3) + "," +
                        String(ax, 3) + "," + String(ay, 3) + "," + String(az, 3);
    sendBLEStatus(msg);
}

// ==================== Control Functions ====================
void setEIMatchingEnabled(bool enabled) {
    ei_matching_enabled = enabled;
    ei_sample_index = 0;
    memset(ei_input_buffer, 0, sizeof(ei_input_buffer));
    ei_printf("EI Matching: %s\n", enabled ? "ON" : "OFF");
    
    // Save state to flash for persistence
    preferences.putBool("ges_match", enabled);
}

bool isEIMatchingEnabled() {
    return ei_matching_enabled;
}

// Compatibility functions for commands.h
bool isStreaming() { return ei_streaming; }
bool isMatchingEnabled() { return ei_matching_enabled; }
void startStreaming() { startEIStreaming(); }
void stopStreaming() { stopEIStreaming(); }
void setMatchingEnabled(bool e) { setEIMatchingEnabled(e); }

#endif // EI_GESTURE_H
