/*
 * shuffle_manager.h - Auto-expression shuffle for leor
 *
 * Randomly cycles through expressions with configurable timing.
 */

#ifndef SHUFFLE_MANAGER_H
#define SHUFFLE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Forward declarations (defined in commands.h / imu_manager.h)
String handleCommand(String cmd);
bool isTraining();
extern bool isReacting;

// ==================== State ====================
bool     shuffleEnabled     = true;
uint32_t shuffleExprMinMs   = 2000;
uint32_t shuffleExprMaxMs   = 5000;
uint32_t shuffleNeutralMinMs = 2000;
uint32_t shuffleNeutralMaxMs = 5000;
int      lastShuffleIndex   = -1;
bool     shuffleNeedsInit   = true;

enum ShufflePhase { SHUFFLE_NEUTRAL, SHUFFLE_EXPRESSION };
ShufflePhase  shufflePhase       = SHUFFLE_NEUTRAL;
unsigned long shuffleNextChangeMs = 0;

// ==================== Init ====================

void initShuffle(Preferences& preferences) {
    shuffleEnabled     = preferences.getBool("shuf_en", true);
    shuffleExprMinMs   = preferences.getUInt("shuf_emin", 2000);
    shuffleExprMaxMs   = preferences.getUInt("shuf_emax", 5000);
    shuffleNeutralMinMs = preferences.getUInt("shuf_nmin", 2000);
    shuffleNeutralMaxMs = preferences.getUInt("shuf_nmax", 5000);
}

// ==================== Shuffle Logic ====================

void maybeShuffleExpression() {
    if (!shuffleEnabled) return;
    if (isReacting) return;
    if (isTraining()) return;

    unsigned long now = millis();

    if (shuffleNeedsInit) {
        shuffleNeedsInit = false;
        shufflePhase = SHUFFLE_NEUTRAL;
        handleCommand(String("neutral"));
        shuffleNextChangeMs = now + 2000;
        return;
    }

    if (shuffleNextChangeMs != 0 && now < shuffleNextChangeMs) return;

    if (shufflePhase == SHUFFLE_EXPRESSION) {
        handleCommand(String("neutral"));
        shufflePhase = SHUFFLE_NEUTRAL;
        shuffleNextChangeMs = now + (unsigned long)random(shuffleNeutralMinMs, shuffleNeutralMaxMs + 1);
        return;
    }

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
    shufflePhase = SHUFFLE_EXPRESSION;
    shuffleNextChangeMs = now + (unsigned long)random(shuffleExprMinMs, shuffleExprMaxMs + 1);
}

#endif // SHUFFLE_MANAGER_H
