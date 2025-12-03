/*
 * gesture_trainer.h - Gesture training and recognition for Leora
 * Records TOF sensor patterns and matches them to named gestures
 */

#ifndef GESTURE_TRAINER_H
#define GESTURE_TRAINER_H

#include <Arduino.h>
#include <Preferences.h>

// ==================== Configuration ====================
#define MAX_GESTURES 8           // Max number of custom gestures
#define SAMPLE_COUNT 12          // Number of distance samples per gesture
#define SAMPLE_INTERVAL_MS 80    // Time between samples during recording
#define MATCH_THRESHOLD 0.65     // Minimum similarity score (0-1) to match
#define GESTURE_NAME_LEN 16      // Max chars for gesture name
#define MATCH_COOLDOWN_MS 1500   // Cooldown after a gesture match before next detection

// ==================== Data Structures ====================
struct GesturePattern {
  char name[GESTURE_NAME_LEN];     // Gesture name
  char action[32];                  // Command to execute when matched
  int samples[SAMPLE_COUNT];        // Distance samples (mm)
  int minDist;                      // Min distance in pattern
  int maxDist;                      // Max distance in pattern
  bool valid;                       // Is this gesture slot used?
};

// ==================== Training Mode State ====================
enum TrainState {
  TRAIN_IDLE,           // Not training
  TRAIN_WAITING,        // Waiting for hand to enter range
  TRAIN_COUNTDOWN,      // Countdown before recording
  TRAIN_RECORDING,      // Recording samples
  TRAIN_DONE,           // Recording complete
  TRAIN_MATCHING        // Actively matching against gestures
};

// Training mode variables
TrainState trainState = TRAIN_IDLE;
GesturePattern gestures[MAX_GESTURES];
GesturePattern recordingGesture;
int sampleIndex = 0;
unsigned long trainStartTime = 0;
unsigned long lastSampleTime = 0;
int countdownNum = 3;
String pendingGestureName = "";
String pendingGestureAction = "";
int gestureCount = 0;
bool matchingEnabled = false;
String lastMatchedGesture = "";
unsigned long lastMatchTime = 0;  // For cooldown after match

// ==================== Preferences Storage ====================
Preferences gesturePrefs;

// ==================== OLED Overlay Drawing ====================
// Forward declare - will be provided by main sketch
extern void drawTrainingOverlay(const char* status, int progress);

// ==================== Gesture Functions ====================

// Initialize gesture system
void initGestureTrainer() {
  // Load saved gestures from flash
  gesturePrefs.begin("gestures", true);  // Read-only
  gestureCount = gesturePrefs.getInt("count", 0);
  
  for (int i = 0; i < MAX_GESTURES; i++) {
    gestures[i].valid = false;
  }
  
  for (int i = 0; i < gestureCount && i < MAX_GESTURES; i++) {
    String key = "g" + String(i);
    if (gesturePrefs.isKey(key.c_str())) {
      size_t len = gesturePrefs.getBytesLength(key.c_str());
      if (len == sizeof(GesturePattern)) {
        gesturePrefs.getBytes(key.c_str(), &gestures[i], sizeof(GesturePattern));
      }
    }
  }
  gesturePrefs.end();
  
  Serial.print(F("Gesture trainer: "));
  Serial.print(gestureCount);
  Serial.println(F(" gestures loaded"));
}

// Save all gestures to flash
void saveGestures() {
  gesturePrefs.begin("gestures", false);  // Read-write
  gesturePrefs.putInt("count", gestureCount);
  
  for (int i = 0; i < MAX_GESTURES; i++) {
    String key = "g" + String(i);
    if (gestures[i].valid) {
      gesturePrefs.putBytes(key.c_str(), &gestures[i], sizeof(GesturePattern));
    } else {
      gesturePrefs.remove(key.c_str());
    }
  }
  gesturePrefs.end();
  Serial.println(F("Gestures saved to flash"));
}

