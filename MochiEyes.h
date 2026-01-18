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
    MOUTH_FLAT = 4,
    MOUTH_W = 5,    // UwU cat mouth
    MOUTH_D = 6     // XD open mouth
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
    
    // Mouth (independent from eyes)
    float mouthOpenness;    // 0 = closed, 1 = open
    MouthShape mouthShape;  // Current shape being displayed
    MouthShape targetMouthShape; // Target shape to transition to
    float mouthTransition;  // 0 = at current shape, 1 = at target shape
    
    // Special effects
    float heartScale;       // 0 = no heart, 1 = full size
    float heartPulse;       // For pulsing animation
    float tearProgress;     // 0 = no tears, >0 = tears falling
    float spiralAngle;      // For knocked/dizzy effect
    float knockedIntensity; // 0 = normal, 1 = full spiral (smooth)
    float sweatIntensity;   // 0 = no sweat, 1 = full sweat (smooth)
    float curiousIntensity; // 0 = normal, 1 = curious mode (smooth)
    float uwuIntensity;     // 0 = normal, 1 = full UwU face (smooth)
    float xdIntensity;      // 0 = normal, 1 = full >U< face (smooth)
    bool cyclops;
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
        targetMouthShape = MOUTH_SMILE;
        mouthTransition = 1.0f;  // Already at target
        heartScale = 0.0f;
        heartPulse = 0.0f;
        tearProgress = 0.0f;
        spiralAngle = 0.0f;
        knockedIntensity = 0.0f;
        sweatIntensity = 0.0f;
        curiousIntensity = 0.0f;
        uwuIntensity = 0.0f;
        xdIntensity = 0.0f;
        cyclops = false;
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
    float knockedIntensity;  // Target for knocked effect
    float sweatIntensity;    // Target for sweat effect
    float curiousIntensity;  // Target for curious mode
    float uwuIntensity;      // Target for UwU effect
    float xdIntensity;       // Target for >U< effect
    
    // Animation speeds (units per second)
    float opennessSpeed;
    float squishSpeed;
    float gazeSpeed;
    float emotionSpeed;
    float mouthSpeed;
    float heartSpeed;
    float effectSpeed;       // Speed for knocked/sweat/curious transitions
    
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
        knockedIntensity = 0.0f;
        sweatIntensity = 0.0f;
        curiousIntensity = 0.0f;
        uwuIntensity = 0.0f;
        xdIntensity = 0.0f;
        
        opennessSpeed = 12.0f;   // Faster blink
        squishSpeed = 10.0f;     // Snappier squish
        gazeSpeed = 6.0f;        // Faster gaze
        emotionSpeed = 5.0f;     // Faster emotion transitions
        mouthSpeed = 15.0f;      // Snappier mouth
        heartSpeed = 8.0f;       // Faster heart transition
        effectSpeed = 4.0f;      // Speed for effect transitions
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
    float uwuRemaining;
    float xdRemaining;
    
    // Mouth animations
    float mouthAnimRemaining;
    int mouthAnimType;  // 1=talk, 2=chew, 3=wobble
    
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
        confusedRemaining = 0.0f;
        loveRemaining = 0.0f;
        cryRemaining = 0.0f;
        laughRemaining = 0.0f;
        uwuRemaining = 0.0f;
        xdRemaining = 0.0f;
        
        idleMode = true;
        mouthAnimType = 0;
        
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
        
        // Smooth effect transitions
        params.knockedIntensity = smoothDamp(params.knockedIntensity, targets.knockedIntensity, targets.effectSpeed, dt);
        params.sweatIntensity = smoothDamp(params.sweatIntensity, targets.sweatIntensity, targets.effectSpeed, dt);
        params.curiousIntensity = smoothDamp(params.curiousIntensity, targets.curiousIntensity, targets.effectSpeed, dt);
        params.uwuIntensity = smoothDamp(params.uwuIntensity, targets.uwuIntensity, targets.effectSpeed, dt);
        params.xdIntensity = smoothDamp(params.xdIntensity, targets.xdIntensity, targets.effectSpeed, dt);
    }
    
    // ========================================================================
    // ANIMATION TIMERS UPDATE
    // ========================================================================
    
    void updateTimers(float dt) {
        // === MOUTH TRANSITION (independent from eyes) ===
        // Smoothly transition mouth to target shape
        if (params.mouthShape != params.targetMouthShape) {
            params.mouthTransition += dt * 8.0f;  // Transition speed
            if (params.mouthTransition >= 1.0f) {
                params.mouthShape = params.targetMouthShape;
                params.mouthTransition = 1.0f;
            }
        }
        
        // Knocked overrides mouth to OOO
        if (params.knockedIntensity > 0.5f) {
            params.mouthShape = MOUTH_OOO;
        }
        
        // Mouth animations (talk, chew, wobble)
        if (timers.mouthAnimRemaining > 0) {
            timers.mouthAnimRemaining -= dt;
            float t = timers.mouthAnimRemaining;
            
            switch (timers.mouthAnimType) {
                case 1: // Talk - rapid open/close with varying amplitude
                    targets.mouthOpenness = (sinf(t * 25.0f) * 0.5f + 0.5f) * 
                                           (0.3f + sinf(t * 5.0f) * 0.2f);
                    break;
                case 2: // Chew - asymmetric jaw movement
                    targets.mouthOpenness = fabsf(sinf(t * 8.0f)) * 0.6f;
                    break;
                case 3: // Wobble - mouth position oscillates left/right
                    // Use mouthOpenness variation to simulate wobble since we can't move mouth X
                    targets.mouthOpenness = 0.2f + sinf(t * 12.0f) * 0.15f;
                    break;
            }
        } else if (timers.mouthAnimType != 0) {
            targets.mouthOpenness = 0.0f;
            timers.mouthAnimType = 0;
        }
        
        // === EYE ANIMATIONS (separate from mouth) ===
        
        // Love animation
        if (timers.loveRemaining > 0) {
            timers.loveRemaining -= dt;
            targets.love = 1.0f;
            targets.heartScale = 1.0f;
            params.heartPulse += dt * 10.0f;
        } else if (params.love > 0.01f) {
            targets.love = 0.0f;
            targets.heartScale = 0.0f;
        }
        
        // Cry animation
        if (timers.cryRemaining > 0) {
            timers.cryRemaining -= dt;
            params.tearProgress += dt * 40.0f;
            if (params.tearProgress > layout.screenH) {
                params.tearProgress = 0.0f;
            }
            targets.fatigue = 0.5f;
        } else if (params.tearProgress > 0 || params.fatigue > 0.01f) {
            // Reset tears and fatigue when cry ends
            params.tearProgress = 0.0f;
            targets.fatigue = 0.0f;
        }
        
        // Confused animation - eyes shake
        if (timers.confusedRemaining > 0) {
            timers.confusedRemaining -= dt;
            params.hFlicker = sinf(timers.confusedRemaining * 50.0f) * 8.0f;
        } else {
            params.hFlicker = 0.0f;
        }
        
        // Laugh animation
        if (timers.laughRemaining > 0) {
            timers.laughRemaining -= dt;
            params.vFlicker = sinf(timers.laughRemaining * 20.0f) * 2.0f;
            targets.mouthOpenness = (sinf(timers.laughRemaining * 12.0f) + 1.0f) * 0.5f;
            targets.joy = 1.0f;
        } else {
            params.vFlicker = 0.0f;
            if (timers.laughRemaining < 0 && timers.laughRemaining > -0.1f) {
                targets.mouthOpenness = 0.0f;
                targets.joy = 0.0f;
            }
        }
        
        // Knocked (spiral eyes)
        if (params.knockedIntensity > 0.1f) {
            params.spiralAngle += dt * 8.0f;
        }
        
        // Auto-blink (disabled when knocked)
        if (timers.autoBlink && params.knockedIntensity < 0.5f) {
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
        
        // Curious mode - use intensity for smooth eye size change
        if (params.curiousIntensity > 0.1f) {
            params.curiousPhase += dt * 1.5f;  // Slow left-right movement
            // Oscillate gaze between left and right
            targets.gazeX = sinf(params.curiousPhase) * 0.8f * params.curiousIntensity;
            targets.gazeY = 0.0f;
        }
        
        // UwU timer - eyes only, no joy override
        if (timers.uwuRemaining > 0) {
            timers.uwuRemaining -= dt;
            targets.uwuIntensity = 1.0f;
        } else if (params.uwuIntensity > 0.01f) {
            targets.uwuIntensity = 0.0f;
        }
        
        // XD (>U<) timer - eyes only, no joy override
        if (timers.xdRemaining > 0) {
            timers.xdRemaining -= dt;
            targets.xdIntensity = 1.0f;
        } else if (params.xdIntensity > 0.01f) {
            targets.xdIntensity = 0.0f;
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
        
        // Curious mode: one eye bigger based on gaze direction (uses smooth intensity)
        float leftScale = 1.0f;
        float rightScale = 1.0f;
        if (params.curiousIntensity > 0.01f) {
            // When looking right (gazeX > 0), right eye gets bigger
            // When looking left (gazeX < 0), left eye gets bigger
            float curiousAmount = 0.25f * params.curiousIntensity;  // 25% size difference, scaled by intensity
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
                
            case MOUTH_W: {
                // UwU cat mouth (two curved bumps forming a 'w')
                int16_t centerX = mx + mw / 2;
                int16_t bumpR = mw / 5;
                int16_t wHeight = 6;
                
                for (int16_t thick = 0; thick < 2; thick++) {
                    // Left bump
                    for (int16_t angle = 0; angle <= 180; angle += 6) {
                        float rad = angle * 3.14159f / 180.0f;
                        int16_t px = centerX - bumpR - (int16_t)(bumpR * cosf(rad));
                        int16_t py = my + thick + (int16_t)(wHeight * sinf(rad));
                        display->drawPixel(px, py, MAINCOLOR);
                        display->drawPixel(px + 1, py, MAINCOLOR);
                    }
                    // Right bump
                    for (int16_t angle = 0; angle <= 180; angle += 6) {
                        float rad = angle * 3.14159f / 180.0f;
                        int16_t px = centerX + bumpR + (int16_t)(bumpR * cosf(rad));
                        int16_t py = my + thick + (int16_t)(wHeight * sinf(rad));
                        display->drawPixel(px, py, MAINCOLOR);
                        display->drawPixel(px - 1, py, MAINCOLOR);
                    }
                }
                break;
            }
                
            case MOUTH_D: {
                // XD open mouth (filled D/U shape)
                int16_t centerX = mx + mw / 2;
                int16_t radius = mw / 3;
                int16_t dHeight = 10;
                
                // Draw filled semi-circle
                for (int16_t angle = 0; angle <= 180; angle++) {
                    float rad = angle * 3.14159f / 180.0f;
                    int16_t x = centerX + (int16_t)(radius * cosf(rad));
                    int16_t y = my + (int16_t)(dHeight * sinf(rad));
                    display->drawLine(centerX, my, x, y, MAINCOLOR);
                }
                display->fillTriangle(mx + mw/2 - radius, my, mx + mw/2 + radius, my, centerX, my + dHeight, MAINCOLOR);
                break;
            }
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
    
    void drawUwUOverlay() {
        if (params.uwuIntensity < 0.1f) return;
        
        float intensity = params.uwuIntensity;
        
        // Center positions for eyes
        int16_t leftCX = render.leftX + render.leftW / 2;
        int16_t leftCY = render.leftY + render.leftH / 2;
        int16_t rightCX = render.rightX + render.rightW / 2;
        int16_t rightCY = render.rightY + render.rightH / 2;
        
        // Clear eye areas when intensity is high enough
        if (intensity > 0.5f) {
            display->fillRoundRect(render.leftX - 2, render.leftY - 2,
                                   render.leftW + 4, render.leftH + 4,
                                   render.borderRadius, BGCOLOR);
            if (!params.cyclops) {
                display->fillRoundRect(render.rightX - 2, render.rightY - 2,
                                       render.rightW + 4, render.rightH + 4,
                                       render.borderRadius, BGCOLOR);
            }
            // Clear mouth area to replace it with w mouth
            display->fillRect(render.mouthX - 2, render.mouthY - 2, 
                             render.mouthW + 4, render.mouthH + 6, BGCOLOR);
        }
        
        // === U-SHAPED EYES (thick top legs, thinning at bottom curve) ===
        int16_t uWidth = (int16_t)(render.leftW * 0.6f * intensity);
        int16_t uHeight = (int16_t)(render.leftH * 0.7f * intensity);
        if (uWidth < 12) uWidth = 12;
        if (uHeight < 14) uHeight = 14;
        
        auto drawU = [&](int16_t cx, int16_t cy, bool mirror) {
            int16_t halfW = uWidth / 2;
            int16_t legH = uHeight - halfW;
            
            // Asymmetrical Thickness Profile
            // Swapped: Thin Start -> Middle -> Thick End
            float startR = 1.0f;   // Start (Thin)
            float medR = 2.0f;     // Middle
            float endR = 3.0f;     // End (Thick)
            
            int16_t totalSteps = legH * 2 + (int16_t)(3.14159f * halfW);
            
            for (int16_t step = 0; step <= totalSteps; step++) {
                float t = (float)step / (float)totalSteps;
                int16_t px, py;
                float radius;
                
                if (t < 0.35f) {
                    // Left leg going down
                    float legT = t / 0.35f;
                    px = cx - halfW;
                    py = cy - uHeight/2 + (int16_t)(legT * legH);
                    
                    if (!mirror) {
                        // Standard (Left Eye): Start -> Medium
                        radius = startR + legT * (medR - startR);
                    } else {
                        // Mirrored (Right Eye): End -> Medium
                        radius = endR - legT * (endR - medR);
                    }
                } else if (t > 0.65f) {
                    // Right leg going up
                    float legT = (t - 0.65f) / 0.35f; // 0 at bottom, 1 at top
                    px = cx + halfW;
                    py = cy - uHeight/2 + legH - (int16_t)(legT * legH);
                    
                    if (!mirror) {
                        // Standard (Left Eye): Medium -> End (Thick)
                        radius = medR + legT * (endR - medR);
                    } else {
                        // Mirrored (Right Eye): Medium -> Start (Thin)
                        radius = medR - legT * (medR - startR);
                    }
                } else {
                    // Bottom curve
                    float curveT = (t - 0.35f) / 0.3f;
                    float angle = 3.14159f * curveT;
                    px = cx - (int16_t)(halfW * cosf(angle));
                    py = cy - uHeight/2 + legH + (int16_t)(halfW * sinf(angle));
                    radius = medR;
                }
                
                if (radius >= 1.0f) {
                    display->fillCircle(px, py, (int16_t)radius, MAINCOLOR);
                } else {
                    display->drawPixel(px, py, MAINCOLOR);
                }
            }
        };
        
        drawU(leftCX, leftCY, false); // Left eye: Thin -> Thick
        if (!params.cyclops) {
            drawU(rightCX, rightCY, true); // Right eye: Thick -> Thin (Mirrored)
        }
        
        // === COMPACT W MOUTH (thick middle, thin edges) ===
        int16_t mouthCX = layout.centerX;
        int16_t mouthY = render.mouthY + 1;
        int16_t wWidth = (int16_t)(26 * intensity);
        int16_t wHeight = (int16_t)(10 * intensity);
        if (wWidth < 14) wWidth = 14;
        if (wHeight < 6) wHeight = 6;
        int16_t bumpR = wWidth / 4;
        
        // Draw W bumps with varying thickness
        // Left Bump: 180 (Edge/Thin) -> 0 (Center/Thick)
        // Right Bump: 180 (Edge/Thin) -> 0 (Center/Thick)
        
        float edgeThick = 1.0f;
        float centerThick = 2.5f;
        
        // Left Bump
        for (int16_t angle = 0; angle <= 180; angle += 4) {
            float t = (float)(180 - angle) / 180.0f; // 0 at Edge, 1 at Center
            float rad = angle * 3.14159f / 180.0f;
            int16_t px = mouthCX - bumpR - (int16_t)(bumpR * cosf(rad));
            int16_t py = mouthY + (int16_t)(wHeight * sinf(rad));
            
            float radius = edgeThick + t * (centerThick - edgeThick);
            if (radius > 1.0f) display->fillCircle(px, py, (int16_t)radius, MAINCOLOR);
            else display->drawPixel(px, py, MAINCOLOR);
        }
        
        // Right Bump
        for (int16_t angle = 0; angle <= 180; angle += 4) {
            float t = (float)(180 - angle) / 180.0f; // 0 at Edge, 1 at Center
            float rad = angle * 3.14159f / 180.0f;
            int16_t px = mouthCX + bumpR + (int16_t)(bumpR * cosf(rad));
            int16_t py = mouthY + (int16_t)(wHeight * sinf(rad));
            
            float radius = edgeThick + t * (centerThick - edgeThick);
             if (radius > 1.0f) display->fillCircle(px, py, (int16_t)radius, MAINCOLOR);
            else display->drawPixel(px, py, MAINCOLOR);
        }
        
        // === SOLID BLUSH OVALS (soft rounded look) ===
        if (intensity > 0.3f) {
            int16_t blushW = (int16_t)(14 * intensity);
            int16_t blushH = (int16_t)(5 * intensity);
            int16_t blushY = leftCY + uHeight/2 + 2;
            
            // Left blush
            display->fillRoundRect(render.leftX + render.leftW/2 - uWidth/2 - blushW/2 - 4, 
                                   blushY, blushW, blushH, 10, MAINCOLOR);
                                   
            // Right blush
            if (!params.cyclops) {
                display->fillRoundRect(render.rightX + render.rightW/2 + uWidth/2 - blushW/2 + 4,
                                       blushY, blushW, blushH, 10, MAINCOLOR);
            }
        }
    }
    
    void drawXDOverlay() {
        if (params.xdIntensity < 0.1f) return;
        
        float intensity = params.xdIntensity;
        
        // Center positions
        int16_t leftCX = render.leftX + render.leftW / 2;
        int16_t leftCY = render.leftY + render.leftH / 2;
        int16_t rightCX = render.rightX + render.rightW / 2;
        int16_t rightCY = render.rightY + render.rightH / 2;
        
        // Clear eye areas
        if (intensity > 0.5f) {
            display->fillRoundRect(render.leftX - 2, render.leftY - 2,
                                   render.leftW + 4, render.leftH + 4,
                                   render.borderRadius, BGCOLOR);
            if (!params.cyclops) {
                display->fillRoundRect(render.rightX - 2, render.rightY - 2,
                                       render.rightW + 4, render.rightH + 4,
                                       render.borderRadius, BGCOLOR);
            }
            // Clear mouth area
            display->fillRect(render.mouthX - 4, render.mouthY - 2, 
                             render.mouthW + 8, render.mouthH + 8, BGCOLOR);
        }
        
        // === > < EYES ===
        int16_t eyeSize = (int16_t)(render.leftW * 0.7f * intensity);
        if (eyeSize < 12) eyeSize = 12;
        int16_t stroke = 3;
        
        // Helper to draw chevron
        auto drawChevron = [&](int16_t cx, int16_t cy, bool pointRight) {
            int16_t hSize = eyeSize / 2;
            int16_t vSize = eyeSize / 2;
            
            if (pointRight) {
                // > Shape (Left Eye)
                // Top stroke: Top-Left to Right-Center
                for (int i=0; i<stroke; i++) {
                    display->drawLine(cx - hSize, cy - vSize + i, cx + hSize, cy + i, MAINCOLOR);
                    display->drawLine(cx - hSize, cy - vSize + i - 1, cx + hSize, cy + i - 1, MAINCOLOR);
                }
                // Bottom stroke: Bottom-Left to Right-Center
                for (int i=0; i<stroke; i++) {
                    display->drawLine(cx - hSize, cy + vSize - i, cx + hSize, cy - i, MAINCOLOR);
                    display->drawLine(cx - hSize, cy + vSize - i + 1, cx + hSize, cy - i + 1, MAINCOLOR);
                }
            } else {
                // < Shape (Right Eye)
                // Top stroke: Top-Right to Left-Center
                for (int i=0; i<stroke; i++) {
                    display->drawLine(cx + hSize, cy - vSize + i, cx - hSize, cy + i, MAINCOLOR);
                    display->drawLine(cx + hSize, cy - vSize + i - 1, cx - hSize, cy + i - 1, MAINCOLOR);
                }
                // Bottom stroke: Bottom-Right to Left-Center
                for (int i=0; i<stroke; i++) {
                    display->drawLine(cx + hSize, cy + vSize - i, cx - hSize, cy - i, MAINCOLOR);
                    display->drawLine(cx + hSize, cy + vSize - i + 1, cx - hSize, cy - i + 1, MAINCOLOR);
                }
            }
        };
        
        drawChevron(leftCX, leftCY, true);  // >
        if (!params.cyclops) {
            drawChevron(rightCX, rightCY, false); // <
        }
        
        // === OPEN MOUTH (D shape / Open U) ===
        // Filled maroon/red based on request, but we only have MAINCOLOR/BGCOLOR?
        // Usually index 0 is bg, 1 is main. If we want red we need color support.
        // Assuming monochrome OLED/LCD for now, so filled with MAINCOLOR
        
        int16_t mouthW = (int16_t)(20 * intensity);
        int16_t mouthH = (int16_t)(14 * intensity);
        int16_t mouthX = layout.centerX - mouthW/2;
        int16_t mouthY = render.mouthY;
        
        // Draw filled semi-circle/U for open mouth
        // Top edge is flat or slightly curved down
        
        // Using fillCircle and rect to make a D shape
        int16_t radius = mouthW / 2;
        
        // Draw top flat part
        // display->fillRect(mouthX, mouthY, mouthW, 2, MAINCOLOR);
        
        // Draw bottom filled arc
        for (int16_t angle = 0; angle <= 180; angle++) {
            float rad = angle * 3.14159f / 180.0f;
            // Draw lines from center-top to edge
            int16_t x = layout.centerX + (int16_t)(radius * cosf(rad));
            int16_t y = mouthY + (int16_t)(mouthH * sinf(rad));
            display->drawLine(layout.centerX, mouthY, x, y, MAINCOLOR);
        }
        // Fill the rest to be sure
        display->fillTriangle(mouthX, mouthY, mouthX + mouthW, mouthY, layout.centerX, mouthY + mouthH, MAINCOLOR);
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
        float radius = 3;  // Start slightly bigger
        int prevX = cx, prevY = cy;
        
        while (radius < maxRadius) {
            int x = cx + (int)(cosf(angle) * radius);
            int y = cy + (int)(sinf(angle) * radius);
            // Draw thicker lines (3 pixels wide)
            display->drawLine(prevX, prevY, x, y, MAINCOLOR);
            display->drawLine(prevX + 1, prevY, x + 1, y, MAINCOLOR);
            display->drawLine(prevX, prevY + 1, x, y + 1, MAINCOLOR);
            prevX = x;
            prevY = y;
            angle += 0.25f;   // Tighter spiral (was 0.3)
            radius += 0.5f;   // Faster growth (was 0.4)
        }
    }
    
    void drawKnockedOverlay() {
        if (params.knockedIntensity < 0.05f) return;
        
        // Make spiral fill most of the eye (bigger radius)
        int16_t spiralR = min(render.leftW, render.leftH) / 2 + 4;  // Added +4 for bigger spiral
        if (spiralR < 12) spiralR = 12;
        
        // Scale spiral by intensity for fade-in effect
        spiralR = (int16_t)(spiralR * params.knockedIntensity);
        if (spiralR < 6) spiralR = 6;
        
        // Clear eyes (partial based on intensity)
        if (params.knockedIntensity > 0.5f) {
            display->fillRoundRect(render.leftX - 1, render.leftY - 1,
                                   render.leftW + 2, render.leftH + 2,
                                   render.borderRadius, BGCOLOR);
            if (!params.cyclops) {
                display->fillRoundRect(render.rightX - 1, render.rightY - 1,
                                       render.rightW + 2, render.rightH + 2,
                                       render.borderRadius, BGCOLOR);
            }
        }
        
        drawSpiral(render.leftX + render.leftW / 2, 
                   render.leftY + render.leftH / 2, spiralR);
        if (!params.cyclops) {
            drawSpiral(render.rightX + render.rightW / 2,
                       render.rightY + render.rightH / 2, spiralR);
        }
    }
    
    void drawSweat() {
        if (params.sweatIntensity < 0.1f) return;
        
        for (int i = 0; i < 3; i++) {
            sweatY[i] += 0.5f * params.sweatIntensity;
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
            
            // Scale size by intensity for fade effect
            float scaledSize = sweatSize[i] * params.sweatIntensity;
            if (scaledSize >= 1.0f) {
                display->fillRoundRect((int16_t)sweatX[i], (int16_t)sweatY[i],
                                       (int16_t)scaledSize, (int16_t)(scaledSize * 1.5f),
                                       3, MAINCOLOR);
            }
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
        drawUwUOverlay();
        drawXDOverlay();
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
        if (params.mouthShape != shape) {
            params.targetMouthShape = shape;
            params.mouthTransition = 0.0f;  // Start transition
        }
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
        // Reset emotion targets
        targets.joy = 0.0f;
        targets.anger = 0.0f;
        targets.fatigue = 0.0f;
        targets.love = 0.0f;
        targets.heartScale = 0.0f;
        
        // Reset openness to fully open
        targets.openness = 1.0f;
        targets.leftOpenness = 1.0f;
        targets.rightOpenness = 1.0f;
        
        // Stop timed animations
        timers.loveRemaining = 0.0f;
        timers.cryRemaining = 0.0f;
        timers.confusedRemaining = 0.0f;
        timers.laughRemaining = 0.0f;
        timers.mouthAnimRemaining = 0.0f;
        timers.mouthAnimType = 0;
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
            // Close left eye
            targets.leftOpenness = 0.0f;
            params.leftOpenness = 0.0f;
            targets.leftOpenness = 1.0f;
            // Right eye squints a little in sympathy
            params.rightOpenness = 0.7f;
            targets.rightOpenness = 1.0f;
        } else {
            // Close right eye
            targets.rightOpenness = 0.0f;
            params.rightOpenness = 0.0f;
            targets.rightOpenness = 1.0f;
            // Left eye squints a little in sympathy
            params.leftOpenness = 0.7f;
            targets.leftOpenness = 1.0f;
        }
        // Add a slight squish for extra character
        params.squish = 0.95f;
        targets.squish = 1.0f;
    }
    
    void close() {
        targets.openness = 0.0f;
    }
    
    void open() {
        targets.openness = 1.0f;
    }
    
    // Helper to clear all overlay expression timers/intensities (for mutual exclusivity)
    void clearAllOverlays() {
        timers.loveRemaining = 0.0f;
        timers.cryRemaining = 0.0f;
        timers.laughRemaining = 0.0f;
        timers.uwuRemaining = 0.0f;
        timers.xdRemaining = 0.0f;
        timers.confusedRemaining = 0.0f;
        
        targets.knockedIntensity = 0.0f;
        targets.uwuIntensity = 0.0f;
        targets.xdIntensity = 0.0f;
        targets.love = 0.0f;
        targets.fatigue = 0.0f;
        
        params.tearProgress = 0.0f;
        params.hFlicker = 0.0f;
        params.vFlicker = 0.0f;
    }
    
    void triggerLove(float durationSec = 2.0f) {
        clearAllOverlays();  // Cancel other expressions
        timers.loveRemaining = durationSec;
        params.heartPulse = 0.0f;
    }
    
    void triggerCry(float durationSec = 3.0f) {
        clearAllOverlays();
        timers.cryRemaining = durationSec;
        params.tearProgress = 0.0f;
    }
    
    void triggerConfused(float durationSec = 0.5f) {
        clearAllOverlays();
        timers.confusedRemaining = durationSec;
    }
    
    void triggerUwU(float duration) {
        clearAllOverlays();
        timers.uwuRemaining = duration;
    }
    
    void triggerXD(float duration) {
        clearAllOverlays();
        timers.xdRemaining = duration;
    }
    
    void triggerLaugh(float durationSec = 1.0f) {
        clearAllOverlays();
        timers.laughRemaining = durationSec;
    }
    
    void setKnocked(bool on) {
        if (on) {
            clearAllOverlays();  // Cancel other expressions
            targets.knockedIntensity = 1.0f;
            params.spiralAngle = 0.0f;
            blink();
        } else {
            targets.knockedIntensity = 0.0f;
        }
    }
    
    void setSweat(bool on) {
        targets.sweatIntensity = on ? 1.0f : 0.0f;
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
        MouthShape shape = MOUTH_SMILE;
        switch (type) {
            case 1: shape = MOUTH_SMILE; break;
            case 2: shape = MOUTH_FROWN; break;
            case 3: shape = MOUTH_OPEN; break;
            case 4: shape = MOUTH_OOO; break;
            case 5: shape = MOUTH_FLAT; break;
            case 6: shape = MOUTH_W; break;    // UwU mouth
            case 7: shape = MOUTH_D; break;    // XD mouth
            default: shape = MOUTH_SMILE; break;
        }
        setMouthShape(shape);  // Use transition system
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
        targets.curiousIntensity = on ? 1.0f : 0.0f;
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
        // anim: 1=talk, 2=chew, 3=wobble
        timers.mouthAnimRemaining = duration / 1000.0f;
        timers.mouthAnimType = anim;
    }
};

#endif // MOCHI_EYES_H
