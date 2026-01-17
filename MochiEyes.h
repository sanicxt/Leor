/*
 * MochiEyes - Parametric Animation Engine for ESP32
 * A soft, squishy, continuous-motion eye animation system
 * 
 * Based on FluxGarage RoboEyes by Dennis Hoelscher
 * Refactored to parametric architecture by Leor project
 * 
 * Copyright (C) 2024-2025 Dennis Hoelscher (original RoboEyes)
 * Copyright (C) 2025-2026 Leor Project (MochiEyes refactor)
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

#ifndef MOCHI_EYES_H
#define MOCHI_EYES_H

#include <Arduino.h>

// ============================================================================
// DISPLAY COLORS
// ============================================================================

static uint8_t BGCOLOR = 0;
static uint8_t MAINCOLOR = 1;

// ============================================================================
// LEGACY CONSTANTS (for backward compatibility with commands.h)
// ============================================================================

// On/Off
#define ON 1
#define OFF 0

// Mood types
#define DEFAULT 0
#define TIRED 1
#define ANGRY 2
#define HAPPY 3

// Position constants (prefixed to avoid ESP-DSP collision)
#define POS_N 1   // north
#define POS_NE 2  // north-east
#define POS_E 3   // east
#define POS_SE 4  // south-east
#define POS_S 5   // south
#define POS_SW 6  // south-west
#define POS_W 7   // west
#define POS_NW 8  // north-west

// ============================================================================
// MOUTH SHAPES
// ============================================================================

enum MouthShape : int8_t {
    MOUTH_SMILE = 0,
    MOUTH_FROWN = 1,
    MOUTH_OPEN = 2,
    MOUTH_OOO = 3,
    MOUTH_FLAT = 4
};

// ============================================================================
// EYE LAYOUT - Immutable base configuration
// ============================================================================

struct EyeLayout {
    int16_t screenW;
    int16_t screenH;
    int16_t baseWidth;
    int16_t baseHeight;
    int16_t spacing;
    int16_t borderRadius;
    
    // Mouth base
    int16_t mouthWidth;
    int16_t mouthHeight;
    
    // Computed at init (cached for performance)
    int16_t centerX;
    int16_t centerY;
    int16_t leftEyeBaseX;
    int16_t rightEyeBaseX;
    int16_t eyeBaseY;
    
    void recompute() {
        centerX = screenW / 2;
        centerY = screenH / 2;
        
        int16_t totalWidth = baseWidth + spacing + baseWidth;
        leftEyeBaseX = (screenW - totalWidth) / 2;
        rightEyeBaseX = leftEyeBaseX + baseWidth + spacing;
        eyeBaseY = (screenH - baseHeight) / 2;
    }
};

// ============================================================================
// EYE PARAMS - Time-driven parameters (animated via impulses)
// ============================================================================

struct EyeParams {
    // Core eye state (0.0 – 1.0)
    float openness;         // 0 = closed, 1 = fully open
    float leftOpenness;     // Individual eye openness (for wink)
    float rightOpenness;
    
    // Squash/stretch (0.8 – 1.3, 1.0 = normal)
    float squish;
    
    // Gaze direction (-1.0 to 1.0)
    float gazeX;
    float gazeY;
    
    // Emotion blend weights (0.0 – 1.0)
    float joy;
    float anger;
    float fatigue;
    float love;
    
    // Mouth
    float mouthOpenness;    // 0 = closed, 1 = open
    MouthShape mouthShape;
    
    // Special effects
    float heartScale;       // 0 = no heart, 1 = full size
    float heartPulse;       // For pulsing animation
    float tearProgress;     // 0 = no tears, >0 = tears falling
    float spiralAngle;      // For knocked/dizzy effect
    bool sweat;
    bool cyclops;
    bool curious;           // One eye bigger, moves left-right
    float curiousPhase;     // Animation phase for left-right movement
    
    // Flicker effects (frame-local, not smoothed)
    float hFlicker;
    float vFlicker;
    
    void reset() {
        openness = 1.0f;
        leftOpenness = 1.0f;
        rightOpenness = 1.0f;
        squish = 1.0f;
        gazeX = 0.0f;
        gazeY = 0.0f;
        joy = 0.0f;
        anger = 0.0f;
        fatigue = 0.0f;
        love = 0.0f;
        mouthOpenness = 0.0f;
        mouthShape = MOUTH_SMILE;
        heartScale = 0.0f;
        heartPulse = 0.0f;
        tearProgress = 0.0f;
        spiralAngle = 0.0f;
        sweat = false;
        cyclops = false;
        curious = false;
        curiousPhase = 0.0f;
        hFlicker = 0.0f;
        vFlicker = 0.0f;
    }
};

// ============================================================================
// IMPULSE TARGETS - Smooth animation targets
// ============================================================================

struct ImpulseTargets {
    float openness;
    float leftOpenness;
    float rightOpenness;
    float squish;
    float gazeX;
    float gazeY;
    float joy;
    float anger;
    float fatigue;
    float love;
    float mouthOpenness;
    float heartScale;
    
    // Animation speeds (units per second)
    float opennessSpeed;
    float squishSpeed;
    float gazeSpeed;
    float emotionSpeed;
    float mouthSpeed;
    float heartSpeed;
    
    void reset() {
        openness = 1.0f;
        leftOpenness = 1.0f;
        rightOpenness = 1.0f;
        squish = 1.0f;
        gazeX = 0.0f;
        gazeY = 0.0f;
        joy = 0.0f;
        anger = 0.0f;
        fatigue = 0.0f;
        love = 0.0f;
        mouthOpenness = 0.0f;
        heartScale = 0.0f;
        
        opennessSpeed = 12.0f;   // Faster blink
        squishSpeed = 10.0f;     // Snappier squish
        gazeSpeed = 6.0f;        // Faster gaze
        emotionSpeed = 5.0f;     // Faster emotion transitions
        mouthSpeed = 15.0f;      // Snappier mouth
        heartSpeed = 8.0f;       // Faster heart transition
    }
};

// ============================================================================
// RENDER STATE - Frame-local computed values (no heap allocation)
// ============================================================================

struct RenderState {
    // Left eye computed rect
    int16_t leftX, leftY, leftW, leftH;
    // Right eye computed rect  
    int16_t rightX, rightY, rightW, rightH;
    // Mouth computed rect
    int16_t mouthX, mouthY, mouthW, mouthH;
    // Current border radius (may vary with squish)
    uint8_t borderRadius;
};

// ============================================================================
// ANIMATION TIMERS
// ============================================================================

struct AnimationTimers {
    // Timed animations
    float loveRemaining;
    float cryRemaining;
    float confusedRemaining;
    float laughRemaining;
    float knockedActive;  // 0 = off, >0 = on
    
    // Auto-blink
    float blinkCooldown;
    float blinkInterval;
    float blinkVariation;
    bool autoBlink;
    
    // Idle mode
    float idleCooldown;
    float idleInterval;
    float idleVariation;
    bool idleMode;
    
    void reset() {
        loveRemaining = 0.0f;
        cryRemaining = 0.0f;
        confusedRemaining = 0.0f;
        laughRemaining = 0.0f;
        knockedActive = 0.0f;
        
        blinkCooldown = 2.0f;
        blinkInterval = 3.0f;
        blinkVariation = 3.0f;
        autoBlink = true;
        
        idleCooldown = 0.0f;
        idleInterval = 2.0f;
        idleVariation = 3.0f;
        idleMode = false;
    }
};

// ============================================================================
// MOCHI EYES CLASS
// ============================================================================

template<typename AdafruitDisplay>
class MochiEyes {
private:
    AdafruitDisplay* display;
    
    EyeLayout layout;
    EyeParams params;
    ImpulseTargets targets;
    RenderState render;
    AnimationTimers timers;
    
    uint16_t frameInterval;  // ms between frames
    unsigned long lastFrameMs;
    
    // Sweat drop state (statically allocated)
    float sweatY[3];
    float sweatX[3];
    float sweatSize[3];
    
    // ========================================================================
    // SMOOTHING UTILITIES
    // ========================================================================
    
    static float lerp(float current, float target, float speed, float dt) {
        float diff = target - current;
        float delta = speed * dt;
        if (fabsf(diff) <= delta) {
            return target;
        }
        return current + (diff > 0 ? delta : -delta);
    }
    
    static float smoothDamp(float current, float target, float speed, float dt) {
        // Exponential easing for smoother motion
        float t = 1.0f - expf(-speed * dt);
        return current + (target - current) * t;
    }
    
    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
    
    // ========================================================================
    // PARAMETER UPDATE (called every frame)
    // ========================================================================
    
    void updateParams(float dt) {
        // Smooth all parameters toward targets
        params.openness = smoothDamp(params.openness, targets.openness, targets.opennessSpeed, dt);
        params.leftOpenness = smoothDamp(params.leftOpenness, targets.leftOpenness, targets.opennessSpeed, dt);
        params.rightOpenness = smoothDamp(params.rightOpenness, targets.rightOpenness, targets.opennessSpeed, dt);
        params.squish = smoothDamp(params.squish, targets.squish, targets.squishSpeed, dt);
        params.gazeX = smoothDamp(params.gazeX, targets.gazeX, targets.gazeSpeed, dt);
        params.gazeY = smoothDamp(params.gazeY, targets.gazeY, targets.gazeSpeed, dt);
        
        params.joy = smoothDamp(params.joy, targets.joy, targets.emotionSpeed, dt);
        params.anger = smoothDamp(params.anger, targets.anger, targets.emotionSpeed, dt);
        params.fatigue = smoothDamp(params.fatigue, targets.fatigue, targets.emotionSpeed, dt);
        params.love = smoothDamp(params.love, targets.love, targets.emotionSpeed, dt);
        
        params.mouthOpenness = smoothDamp(params.mouthOpenness, targets.mouthOpenness, targets.mouthSpeed, dt);
        params.heartScale = smoothDamp(params.heartScale, targets.heartScale, targets.heartSpeed, dt);
    }
    
    // ========================================================================
    // ANIMATION TIMERS UPDATE
    // ========================================================================
    
    void updateTimers(float dt) {
        // Love animation
        if (timers.loveRemaining > 0) {
            timers.loveRemaining -= dt;
            targets.love = 1.0f;
            targets.heartScale = 1.0f;
            params.heartPulse += dt * 10.0f;  // Faster pulse frequency
            params.mouthShape = MOUTH_SMILE;
        } else if (params.love > 0.01f) {
            targets.love = 0.0f;
            targets.heartScale = 0.0f;
        }
        
        // Cry animation
        if (timers.cryRemaining > 0) {
            timers.cryRemaining -= dt;
            params.tearProgress += dt * 40.0f;  // Tear speed
            if (params.tearProgress > layout.screenH) {
                params.tearProgress = 0.0f;
            }
            params.mouthShape = MOUTH_FROWN;
            targets.fatigue = 0.5f;
        } else {
            params.tearProgress = 0.0f;
        }
        
        // Confused animation - eyes shake left AND right
        if (timers.confusedRemaining > 0) {
            timers.confusedRemaining -= dt;
            // Stronger horizontal flicker - shakes both directions
            params.hFlicker = sinf(timers.confusedRemaining * 50.0f) * 8.0f;  // Faster, wider shake
            params.mouthShape = MOUTH_OOO;
        } else {
            params.hFlicker = 0.0f;
        }
        
        // Laugh animation
        if (timers.laughRemaining > 0) {
            timers.laughRemaining -= dt;
            // Vertical bounce + mouth animation
            params.vFlicker = sinf(timers.laughRemaining * 20.0f) * 2.0f;
            targets.mouthOpenness = (sinf(timers.laughRemaining * 12.0f) + 1.0f) * 0.5f;
            params.mouthShape = MOUTH_SMILE;
            targets.joy = 1.0f;
        } else {
            params.vFlicker = 0.0f;
            if (timers.laughRemaining < 0 && timers.laughRemaining > -0.1f) {
                targets.mouthOpenness = 0.0f;
                targets.joy = 0.0f;
            }
        }
        
        // Knocked (spiral eyes) - persists until turned off
        if (timers.knockedActive > 0) {
            params.spiralAngle += dt * 5.0f;
            params.mouthShape = MOUTH_OOO;
        }
        
        // Auto-blink
        if (timers.autoBlink && timers.knockedActive <= 0) {
            timers.blinkCooldown -= dt;
            if (timers.blinkCooldown <= 0) {
                blink();
                timers.blinkCooldown = timers.blinkInterval + 
                    ((float)random(100) / 100.0f) * timers.blinkVariation;
            }
        }
        
        // Idle mode - random gaze
        if (timers.idleMode) {
            timers.idleCooldown -= dt;
            if (timers.idleCooldown <= 0) {
                targets.gazeX = ((float)random(200) - 100.0f) / 100.0f;
                targets.gazeY = ((float)random(200) - 100.0f) / 100.0f;
                timers.idleCooldown = timers.idleInterval + 
                    ((float)random(100) / 100.0f) * timers.idleVariation;
            }
        }
        
        // Curious mode - eyes move left-right, one eye bigger
        if (params.curious) {
            params.curiousPhase += dt * 1.5f;  // Slow left-right movement
            // Oscillate gaze between left and right
            targets.gazeX = sinf(params.curiousPhase) * 0.8f;
            targets.gazeY = 0.0f;
        }
    }
    
    // ========================================================================
    // COMPUTE RENDER STATE
    // ========================================================================
    
    void computeRenderState() {
        // Effective openness for each eye
        float leftOpen = params.openness * params.leftOpenness;
        float rightOpen = params.openness * params.rightOpenness;
        
        // Squash/stretch: vertical stretch = horizontal compress
        float stretchY = params.squish;
        float stretchX = 1.0f / params.squish;  // Inverse for volume preservation
        
        // Base dimensions with squash/stretch
        int16_t eyeW = (int16_t)(layout.baseWidth * stretchX);
        int16_t eyeH = (int16_t)(layout.baseHeight * stretchY);
        
        // Apply openness to height
        int16_t leftH = (int16_t)(eyeH * leftOpen);
        int16_t rightH = (int16_t)(eyeH * rightOpen);
        if (leftH < 1) leftH = 1;
        if (rightH < 1) rightH = 1;
        
        // Gaze offset (constrained to screen)
        int16_t maxGazeX = (layout.screenW - layout.baseWidth * 2 - layout.spacing) / 2;
        int16_t maxGazeY = (layout.screenH - layout.baseHeight) / 2;
        int16_t gazeOffsetX = (int16_t)(params.gazeX * maxGazeX);
        int16_t gazeOffsetY = (int16_t)(params.gazeY * maxGazeY);
        
        // Add flicker
        gazeOffsetX += (int16_t)params.hFlicker;
        gazeOffsetY += (int16_t)params.vFlicker;
        
        // Compute final positions (centered vertically on squash/stretch)
        render.leftW = eyeW;
        render.leftH = leftH;
        render.leftX = layout.leftEyeBaseX + gazeOffsetX + (layout.baseWidth - eyeW) / 2;
        render.leftY = layout.eyeBaseY + gazeOffsetY + (layout.baseHeight - leftH) / 2;
        
        render.rightW = params.cyclops ? 0 : eyeW;
        render.rightH = params.cyclops ? 0 : rightH;
        render.rightX = layout.rightEyeBaseX + gazeOffsetX + (layout.baseWidth - eyeW) / 2;
        render.rightY = layout.eyeBaseY + gazeOffsetY + (layout.baseHeight - rightH) / 2;
        
        // Border radius scales with squish
        render.borderRadius = (uint8_t)(layout.borderRadius * min(stretchX, stretchY));
        if (render.borderRadius < 2) render.borderRadius = 2;
        
        // Mouth position (below eyes)
        int16_t eyeBottom = max(render.leftY + render.leftH, render.rightY + render.rightH);
        render.mouthX = (layout.screenW - layout.mouthWidth) / 2 + gazeOffsetX;
        render.mouthY = eyeBottom + 4;
        render.mouthW = layout.mouthWidth;
        render.mouthH = layout.mouthHeight + (int16_t)(params.mouthOpenness * 6);
    }
    
    // ========================================================================
    // DRAWING HELPERS
    // ========================================================================
    
    void drawEyes() {
        // === MOCHI 3D ILLUSION ===
        // Asymmetric squish: slightly flatter at bottom (gravity feel)
        int16_t topSquish = 0;
        int16_t bottomBulge = (int16_t)(2.0f * params.squish);
        
        // Parallax scale bias: eyes slightly larger when looking away from center
        float parallaxScale = 1.0f + fabsf(params.gazeX) * 0.05f;
        
        // Curious mode: one eye bigger based on gaze direction
        float leftScale = 1.0f;
        float rightScale = 1.0f;
        if (params.curious) {
            // When looking right (gazeX > 0), right eye gets bigger
            // When looking left (gazeX < 0), left eye gets bigger
            float curiousAmount = 0.25f;  // 25% size difference
            leftScale = 1.0f - params.gazeX * curiousAmount;   // Bigger when looking left
            rightScale = 1.0f + params.gazeX * curiousAmount;  // Bigger when looking right
        }
        
        // Left eye with 3D illusion
        int16_t lw = (int16_t)(render.leftW * parallaxScale * leftScale);
        int16_t lh = (int16_t)((render.leftH + bottomBulge) * leftScale);
        int16_t lx = render.leftX - (lw - render.leftW) / 2;
        int16_t ly = render.leftY - topSquish + (render.leftH - lh) / 2;  // Center vertically
        
        display->fillRoundRect(lx, ly, lw, lh, render.borderRadius, MAINCOLOR);
        
        // Right eye (skip if cyclops)
        if (!params.cyclops) {
            int16_t rw = (int16_t)(render.rightW * parallaxScale * rightScale);
            int16_t rh = (int16_t)((render.rightH + bottomBulge) * rightScale);
            int16_t rx = render.rightX - (rw - render.rightW) / 2;
            int16_t ry = render.rightY - topSquish + (render.rightH - rh) / 2;  // Center vertically
            
            display->fillRoundRect(rx, ry, rw, rh, render.borderRadius, MAINCOLOR);
        }
        
        // Store adjusted values for eyelid drawing
        render.leftW = lw;
        render.leftH = lh;
        render.leftX = lx;
        render.leftY = ly;
        if (!params.cyclops) {
            render.rightW = (int16_t)(render.rightW * parallaxScale * rightScale);
            render.rightH = (int16_t)((render.rightH + bottomBulge) * rightScale);
        }
    }
    
    void drawEyelids() {
        // Tired eyelids (top droop)
        if (params.fatigue > 0.1f) {
            int16_t droopH = (int16_t)(render.leftH * 0.4f * params.fatigue);
            display->fillTriangle(
                render.leftX, render.leftY - 1,
                render.leftX + render.leftW, render.leftY - 1,
                render.leftX, render.leftY + droopH,
                BGCOLOR
            );
            if (!params.cyclops) {
                display->fillTriangle(
                    render.rightX, render.rightY - 1,
                    render.rightX + render.rightW, render.rightY - 1,
                    render.rightX + render.rightW, render.rightY + droopH,
                    BGCOLOR
                );
            }
        }
        
        // Angry eyelids (inner droop)
        if (params.anger > 0.1f) {
            int16_t droopH = (int16_t)(render.leftH * 0.4f * params.anger);
            display->fillTriangle(
                render.leftX, render.leftY - 1,
                render.leftX + render.leftW, render.leftY - 1,
                render.leftX + render.leftW, render.leftY + droopH,
                BGCOLOR
            );
            if (!params.cyclops) {
                display->fillTriangle(
                    render.rightX, render.rightY - 1,
                    render.rightX + render.rightW, render.rightY - 1,
                    render.rightX, render.rightY + droopH,
                    BGCOLOR
                );
            }
        }
        
        // Happy eyelids (bottom arc)
        if (params.joy > 0.1f) {
            int16_t happyOffset = (int16_t)(render.leftH * 0.5f * params.joy);
            display->fillRoundRect(
                render.leftX - 1, render.leftY + render.leftH - happyOffset + 1,
                render.leftW + 2, layout.baseHeight,
                render.borderRadius, BGCOLOR
            );
            if (!params.cyclops) {
                display->fillRoundRect(
                    render.rightX - 1, render.rightY + render.rightH - happyOffset + 1,
                    render.rightW + 2, layout.baseHeight,
                    render.borderRadius, BGCOLOR
                );
            }
        }
    }
    
    void drawMouth() {
        if (render.mouthY > layout.screenH - 8) return;  // Off screen
        
        int16_t mx = render.mouthX;
        int16_t my = render.mouthY;
        int16_t mw = render.mouthW;
        
        // Clamp to screen
        if (mx < 0) mx = 0;
        if (mx + mw > layout.screenW) mx = layout.screenW - mw;
        
        int16_t openH = (int16_t)(params.mouthOpenness * 8);
        int16_t centerX = mx + mw / 2;
        
        switch (params.mouthShape) {
            case MOUTH_SMILE:
                if (params.mouthOpenness > 0.1f) {
                    // Open laughing mouth - oval with dark inside
                    int16_t openW = mw - 4;
                    int16_t openHt = 4 + openH;
                    display->fillRoundRect(mx + 2, my, openW, openHt, openHt / 2, MAINCOLOR);
                    if (openH > 2) {
                        display->fillRoundRect(mx + 4, my + 2, openW - 4, openHt - 4, (openHt - 4) / 2, BGCOLOR);
                    }
                } else {
                    // SMILE: curves DOWN in the middle (like a happy U shape)
                    // High at edges, low in center
                    int16_t smileDepth = 5;
                    int16_t thickness = 3;
                    
                    for (int16_t x = mx; x <= mx + mw; x++) {
                        float dx = (float)(x - centerX);
                        float normalizedX = dx / (mw / 2.0f);
                        // Parabola: high at edges (x^2), low in center
                        float curve = normalizedX * normalizedX;  // 0 at center, 1 at edges
                        int16_t y = my + smileDepth - (int16_t)(curve * smileDepth);
                        
                        for (int16_t t = 0; t < thickness; t++) {
                            display->drawPixel(x, y + t, MAINCOLOR);
                        }
                    }
                }
                break;
                
            case MOUTH_FROWN:
                {
                    // FROWN: corners DOWN (high Y), center UP (low Y) = ∩ shape
                    int16_t frownDepth = 4;
                    int16_t thickness = 3;
                    
                    for (int16_t x = mx; x <= mx + mw; x++) {
                        float dx = (float)(x - centerX);
                        float normalizedX = dx / (mw / 2.0f);
                        // Parabola: high at edges (x^2), low in center
                        float curve = normalizedX * normalizedX;  // 0 at center, 1 at edges
                        int16_t y = my + (int16_t)(curve * frownDepth);  // Higher Y at edges = corners down
                        
                        for (int16_t t = 0; t < thickness; t++) {
                            display->drawPixel(x, y + t, MAINCOLOR);
                        }
                    }
                }
                break;
                
            case MOUTH_OPEN:
                // Surprised O mouth
                display->fillRoundRect(mx + 4, my - 2, mw - 8, 10, 4, MAINCOLOR);
                display->fillRoundRect(mx + 6, my, mw - 12, 6, 3, BGCOLOR);
                break;
                
            case MOUTH_OOO:
                // Small O mouth
                display->fillCircle(centerX, my + 3, 5, MAINCOLOR);
                display->fillCircle(centerX, my + 3, 3, BGCOLOR);
                break;
                
            case MOUTH_FLAT:
                // Flat line with rounded ends
                display->fillRoundRect(mx + 2, my + 2, mw - 4, 3, 1, MAINCOLOR);
                break;
        }
    }
    
    void drawHeart(int16_t cx, int16_t cy, float scale) {
        if (scale < 0.1f) return;
        
        // Strong pulsing effect (bigger amplitude)
        float pulse = 1.0f + sinf(params.heartPulse) * 0.15f;
        scale *= pulse;
        
        // BIGGER hearts - increased base size from 16 to 28
        int16_t size = (int16_t)(28 * scale);
        int16_t r = size / 3;      // Bigger circles
        int16_t offset = size / 3;
        
        // Draw filled heart shape
        display->fillCircle(cx - offset + 2, cy - offset/3, r, MAINCOLOR);
        display->fillCircle(cx + offset - 2, cy - offset/3, r, MAINCOLOR);
        display->fillTriangle(
            cx - size/2 - 2, cy + 2,
            cx + size/2 + 2, cy + 2,
            cx, cy + size/2 + 4,
            MAINCOLOR
        );
        // Fill center gap
        display->fillRect(cx - offset + 2, cy - offset/3, (offset - 2) * 2, r + 2, MAINCOLOR);
        
        // Add inner highlight for 3D look
        if (size > 16) {
            int16_t hlR = max((int16_t)2, (int16_t)(size / 10));
            display->fillCircle(cx - offset/2, cy - offset/2, hlR, BGCOLOR);
        }
    }
    
    void drawLoveOverlay() {
        if (params.love < 0.1f) return;
        
        // Clear eye areas and draw hearts
        int16_t leftCX = render.leftX + render.leftW / 2;
        int16_t leftCY = render.leftY + render.leftH / 2;
        int16_t rightCX = render.rightX + render.rightW / 2;
        int16_t rightCY = render.rightY + render.rightH / 2;
        
        if (params.heartScale >= 0.9f) {
            // Full hearts - clear eyes first
            display->fillRoundRect(render.leftX - 2, render.leftY - 2, 
                                   render.leftW + 4, render.leftH + 4, 
                                   render.borderRadius, BGCOLOR);
            if (!params.cyclops) {
                display->fillRoundRect(render.rightX - 2, render.rightY - 2,
                                       render.rightW + 4, render.rightH + 4,
                                       render.borderRadius, BGCOLOR);
            }
        }
        
        drawHeart(leftCX, leftCY, params.heartScale);
        if (!params.cyclops) {
            drawHeart(rightCX, rightCY, params.heartScale);
        }
        
        // Blush cheeks
        if (params.love > 0.3f) {
            int16_t blushW = (int16_t)(10 * params.love);
            int16_t blushH = (int16_t)(5 * params.love);
            display->fillRoundRect(render.leftX - 12, render.leftY + render.leftH - 5, 
                                   blushW, blushH, 2, MAINCOLOR);
            if (!params.cyclops) {
                display->fillRoundRect(render.rightX + render.rightW + 2, 
                                       render.rightY + render.rightH - 5,
                                       blushW, blushH, 2, MAINCOLOR);
            }
        }
    }
    
    void drawTears() {
        if (params.tearProgress <= 0) return;
        
        int16_t tearLX = render.leftX + render.leftW / 2;
        int16_t tearRX = render.rightX + render.rightW / 2;
        int16_t startY = render.leftY + render.leftH;
        
        int16_t y1 = startY + (int16_t)fmodf(params.tearProgress, layout.screenH - startY);
        int16_t y2 = startY + (int16_t)fmodf(params.tearProgress + 10, layout.screenH - startY);
        
        int16_t tearSize = 4;
        if (y1 < layout.screenH - tearSize) {
            display->fillCircle(tearLX, y1 + tearSize, tearSize, MAINCOLOR);
            display->fillTriangle(tearLX - tearSize + 1, y1 + tearSize,
                                  tearLX + tearSize - 1, y1 + tearSize,
                                  tearLX, y1, MAINCOLOR);
        }
        if (!params.cyclops && y2 < layout.screenH - tearSize) {
            display->fillCircle(tearRX, y2 + tearSize, tearSize, MAINCOLOR);
            display->fillTriangle(tearRX - tearSize + 1, y2 + tearSize,
                                  tearRX + tearSize - 1, y2 + tearSize,
                                  tearRX, y2, MAINCOLOR);
        }
    }
    
    void drawSpiral(int16_t cx, int16_t cy, int16_t maxRadius) {
        float angle = params.spiralAngle;
        float radius = 2;
        int prevX = cx, prevY = cy;
        
        while (radius < maxRadius) {
            int x = cx + (int)(cosf(angle) * radius);
            int y = cy + (int)(sinf(angle) * radius);
            display->drawLine(prevX, prevY, x, y, MAINCOLOR);
            display->drawLine(prevX + 1, prevY, x + 1, y, MAINCOLOR);
            prevX = x;
            prevY = y;
            angle += 0.3f;
            radius += 0.4f;
        }
    }
    
    void drawKnockedOverlay() {
        if (timers.knockedActive <= 0) return;
        
        int16_t spiralR = min(render.leftW, render.leftH) / 2 - 2;
        if (spiralR < 8) spiralR = 8;
        
        // Clear eyes
        display->fillRoundRect(render.leftX - 1, render.leftY - 1,
                               render.leftW + 2, render.leftH + 2,
                               render.borderRadius, BGCOLOR);
        if (!params.cyclops) {
            display->fillRoundRect(render.rightX - 1, render.rightY - 1,
                                   render.rightW + 2, render.rightH + 2,
                                   render.borderRadius, BGCOLOR);
        }
        
        drawSpiral(render.leftX + render.leftW / 2, 
                   render.leftY + render.leftH / 2, spiralR);
        if (!params.cyclops) {
            drawSpiral(render.rightX + render.rightW / 2,
                       render.rightY + render.rightH / 2, spiralR);
        }
    }
    
    void drawSweat() {
        if (!params.sweat) return;
        
        for (int i = 0; i < 3; i++) {
            sweatY[i] += 0.5f;
            if (sweatY[i] > 20 + random(10)) {
                // Reset drop
                if (i == 0) sweatX[i] = random(30);
                else if (i == 1) sweatX[i] = 30 + random(layout.screenW - 60);
                else sweatX[i] = layout.screenW - 30 + random(30);
                sweatY[i] = 2;
                sweatSize[i] = 2;
            }
            
            if (sweatY[i] < 15) {
                sweatSize[i] += 0.3f;
            } else {
                sweatSize[i] -= 0.1f;
            }
            if (sweatSize[i] < 1) sweatSize[i] = 1;
            
            display->fillRoundRect((int16_t)sweatX[i], (int16_t)sweatY[i],
                                   (int16_t)sweatSize[i], (int16_t)(sweatSize[i] * 1.5f),
                                   3, MAINCOLOR);
        }
    }

public:
    // ========================================================================
    // CONSTRUCTOR
    // ========================================================================
    
    MochiEyes(AdafruitDisplay& disp) : display(&disp) {
        // Initialize layout with defaults
        layout.screenW = 128;
        layout.screenH = 64;
        layout.baseWidth = 36;
        layout.baseHeight = 36;
        layout.spacing = 10;
        layout.borderRadius = 8;
        layout.mouthWidth = 20;
        layout.mouthHeight = 6;
        layout.recompute();
        
        params.reset();
        targets.reset();
        timers.reset();
        
        lastFrameMs = 0;
        frameInterval = 20;  // 50fps default
        
        for (int i = 0; i < 3; i++) {
            sweatX[i] = random(layout.screenW);
            sweatY[i] = random(20);
            sweatSize[i] = 2;
        }
    }
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    void begin(int width, int height, uint8_t fps) {
        layout.screenW = width;
        layout.screenH = height;
        layout.recompute();
        
        frameInterval = 1000 / fps;
        
        display->clearDisplay();
        display->display();
        
        // Start with closed eyes, will open smoothly
        params.openness = 0.0f;
        params.leftOpenness = 1.0f;
        params.rightOpenness = 1.0f;
        targets.openness = 1.0f;
    }
    
    // ========================================================================
    // MAIN UPDATE LOOP
    // ========================================================================
    
    void update() {
        unsigned long now = millis();
        if (now - lastFrameMs < frameInterval) return;
        
        float dt = (now - lastFrameMs) / 1000.0f;
        lastFrameMs = now;
        
        updateTimers(dt);
        updateParams(dt);
        computeRenderState();
        
        // Render
        display->clearDisplay();
        drawEyes();
        drawEyelids();
        drawMouth();
        drawSweat();
        drawLoveOverlay();
        drawTears();
        drawKnockedOverlay();
        display->display();
    }
    
    // ========================================================================
    // PUBLIC API - IMPULSE SETTERS
    // ========================================================================
    
    void setOpenness(float target, float speed = 8.0f) {
        targets.openness = clampf(target, 0.0f, 1.0f);
        targets.opennessSpeed = speed;
    }
    
    void setSquish(float target, float speed = 6.0f) {
        targets.squish = clampf(target, 0.5f, 1.5f);
        targets.squishSpeed = speed;
    }
    
    void setGaze(float x, float y, float speed = 4.0f) {
        targets.gazeX = clampf(x, -1.0f, 1.0f);
        targets.gazeY = clampf(y, -1.0f, 1.0f);
        targets.gazeSpeed = speed;
    }
    
    void setMouthShape(MouthShape shape) {
        params.mouthShape = shape;
    }
    
    void setMouthOpenness(float target, float speed = 10.0f) {
        targets.mouthOpenness = clampf(target, 0.0f, 1.0f);
        targets.mouthSpeed = speed;
    }
    
    // ========================================================================
    // PUBLIC API - EMOTIONS
    // ========================================================================
    
    void setJoy(float weight, float speed = 3.0f) {
        targets.joy = clampf(weight, 0.0f, 1.0f);
        targets.emotionSpeed = speed;
    }
    
    void setAnger(float weight, float speed = 3.0f) {
        targets.anger = clampf(weight, 0.0f, 1.0f);
        targets.emotionSpeed = speed;
    }
    
    void setFatigue(float weight, float speed = 3.0f) {
        targets.fatigue = clampf(weight, 0.0f, 1.0f);
        targets.emotionSpeed = speed;
    }
    
    void setLove(float weight, float speed = 3.0f) {
        targets.love = clampf(weight, 0.0f, 1.0f);
        targets.emotionSpeed = speed;
    }
    
    void resetEmotions() {
        targets.joy = 0.0f;
        targets.anger = 0.0f;
        targets.fatigue = 0.0f;
        targets.love = 0.0f;
        targets.heartScale = 0.0f;
    }
    
    // ========================================================================
    // PUBLIC API - ONE-SHOT ANIMATIONS
    // ========================================================================
    
    void blink() {
        targets.openness = 0.0f;
        // Will automatically reopen due to smoothing toward 1.0
        targets.openness = 1.0f;
        params.openness = 0.0f;  // Force instant close
    }
    
    void wink(bool left) {
        if (left) {
            targets.leftOpenness = 0.0f;
            params.leftOpenness = 0.0f;
            targets.leftOpenness = 1.0f;
        } else {
            targets.rightOpenness = 0.0f;
            params.rightOpenness = 0.0f;
            targets.rightOpenness = 1.0f;
        }
    }
    
    void close() {
        targets.openness = 0.0f;
    }
    
    void open() {
        targets.openness = 1.0f;
    }
    
    void triggerLove(float durationSec = 2.0f) {
        timers.loveRemaining = durationSec;
        params.heartPulse = 0.0f;
    }
    
    void triggerCry(float durationSec = 3.0f) {
        timers.cryRemaining = durationSec;
        params.tearProgress = 0.0f;
    }
    
    void triggerConfused(float durationSec = 0.5f) {
        timers.confusedRemaining = durationSec;
    }
    
    void triggerLaugh(float durationSec = 1.0f) {
        timers.laughRemaining = durationSec;
    }
    
    void setKnocked(bool on) {
        timers.knockedActive = on ? 1.0f : 0.0f;
        if (on) {
            params.spiralAngle = 0.0f;
            params.mouthShape = MOUTH_OOO;
        }
    }
    
    void setSweat(bool on) {
        params.sweat = on;
    }
    
    void setCyclops(bool on) {
        params.cyclops = on;
    }
    
    // ========================================================================
    // PUBLIC API - AUTO BEHAVIORS
    // ========================================================================
    
    void setAutoblinker(bool active, float interval = 3.0f, float variation = 3.0f) {
        timers.autoBlink = active;
        timers.blinkInterval = interval;
        timers.blinkVariation = variation;
    }
    
    void setIdleMode(bool active, float interval = 2.0f, float variation = 3.0f) {
        timers.idleMode = active;
        timers.idleInterval = interval;
        timers.idleVariation = variation;
        if (active) {
            timers.idleCooldown = 0.5f;  // Start soon
        }
    }
    
    // Animation speed setters
    void setGazeSpeed(float speed) { targets.gazeSpeed = speed; }
    void setOpennessSpeed(float speed) { targets.opennessSpeed = speed; }
    void setSquishSpeed(float speed) { targets.squishSpeed = speed; }
    
    // ========================================================================
    // PUBLIC API - LAYOUT CONFIGURATION
    // ========================================================================
    
    void setWidth(int16_t left, int16_t right) {
        layout.baseWidth = left;  // Use left for both (symmetric)
        layout.recompute();
    }
    
    void setHeight(int16_t left, int16_t right) {
        layout.baseHeight = left;
        layout.recompute();
    }
    
    void setSpacebetween(int16_t space) {
        layout.spacing = space;
        layout.recompute();
    }
    
    void setBorderradius(int16_t left, int16_t right) {
        layout.borderRadius = left;
    }
    
    void setMouthSize(int16_t width, int16_t height) {
        layout.mouthWidth = width;
        layout.mouthHeight = height;
    }
    
    void setDisplayColors(uint8_t bg, uint8_t main) {
        BGCOLOR = bg;
        MAINCOLOR = main;
    }
    
    // ========================================================================
    // PUBLIC API - GETTERS (for external state sync)
    // ========================================================================
    
    int16_t getEyeWidth() const { return layout.baseWidth; }
    int16_t getEyeHeight() const { return layout.baseHeight; }
    int16_t getSpaceBetween() const { return layout.spacing; }
    int16_t getBorderRadius() const { return layout.borderRadius; }
    int16_t getMouthWidth() const { return layout.mouthWidth; }
    
    // ========================================================================
    // LEGACY API COMPATIBILITY (maps to new system)
    // ========================================================================
    
    void setMood(uint8_t mood) {
        resetEmotions();
        switch (mood) {
            case 1:  // TIRED
                setFatigue(1.0f);
                break;
            case 2:  // ANGRY
                setAnger(1.0f);
                break;
            case 3:  // HAPPY
                setJoy(1.0f);
                break;
            default:  // DEFAULT
                break;
        }
    }
    
    void setPosition(uint8_t pos) {
        switch (pos) {
            case 1: setGaze(0, -1); break;     // N
            case 2: setGaze(1, -1); break;     // NE
            case 3: setGaze(1, 0); break;      // E
            case 4: setGaze(1, 1); break;      // SE
            case 5: setGaze(0, 1); break;      // S
            case 6: setGaze(-1, 1); break;     // SW
            case 7: setGaze(-1, 0); break;     // W
            case 8: setGaze(-1, -1); break;    // NW
            default: setGaze(0, 0); break;     // CENTER
        }
    }
    
    void setMouthType(int type) {
        switch (type) {
            case 1: params.mouthShape = MOUTH_SMILE; break;
            case 2: params.mouthShape = MOUTH_FROWN; break;
            case 3: params.mouthShape = MOUTH_OPEN; break;
            case 4: params.mouthShape = MOUTH_OOO; break;
            case 5: params.mouthShape = MOUTH_FLAT; break;
            default: params.mouthShape = MOUTH_SMILE; break;
        }
    }
    
    void setMouthEnabled(bool enabled) {
        // Not implemented - mouth always shows
    }
    
    void setLaughDuration(int ms) {
        // Legacy - use triggerLaugh(seconds) instead
    }
    
    void setLoveDuration(int ms) {
        // Legacy - use triggerLove(seconds) instead
    }
    
    void setCuriosity(bool on) {
        params.curious = on;
        if (on) {
            params.curiousPhase = 0.0f;
        }
    }
    
    void setHFlicker(bool on, uint8_t amplitude = 2) {
        params.hFlicker = on ? amplitude : 0;
    }
    
    void setVFlicker(bool on, uint8_t amplitude = 2) {
        params.vFlicker = on ? amplitude : 0;
    }
    
    void setEyebrows(bool raised) {
        // Not implemented in v2 - could add later
    }
    
    void setConfusedDuration(int ms) {}
    void setCryDuration(int ms) {}
    
    // Animation triggers (legacy names)
    void anim_love() { triggerLove(2.0f); }
    void anim_cry() { triggerCry(3.0f); }
    void anim_confused() { triggerConfused(0.5f); }
    void anim_laugh() { triggerLaugh(1.0f); }
    void anim_knocked() { setKnocked(true); }
    
    void startMouthAnim(int anim, unsigned long duration) {
        // Map to laugh for now
        if (anim == 1 || anim == 4) {
            triggerLaugh(duration / 1000.0f);
        }
    }
};

#endif // MOCHI_EYES_H