// Start recording a new gesture
bool startGestureRecording(const String& name, const String& action) {
  if (trainState != TRAIN_IDLE) {
    return false;  // Already training
  }
  
  // Find empty slot
  int slot = -1;
  for (int i = 0; i < MAX_GESTURES; i++) {
    if (!gestures[i].valid) {
      slot = i;
      break;
    }
    // Or overwrite existing with same name
    if (strcmp(gestures[i].name, name.c_str()) == 0) {
      slot = i;
      break;
    }
  }
  
  if (slot == -1) {
    Serial.println(F("No gesture slots available"));
    return false;
  }
  
  // Initialize recording
  pendingGestureName = name;
  pendingGestureAction = action;
  memset(&recordingGesture, 0, sizeof(GesturePattern));
  strncpy(recordingGesture.name, name.c_str(), GESTURE_NAME_LEN - 1);
  strncpy(recordingGesture.action, action.c_str(), 31);
  recordingGesture.minDist = 9999;
  recordingGesture.maxDist = 0;
  
  sampleIndex = 0;
  trainState = TRAIN_WAITING;
  trainStartTime = millis();
  
  Serial.print(F("Training gesture: "));
  Serial.println(name);
  Serial.println(F("Place hand in range to start..."));
  
  return true;
}

// Cancel current recording
void cancelGestureRecording() {
  trainState = TRAIN_IDLE;
  sampleIndex = 0;
  Serial.println(F("Training cancelled"));
}

// Process a distance sample during training
void processTrainingSample(int distanceMm, int distanceCm) {
  unsigned long now = millis();
  
  switch (trainState) {
    case TRAIN_WAITING:
      // Wait for hand to enter range
      if (distanceCm <= 15 && distanceCm >= 3) {
        trainState = TRAIN_COUNTDOWN;
        countdownNum = 3;
        trainStartTime = now;
        Serial.println(F("Hand detected! Starting countdown..."));
      } else if (now - trainStartTime > 30000) {
        // Timeout after 30 seconds
        cancelGestureRecording();
        Serial.println(F("Training timeout"));
      }
      break;
      
    case TRAIN_COUNTDOWN:
      // 3-2-1 countdown
      {
        int elapsed = (now - trainStartTime) / 1000;
        countdownNum = 3 - elapsed;
        if (countdownNum <= 0) {
          trainState = TRAIN_RECORDING;
          sampleIndex = 0;
          lastSampleTime = now;
          Serial.println(F("Recording..."));
        }
      }
      break;
      
    case TRAIN_RECORDING:
      // Record samples at fixed intervals
      if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
        if (distanceCm <= 20 && distanceCm >= 2) {
          recordingGesture.samples[sampleIndex] = distanceMm;
          if (distanceMm < recordingGesture.minDist) recordingGesture.minDist = distanceMm;
          if (distanceMm > recordingGesture.maxDist) recordingGesture.maxDist = distanceMm;
          sampleIndex++;
          lastSampleTime = now;
          
          if (sampleIndex >= SAMPLE_COUNT) {
            // Recording complete
            trainState = TRAIN_DONE;
            recordingGesture.valid = true;
            
            // Save to slot
            for (int i = 0; i < MAX_GESTURES; i++) {
              if (!gestures[i].valid || strcmp(gestures[i].name, recordingGesture.name) == 0) {
                memcpy(&gestures[i], &recordingGesture, sizeof(GesturePattern));
                if (!gestures[i].valid || i >= gestureCount) {
                  gestureCount = i + 1;
                }
                gestures[i].valid = true;
                break;
              }
            }
            
            saveGestures();
            Serial.print(F("✓ Gesture '"));
            Serial.print(recordingGesture.name);
            Serial.println(F("' saved!"));
            
            // Return to idle after short delay
            trainStartTime = now;
          }
        } else {
          // Hand left range during recording
          Serial.println(F("Hand lost! Restarting..."));
          trainState = TRAIN_WAITING;
          trainStartTime = now;
          sampleIndex = 0;
        }
      }
      break;
      
    case TRAIN_DONE:
      // Show completion for a moment then return to idle
      if (now - trainStartTime > 2000) {
        trainState = TRAIN_IDLE;
      }
      break;
      
    default:
      break;
  }
}

// Calculate similarity between two gesture patterns (0-1)
float calculateSimilarity(const GesturePattern& pattern, const int* currentSamples) {
  if (!pattern.valid) return 0;
  
  // Normalize both patterns to 0-100 range for comparison
  int patternRange = pattern.maxDist - pattern.minDist;
  if (patternRange < 20) patternRange = 20;  // Minimum range
  
  int currentMin = 9999, currentMax = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    if (currentSamples[i] < currentMin) currentMin = currentSamples[i];
    if (currentSamples[i] > currentMax) currentMax = currentSamples[i];
  }
  int currentRange = currentMax - currentMin;
  if (currentRange < 20) currentRange = 20;
  
  // Compare normalized patterns
  float totalDiff = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float patternNorm = (float)(pattern.samples[i] - pattern.minDist) / patternRange;
    float currentNorm = (float)(currentSamples[i] - currentMin) / currentRange;
    totalDiff += abs(patternNorm - currentNorm);
  }
  
  float avgDiff = totalDiff / SAMPLE_COUNT;
  float similarity = 1.0 - avgDiff;
  return similarity > 0 ? similarity : 0;
}

// Match current samples against saved gestures
// Returns the gesture name if matched, empty string otherwise
String matchGesture(const int* samples) {
  float bestScore = 0;
  int bestIndex = -1;
  
  for (int i = 0; i < MAX_GESTURES; i++) {
    if (gestures[i].valid) {
      float score = calculateSimilarity(gestures[i], samples);
      if (score > bestScore) {
        bestScore = score;
        bestIndex = i;
      }
    }
  }
  
  if (bestScore >= MATCH_THRESHOLD && bestIndex >= 0) {
    Serial.print(F("Matched gesture: "));
    Serial.print(gestures[bestIndex].name);
    Serial.print(F(" (score: "));
    Serial.print(bestScore);
    Serial.println(F(")"));
    lastMatchedGesture = gestures[bestIndex].name;
    lastMatchTime = millis();  // Start cooldown
    return String(gestures[bestIndex].action);
  }
  
  return "";
}

// Delete a gesture by name
bool deleteGesture(const String& name) {
  for (int i = 0; i < MAX_GESTURES; i++) {
    if (gestures[i].valid && strcmp(gestures[i].name, name.c_str()) == 0) {
      gestures[i].valid = false;
      memset(&gestures[i], 0, sizeof(GesturePattern));
      saveGestures();
      
      // Recount valid gestures
      gestureCount = 0;
      for (int j = 0; j < MAX_GESTURES; j++) {
        if (gestures[j].valid) gestureCount++;
      }
      
      Serial.print(F("Deleted gesture: "));
      Serial.println(name);
      return true;
    }
  }
  return false;
}

// Clear all gestures
void clearAllGestures() {
  for (int i = 0; i < MAX_GESTURES; i++) {
    gestures[i].valid = false;
    memset(&gestures[i], 0, sizeof(GesturePattern));
  }
  gestureCount = 0;
  saveGestures();
  Serial.println(F("All gestures cleared"));
}

// List all gestures (returns JSON string) - includes default gestures
String listGestures(bool includeDefaults = true) {
  String json = "[";
  bool first = true;
  
  // Add built-in default gestures first
  if (includeDefaults) {
    json += "{\"name\":\"pat\",\"action\":\"happy\",\"default\":true}";
    json += ",{\"name\":\"rub\",\"action\":\"love\",\"default\":true}";
    first = false;
  }
  
  // Add custom trained gestures
  for (int i = 0; i < MAX_GESTURES; i++) {
    if (gestures[i].valid) {
      if (!first) json += ",";
      json += "{\"name\":\"" + String(gestures[i].name) + "\",";
      json += "\"action\":\"" + String(gestures[i].action) + "\",";
      json += "\"default\":false}";
      first = false;
    }
  }
  json += "]";
  return json;
}

// Get training status string for OLED
String getTrainingStatus() {
  switch (trainState) {
    case TRAIN_WAITING:
      return "WAITING...";
    case TRAIN_COUNTDOWN:
      return String(countdownNum);
    case TRAIN_RECORDING:
      return "REC " + String(sampleIndex) + "/" + String(SAMPLE_COUNT);
    case TRAIN_DONE:
      return "SAVED!";
    default:
      return "";
  }
}

// Get recording progress (0-100)
int getTrainingProgress() {
  if (trainState == TRAIN_RECORDING) {
    return (sampleIndex * 100) / SAMPLE_COUNT;
  } else if (trainState == TRAIN_COUNTDOWN) {
    return 0;
  } else if (trainState == TRAIN_DONE) {
    return 100;
  }
  return -1;  // Not recording
}

// Check if currently training
bool isTraining() {
  return trainState != TRAIN_IDLE;
}

// Enable/disable gesture matching
void setMatchingEnabled(bool enabled) {
  matchingEnabled = enabled;
  Serial.print(F("Gesture matching: "));
  Serial.println(enabled ? F("ON") : F("OFF"));
}

bool isMatchingEnabled() {
  // Include cooldown check
  if (millis() - lastMatchTime < MATCH_COOLDOWN_MS) return false;
  return matchingEnabled && gestureCount > 0;
}

#endif // GESTURE_TRAINER_H
