#include "leor/mochi_eyes_engine.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace leor {

// ---------------------------------------------------------------------------
// Expression presets – ported from esp32-eyes EyePresets.h
// [0] = right eye (or both if symmetric), [1] = left eye (or alternate)
// ---------------------------------------------------------------------------
struct ExpressionPreset {
    EyeShapeConfig right;
    EyeShapeConfig left;
};static const ExpressionPreset kPresets[EXPR_COUNT] = {
    // EXPR_NORMAL
    { {0,0,40,40, 0,0, 8,8}, {0,0,40,40, 0,0, 8,8} },
    // EXPR_ANGRY
    { {-3,0,24,40, 0.3f,0, 4,10}, {-3,0,24,40, 0.3f,0, 4,10} },
    // EXPR_GLEE
    { {0,0,14,40, 0,0, 8,0}, {0,0,14,40, 0,0, 8,0} },
    // EXPR_HAPPY
    { {0,-2,18,40, 0,0, 10,0}, {0,-2,18,40, 0,0, 10,0} },
    // EXPR_SAD
    { {0,0,22,40, -0.4f,0, 2,10}, {0,0,22,40, -0.4f,0, 2,10} },
    // EXPR_WORRIED
    { {-1,0,30,40, -0.2f,0, 8,8}, {-1,0,24,40, -0.1f,0, 8,8} },
    // EXPR_FOCUSED
    { {0,1,16,40, 0.2f,0, 4,4}, {0,1,16,40, 0.2f,0, 4,4} },
    // EXPR_ANNOYED
    { {0,-1,10,40, 0,0, 2,4}, {0,0,16,40, 0,0, 4,6} },
    // EXPR_SURPRISED
    { {-2,0,45,45, 0,0, 16,16}, {-2,0,45,45, 0,0, 16,16} },
    // EXPR_SKEPTIC
    { {0,-4,22,40, 0.3f,0, 2,8}, {0,0,30,40, 0,0, 8,8} },
    // EXPR_FRUSTRATED
    { {3,-4,14,40, 0,0, 2,6}, {3,-4,14,40, 0,0, 2,6} },
    // EXPR_UNIMPRESSED
    { {3,-2,24,40, 0,0, 2,8}, {3,0,18,40, 0,0, 2,6} },
    // EXPR_SLEEPY — pure horizontal slits
    { {0,-2,10,35, -0.1f,-0.1f, 4,4}, {0,-2,10,35, -0.1f,-0.1f, 4,4} },
    // EXPR_SUSPICIOUS
    { {0,-2,16,40, 0.2f,0, 4,4}, {0,0,22,40, 0,0, 6,4} },
    // EXPR_SQUINT
    { {4,0,16,26, 0,0, 4,4}, {-4,-2,24,35, 0,0, 6,6} },
    // EXPR_FURIOUS
    { {-2,0,26,40, 0.4f,0, 2,8}, {-2,0,26,40, 0.4f,0, 2,8} },
    // EXPR_SCARED
    { {-3,0,36,40, -0.2f,0, 10,8}, {-3,0,36,40, -0.2f,0, 10,8} },
    // EXPR_AWE
    { {2,-3,40,48, -0.1f,0.1f, 12,12}, {2,-3,40,48, -0.1f,0.1f, 12,12} }
};

float MochiEyesEngine::lerp(float current, float target, float speed,
                            float dt) {
  float diff = target - current;
  float delta = speed * dt;
  if (std::fabs(diff) <= delta) {
    return target;
  }
  return current + (diff > 0 ? delta : -delta);
}

float MochiEyesEngine::smoothDamp(float current, float target, float speed,
                                  float dt) {
  float t = 1.0f - std::exp(-speed * dt);
  return current + (target - current) * t;
}

float MochiEyesEngine::clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

MochiEyesEngine::MochiEyesEngine(DisplayBackend &display) : display_(display) {
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
  frameInterval = 20; // 50fps default

  for (int i = 0; i < 3; i++) {
    sweatX[i] = std::rand() % layout.screenW;
    sweatY[i] = std::rand() % 20;
    sweatSize[i] = 2;
  }
}

void MochiEyesEngine::begin() {
  layout.screenW = display_.width();
  layout.screenH = display_.height();
  layout.recompute();

  display_.clear();
  display_.send_buffer();

  params.openness = 0.0f;
  params.leftOpenness = 1.0f;
  params.rightOpenness = 1.0f;
  targets.openness = 1.0f;
}

void MochiEyesEngine::update(uint32_t now_ms) {
  if (lastFrameMs != 0 && now_ms - lastFrameMs < frameInterval)
    return;

  float dt = (now_ms - lastFrameMs) / 1000.0f;
  if (lastFrameMs == 0)
    dt = 0.02f;
  if (dt > 0.1f)
    dt = 0.1f; // Clamp max dt to prevent animation explosions
  lastFrameMs = now_ms;

  render.saveOldDirty();
  render.resetDirty();

  updateTimers(dt);
  updateParams(dt);
  computeRenderState();

  // Always clear full screen to prevent parametric shape artifacts
  display_.clear();

  drawEyes();
  drawMouth();
  drawSweat();
  drawLoveOverlay();
  drawUwUOverlay();
  drawXDOverlay();
  drawTears();
  drawKnockedOverlay();
  drawSleepOverlay();

  display_.send_buffer();
}

void MochiEyesEngine::lerpShape(EyeShapeConfig& current, const EyeShapeConfig& target, float speed, float dt) {
    if (speed <= 0.0f) return;
    float t = 1.0f - std::exp(-speed * dt);
    
    auto lerpInt = [t](int16_t c, int16_t tgt) -> int16_t {
        if (c == tgt) return c;
        float diff = static_cast<float>(tgt - c);
        float step = diff * t;
        int16_t int_step = static_cast<int16_t>(step);
        if (int_step == 0) {
            return c + (diff > 0.0f ? 1 : -1);
        }
        return c + int_step;
    };

    current.OffsetX = lerpInt(current.OffsetX, target.OffsetX);
    current.OffsetY = lerpInt(current.OffsetY, target.OffsetY);
    current.Height  = lerpInt(current.Height, target.Height);
    current.Width   = lerpInt(current.Width, target.Width);
    
    current.Slope_Top    += (target.Slope_Top    - current.Slope_Top)    * t;
    current.Slope_Bottom += (target.Slope_Bottom - current.Slope_Bottom) * t;
    
    current.Radius_Top    = lerpInt(current.Radius_Top, target.Radius_Top);
    current.Radius_Bottom = lerpInt(current.Radius_Bottom, target.Radius_Bottom);
}

void MochiEyesEngine::updateParams(float dt) {
  params.openness =
      smoothDamp(params.openness, targets.openness, targets.opennessSpeed, dt);
  params.leftOpenness = smoothDamp(params.leftOpenness, targets.leftOpenness,
                                   targets.opennessSpeed, dt);
  params.rightOpenness = smoothDamp(params.rightOpenness, targets.rightOpenness,
                                    targets.opennessSpeed, dt);
  params.squish =
      smoothDamp(params.squish, targets.squish, targets.squishSpeed, dt);
  params.gazeX = smoothDamp(params.gazeX, targets.gazeX, targets.gazeSpeed, dt);
  params.gazeY = smoothDamp(params.gazeY, targets.gazeY, targets.gazeSpeed, dt);

  params.joy = smoothDamp(params.joy, targets.joy, targets.emotionSpeed, dt);
  params.anger =
      smoothDamp(params.anger, targets.anger, targets.emotionSpeed, dt);
  params.fatigue =
      smoothDamp(params.fatigue, targets.fatigue, targets.emotionSpeed, dt);
  params.love = smoothDamp(params.love, targets.love, targets.emotionSpeed, dt);

  params.mouthOpenness = smoothDamp(params.mouthOpenness, targets.mouthOpenness,
                                    targets.mouthSpeed, dt);
  params.heartScale =
      smoothDamp(params.heartScale, targets.heartScale, targets.heartSpeed, dt);

  params.knockedIntensity =
      smoothDamp(params.knockedIntensity, targets.knockedIntensity,
                 targets.effectSpeed, dt);
  params.sweatIntensity = smoothDamp(
      params.sweatIntensity, targets.sweatIntensity, targets.effectSpeed, dt);
  params.curiousIntensity =
      smoothDamp(params.curiousIntensity, targets.curiousIntensity,
                 targets.effectSpeed, dt);
  params.uwuIntensity = smoothDamp(params.uwuIntensity, targets.uwuIntensity,
                                   targets.effectSpeed, dt);
  params.xdIntensity = smoothDamp(params.xdIntensity, targets.xdIntensity,
                                  targets.effectSpeed, dt);
  params.sleepIntensity = smoothDamp(params.sleepIntensity, targets.sleepIntensity,
                                     3.0f, dt);
  if (params.sleepIntensity > 0.1f) {
    params.sleepPhase += dt;
  }

  // Interpolate parametric eye shapes toward targets
  float shapeSpeed = 5.0f;
  lerpShape(params.leftShape, params.leftShapeTarget, shapeSpeed, dt);
  lerpShape(params.rightShape, params.rightShapeTarget, shapeSpeed, dt);
}

void MochiEyesEngine::updateTimers(float dt) {
  if (params.mouthShape != params.targetMouthShape) {
    params.mouthTransition += dt * 8.0f;
    if (params.mouthTransition >= 1.0f) {
      params.mouthShape = params.targetMouthShape;
      params.mouthTransition = 1.0f;
    }
  }

  if (params.knockedIntensity > 0.5f) {
    params.mouthShape = MOUTH_OOO;
  }

  if (timers.mouthAnimRemaining > 0) {
    timers.mouthAnimRemaining -= dt;
    float t = timers.mouthAnimRemaining;

    switch (timers.mouthAnimType) {
    case 1:
      targets.mouthOpenness = (std::sin(t * 20.0f) * 0.4f + 0.6f) *
                              (0.4f + std::sin(t * 3.0f) * 0.3f);
      break;
    case 2:
      targets.mouthOpenness = std::fabs(std::sin(t * 6.0f)) * 0.5f + 0.1f;
      break;
    case 3:
      targets.mouthOpenness =
          0.3f + std::sin(t * 15.0f) * 0.2f + std::cos(t * 7.0f) * 0.1f;
      break;
    }
  } else if (timers.mouthAnimType != 0) {
    targets.mouthOpenness = 0.0f;
    timers.mouthAnimType = 0;
  }

  if (targets.love > 0.5f) {
    params.heartPulse += dt * 10.0f;
  }

  if (targets.fatigue > 0.3f) {
    params.tearProgress += dt * 40.0f;
    if (params.tearProgress > layout.screenH) {
      params.tearProgress = 0.0f;
    }
  } else if (params.tearProgress > 0) {
    params.tearProgress = 0.0f;
  }

  if (params.confusedIntensity > 0.1f) {
    params.confusedPhase += dt * 50.0f;
    params.hFlicker =
        std::sin(params.confusedPhase) * 8.0f * params.confusedIntensity;
  } else {
    params.hFlicker = 0.0f;
  }

  if (params.laughIntensity > 0.1f) {
    params.laughPhase += dt;
    params.vFlicker =
        std::sin(params.laughPhase * 20.0f) * 2.0f * params.laughIntensity;
    targets.mouthOpenness = (std::sin(params.laughPhase * 12.0f) + 1.0f) *
                            0.5f * params.laughIntensity;
  } else {
    params.vFlicker = 0.0f;
  }

  if (params.knockedIntensity > 0.1f) {
    params.spiralAngle += dt * 8.0f;
  }

  if (timers.autoBlink && params.knockedIntensity < 0.5f) {
    timers.blinkCooldown -= dt;
    if (timers.blinkCooldown <= 0) {
      int blinkRoll = std::rand() % 100;
      if (blinkRoll < 60)
        timers.nextBlinkType = 0;
      else if (blinkRoll < 75)
        timers.nextBlinkType = 1;
      else if (blinkRoll < 85)
        timers.nextBlinkType = 2;
      else if (blinkRoll < 95)
        timers.nextBlinkType = 3;
      else
        timers.nextBlinkType = 4;

      switch (timers.nextBlinkType) {
      case 0:
        blink();
        break;
      case 1:
        targets.openness = 0.0f;
        targets.opennessSpeed = 6.0f;
        params.openness = 0.0f;
        targets.openness = 1.0f;
        break;
      case 2:
        targets.openness = 0.0f;
        targets.opennessSpeed = 18.0f;
        params.openness = 0.0f;
        targets.openness = 1.0f;
        break;
      case 3:
        targets.openness = 0.3f;
        targets.opennessSpeed = 14.0f;
        params.openness = 0.3f;
        targets.openness = 1.0f;
        break;
      case 4:
        if ((std::rand() % 2) == 0) {
          params.leftOpenness = 0.0f;
          targets.leftOpenness = 1.0f;
          params.rightOpenness = 0.3f;
          targets.rightOpenness = 1.0f;
        } else {
          params.rightOpenness = 0.0f;
          targets.rightOpenness = 1.0f;
          params.leftOpenness = 0.3f;
          targets.leftOpenness = 1.0f;
        }
        targets.openness = 0.0f;
        params.openness = 0.0f;
        targets.openness = 1.0f;
        break;
      }

      float intervalVariation = ((float)(std::rand() % 200) - 50.0f) / 100.0f;
      timers.blinkCooldown =
          timers.blinkInterval + (intervalVariation * timers.blinkVariation);
      if (timers.blinkCooldown < 1.0f)
        timers.blinkCooldown = 1.0f;
    }
  }

  if (timers.idleMode) {
    timers.idleCooldown -= dt;
    if (timers.idleCooldown <= 0) {
      targets.gazeX = ((float)(std::rand() % 200) - 100.0f) / 100.0f;
      targets.gazeY = ((float)(std::rand() % 200) - 100.0f) / 100.0f;
      timers.idleCooldown =
          timers.idleInterval +
          ((float)(std::rand() % 100) / 100.0f) * timers.idleVariation;
    }
  }

  if (timers.breathingEnabled) {
    timers.breathingPhase += dt * timers.breathingSpeed * 6.28318f;
    float breathCycle = std::sin(timers.breathingPhase);
    targets.squish = 1.0f + (breathCycle * timers.breathingIntensity);
  }

  if (params.curiousIntensity > 0.01f) {
    params.curiousPhase += dt * 1.5f;
    targets.gazeX =
        std::sin(params.curiousPhase) * 1.0f * params.curiousIntensity;
    targets.gazeY = 0.0f;
    // Visually squint the eyes and make them wider left/right during curious/nervous gaze
    float squintFactor = 1.0f - (std::fabs(targets.gazeX) * 0.15f * params.curiousIntensity);
    if (!timers.breathingEnabled) {
      targets.squish = squintFactor; 
    } else {
      targets.squish *= squintFactor; 
    }
  } else if (!timers.breathingEnabled) {
    targets.squish = 1.0f;
  }
}

void MochiEyesEngine::computeRenderState() {
  float leftOpen = params.openness * params.leftOpenness;
  float rightOpen = params.openness * params.rightOpenness;

  float stretchY = params.squish;
  float stretchX = 1.0f / params.squish;

  int16_t eyeW = (int16_t)(layout.baseWidth * stretchX);
  int16_t eyeH = (int16_t)(layout.baseHeight * stretchY);

  int16_t leftH = (int16_t)(eyeH * leftOpen);
  int16_t rightH = (int16_t)(eyeH * rightOpen);
  if (leftH < 1)
    leftH = 1;
  if (rightH < 1)
    rightH = 1;

  int16_t maxGazeX =
      (layout.screenW - layout.baseWidth * 2 - layout.spacing) / 2;
  int16_t maxGazeY = (layout.screenH - layout.baseHeight) / 2;
  int16_t gazeOffsetX = (int16_t)(params.gazeX * maxGazeX);
  int16_t gazeOffsetY = (int16_t)(params.gazeY * maxGazeY);

  gazeOffsetX += (int16_t)params.hFlicker;
  gazeOffsetY += (int16_t)params.vFlicker;

  render.leftW = eyeW;
  render.leftH = leftH;
  render.leftX =
      layout.leftEyeBaseX + gazeOffsetX + (layout.baseWidth - eyeW) / 2;
  render.leftY =
      layout.eyeBaseY + gazeOffsetY + (layout.baseHeight - leftH) / 2;

  render.rightW = params.cyclops ? 0 : eyeW;
  render.rightH = params.cyclops ? 0 : rightH;
  render.rightX =
      layout.rightEyeBaseX + gazeOffsetX + (layout.baseWidth - eyeW) / 2;
  render.rightY =
      layout.eyeBaseY + gazeOffsetY + (layout.baseHeight - rightH) / 2;

  render.borderRadius =
      (uint8_t)(layout.borderRadius * std::min(stretchX, stretchY));
  if (render.borderRadius < 2)
    render.borderRadius = 2;

  int16_t eyeBottom =
      std::max(render.leftY + render.leftH, render.rightY + render.rightH);
  render.mouthX = (layout.screenW - layout.mouthWidth) / 2 + gazeOffsetX;
  render.mouthY = eyeBottom + 4;
  render.mouthW = layout.mouthWidth;
  int16_t openAdd = (int16_t)(params.mouthOpenness * 8);
  render.mouthH = layout.mouthHeight + openAdd + 6;

  int16_t pad = 8;

  render.expandDirty(render.leftX - pad, render.leftY - pad,
                     render.leftW + pad * 2, render.leftH + pad * 2);
  if (!params.cyclops) {
    render.expandDirty(render.rightX - pad, render.rightY - pad,
                       render.rightW + pad * 2, render.rightH + pad * 2);
  }

  render.expandDirty(render.mouthX - pad * 2, render.mouthY - pad * 2,
                     render.mouthW + pad * 4, render.mouthH + pad * 4);

  if (params.tearProgress > 0 || params.knockedIntensity > 0.05f ||
      params.sweatIntensity > 0.1f) {
    render.expandDirty(0, 0, layout.screenW, layout.screenH);
  } else if (params.love > 0.1f) {
    render.expandDirty(0, 0, layout.screenW, layout.screenH);
  } else if (params.uwuIntensity > 0.1f || params.xdIntensity > 0.1f) {
    render.expandDirty(0, 0, layout.screenW, layout.screenH);
  }
}

void MochiEyesEngine::fillRoundRect(int x, int y, int w, int h, int r,
                                    uint8_t color) {
  display_.set_color(color);
  display_.fill_round_rect(x, y, w, h, r);
}

void MochiEyesEngine::fillTriangle(int x0, int y0, int x1, int y1, int x2,
                                   int y2, uint8_t color) {
  display_.set_color(color);
  display_.fill_triangle(x0, y0, x1, y1, x2, y2);
}

void MochiEyesEngine::drawPixel(int x, int y, uint8_t color) {
  display_.set_color(color);
  display_.draw_pixel(x, y);
}

void MochiEyesEngine::drawLine(int x0, int y0, int x1, int y1, uint8_t color) {
  display_.set_color(color);
  display_.draw_line(x0, y0, x1, y1);
}

void MochiEyesEngine::fillRect(int x, int y, int w, int h, uint8_t color) {
  if (w <= 0 || h <= 0) return;
  display_.set_color(color);
  display_.fill_box(x, y, w, h);
}

void MochiEyesEngine::fillCircle(int x, int y, int r, uint8_t color) {
  display_.set_color(color);
  display_.fill_circle(x, y, r);
}

// ---------------------------------------------------------------------------
// Parametric eye renderer (ported from esp32-eyes EyeDrawer)
// ---------------------------------------------------------------------------
void MochiEyesEngine::fillEllipseCorner(CornerType corner, int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint8_t color) {
    if (rx < 1 || ry < 1) return;
    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t fx2 = 4 * rx2;
    int32_t fy2 = 4 * ry2;
    int32_t x, y, s;

    display_.set_color(color);

    if (corner == T_R) {
        for (x = 0, y = ry, s = 2*ry2 + rx2*(1 - 2*ry); ry2*x <= rx2*y; x++) {
            display_.draw_hline(x0, y0 - y, x);
            if (s >= 0) { s += fx2*(1-y); y--; }
            s += ry2*((4*x)+6);
        }
        for (x = rx, y = 0, s = 2*rx2 + ry2*(1 - 2*rx); rx2*y <= ry2*x; y++) {
            display_.draw_hline(x0, y0 - y, x);
            if (s >= 0) { s += fy2*(1-x); x--; }
            s += rx2*((4*y)+6);
        }
    } else if (corner == B_R) {
        for (x = 0, y = ry, s = 2*ry2 + rx2*(1 - 2*ry); ry2*x <= rx2*y; x++) {
            display_.draw_hline(x0, y0 + y - 1, x);
            if (s >= 0) { s += fx2*(1-y); y--; }
            s += ry2*((4*x)+6);
        }
        for (x = rx, y = 0, s = 2*rx2 + ry2*(1 - 2*rx); rx2*y <= ry2*x; y++) {
            display_.draw_hline(x0, y0 + y - 1, x);
            if (s >= 0) { s += fy2*(1-x); x--; }
            s += rx2*((4*y)+6);
        }
    } else if (corner == T_L) {
        for (x = 0, y = ry, s = 2*ry2 + rx2*(1 - 2*ry); ry2*x <= rx2*y; x++) {
            display_.draw_hline(x0 - x, y0 - y, x);
            if (s >= 0) { s += fx2*(1-y); y--; }
            s += ry2*((4*x)+6);
        }
        for (x = rx, y = 0, s = 2*rx2 + ry2*(1 - 2*rx); rx2*y <= ry2*x; y++) {
            display_.draw_hline(x0 - x, y0 - y, x);
            if (s >= 0) { s += fy2*(1-x); x--; }
            s += rx2*((4*y)+6);
        }
    } else if (corner == B_L) {
        for (x = 0, y = ry, s = 2*ry2 + rx2*(1 - 2*ry); ry2*x <= rx2*y; x++) {
            display_.draw_hline(x0 - x, y0 + y - 1, x);
            if (s >= 0) { s += fx2*(1-y); y--; }
            s += ry2*((4*x)+6);
        }
        for (x = rx, y = 0, s = 2*rx2 + ry2*(1 - 2*rx); rx2*y <= ry2*x; y++) {
            display_.draw_hline(x0 - x, y0 + y - 1, x);
            if (s >= 0) { s += fy2*(1-x); x--; }
            s += rx2*((4*y)+6);
        }
    }
    display_.set_color(1);
}

void MochiEyesEngine::drawEyeShape(int16_t centerX, int16_t centerY, EyeShapeConfig* config) {
    if (config->Height < 1 || config->Width < 2) return;

    // For very small or zero-radius shapes, fall back to simple filled rect
    if (config->Radius_Top < 1 && config->Radius_Bottom < 1 &&
        std::fabs(config->Slope_Top) < 0.01f && std::fabs(config->Slope_Bottom) < 0.01f) {
        int16_t x = centerX + config->OffsetX - config->Width / 2;
        int16_t y = centerY + config->OffsetY - config->Height / 2;
        fillRect(x, y, config->Width, config->Height, MAINCOLOR);
        return;
    }

    int32_t delta_y_top = static_cast<int32_t>(config->Height * config->Slope_Top / 2.0f);
    int32_t delta_y_bottom = static_cast<int32_t>(config->Height * config->Slope_Bottom / 2.0f);
    auto totalHeight = config->Height + delta_y_top - delta_y_bottom;

    int16_t rTop = config->Radius_Top;
    int16_t rBot = config->Radius_Bottom;
    if (rBot > 0 && rTop > 0 && totalHeight - 1 < rBot + rTop) {
        int32_t corrTop = static_cast<int32_t>(static_cast<float>(rTop) * (totalHeight - 1) / (rBot + rTop));
        int32_t corrBot = static_cast<int32_t>(static_cast<float>(rBot) * (totalHeight - 1) / (rBot + rTop));
        rTop = static_cast<int16_t>(corrTop);
        rBot = static_cast<int16_t>(corrBot);
    }

    int32_t TLc_y = centerY + config->OffsetY - config->Height/2 + rTop - delta_y_top;
    int32_t TLc_x = centerX + config->OffsetX - config->Width/2 + rTop;
    int32_t TRc_y = centerY + config->OffsetY - config->Height/2 + rTop + delta_y_top;
    int32_t TRc_x = centerX + config->OffsetX + config->Width/2 - rTop;
    int32_t BLc_y = centerY + config->OffsetY + config->Height/2 - rBot - delta_y_bottom;
    int32_t BLc_x = centerX + config->OffsetX - config->Width/2 + rBot;
    int32_t BRc_y = centerY + config->OffsetY + config->Height/2 - rBot + delta_y_bottom;
    int32_t BRc_x = centerX + config->OffsetX + config->Width/2 - rBot;

    int32_t min_c_x = std::min(TLc_x, BLc_x);
    int32_t max_c_x = std::max(TRc_x, BRc_x);
    int32_t min_c_y = std::min(TLc_y, TRc_y);
    int32_t max_c_y = std::max(BLc_y, BRc_y);

    // Helper: fill rect from two corner coords (like original FillRectangle)
    auto fillRect2 = [&](int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t color) {
        int32_t l = std::min(x0, x1);
        int32_t r = std::max(x0, x1);
        int32_t t = std::min(y0, y1);
        int32_t b = std::max(y0, y1);
        int32_t w = r - l;
        int32_t h = b - t;
        if (w > 0 && h > 0) fillRect(l, t, w, h, color);
    };

    // Fill eye centre
    fillRect2(min_c_x, min_c_y, max_c_x, max_c_y, MAINCOLOR);
    // Fill outward to meet edges of rounded corners
    fillRect2(TRc_x, TRc_y, BRc_x + rBot, BRc_y, MAINCOLOR);  // Right
    fillRect2(TLc_x - rTop, TLc_y, BLc_x, BLc_y, MAINCOLOR);  // Left
    fillRect2(TLc_x, TLc_y - rTop, TRc_x, TRc_y, MAINCOLOR);  // Top
    fillRect2(BLc_x, BLc_y, BRc_x, BRc_y + rBot, MAINCOLOR);  // Bottom

    // Draw slanted edges at top (erase the rect overshoot, fill the correct triangle)
    if (config->Slope_Top > 0.02f) {
        fillTriangle(TLc_x, TLc_y - rTop, TRc_x, TRc_y - rTop, TRc_x, TLc_y - rTop, BGCOLOR);
        fillTriangle(TRc_x, TRc_y - rTop, TLc_x, TLc_y - rTop, TLc_x, TRc_y - rTop, MAINCOLOR);
    } else if (config->Slope_Top < -0.02f) {
        fillTriangle(TRc_x, TRc_y - rTop, TLc_x, TLc_y - rTop, TLc_x, TRc_y - rTop, BGCOLOR);
        fillTriangle(TLc_x, TLc_y - rTop, TRc_x, TRc_y - rTop, TRc_x, TLc_y - rTop, MAINCOLOR);
    }

    // Draw rounded corners
    if (rTop > 0) {
        fillEllipseCorner(T_L, TLc_x, TLc_y, rTop, rTop, MAINCOLOR);
        fillEllipseCorner(T_R, TRc_x, TRc_y, rTop, rTop, MAINCOLOR);
    }
    if (rBot > 0) {
        fillEllipseCorner(B_L, BLc_x, BLc_y, rBot, rBot, MAINCOLOR);
        fillEllipseCorner(B_R, BRc_x, BRc_y, rBot, rBot, MAINCOLOR);
    }

    // Draw slanted edges at bottom MUST happen after corners so the corners get properly masked!
    // Use a deadzone to avoid ghost cuts during animation interpolation
    if (config->Slope_Bottom > 0.02f) {
        fillTriangle(BRc_x + rBot - 1, BRc_y + rBot - 1, BLc_x - rBot, BLc_y + rBot - 1, BLc_x - rBot, BRc_y + rBot - 1, BGCOLOR);
    } else if (config->Slope_Bottom < -0.02f) {
        fillTriangle(BLc_x - rBot, BLc_y + rBot - 1, BRc_x + rBot - 1, BRc_y + rBot - 1, BRc_x + rBot - 1, BLc_y + rBot - 1, BGCOLOR);
    }
}

void MochiEyesEngine::drawEyes() {
    // Scale shapes relative to baseWidth/baseHeight (presets assume 40x40 base)
    float scaleX = layout.baseWidth / 40.0f;
    float scaleY = (layout.baseHeight / 40.0f) * params.squish;
    float openLeft = params.openness * params.leftOpenness;
    float openRight = params.openness * params.rightOpenness;

    // Compute gaze offset
    int16_t maxGazeX = (layout.screenW - layout.baseWidth * 2 - layout.spacing) / 2;
    int16_t maxGazeY = (layout.screenH - layout.baseHeight) / 2;
    int16_t gazeOffsetX = static_cast<int16_t>(params.gazeX * maxGazeX) + static_cast<int16_t>(params.hFlicker);
    int16_t gazeOffsetY = static_cast<int16_t>(params.gazeY * maxGazeY) + static_cast<int16_t>(params.vFlicker);

    int16_t leftCX = layout.leftEyeBaseX + layout.baseWidth / 2 + gazeOffsetX;
    int16_t leftCY = layout.eyeBaseY + layout.baseHeight / 2 + gazeOffsetY;
    int16_t rightCX = layout.rightEyeBaseX + layout.baseWidth / 2 + gazeOffsetX;
    int16_t rightCY = layout.eyeBaseY + layout.baseHeight / 2 + gazeOffsetY;

    // Build scaled + openness-modulated shape configs
    EyeShapeConfig leftCfg = params.leftShape;
    leftCfg.Width   = static_cast<int16_t>(leftCfg.Width   * scaleX);
    leftCfg.Height  = static_cast<int16_t>(leftCfg.Height  * scaleY * openLeft);
    leftCfg.OffsetX = static_cast<int16_t>(leftCfg.OffsetX * scaleX);
    leftCfg.OffsetY = static_cast<int16_t>(leftCfg.OffsetY * scaleY);
    leftCfg.Radius_Top    = static_cast<int16_t>(leftCfg.Radius_Top    * std::min(scaleX, scaleY));
    leftCfg.Radius_Bottom = static_cast<int16_t>(leftCfg.Radius_Bottom * std::min(scaleX, scaleY));
    if (leftCfg.Height < 1) leftCfg.Height = 1;

    // Left eye uses preset slopes as-is (no mirroring)
    drawEyeShape(leftCX, leftCY, &leftCfg);

    // Track render bounds for overlays
    render.leftX = leftCX - leftCfg.Width / 2;
    render.leftY = leftCY - leftCfg.Height / 2;
    render.leftW = leftCfg.Width;
    render.leftH = leftCfg.Height;
    if (render.leftH < 1) render.leftH = 1;

    if (!params.cyclops) {
        EyeShapeConfig rightCfg = params.rightShape;
        rightCfg.Width   = static_cast<int16_t>(rightCfg.Width   * scaleX);
        rightCfg.Height  = static_cast<int16_t>(rightCfg.Height  * scaleY * openRight);
        rightCfg.OffsetX = static_cast<int16_t>(rightCfg.OffsetX * scaleX);
        rightCfg.OffsetY = static_cast<int16_t>(rightCfg.OffsetY * scaleY);
        rightCfg.Radius_Top    = static_cast<int16_t>(rightCfg.Radius_Top    * std::min(scaleX, scaleY));
        rightCfg.Radius_Bottom = static_cast<int16_t>(rightCfg.Radius_Bottom * std::min(scaleX, scaleY));
        if (rightCfg.Height < 1) rightCfg.Height = 1;

        // RIGHT eye is mirrored (matching esp32-eyes IsMirrored on RightEye)
        EyeShapeConfig rightMirrored = rightCfg;
        rightMirrored.Slope_Top = -rightCfg.Slope_Top;
        rightMirrored.Slope_Bottom = -rightCfg.Slope_Bottom;
        rightMirrored.OffsetX = -rightCfg.OffsetX;

        drawEyeShape(rightCX, rightCY, &rightMirrored);

        render.rightX = rightCX - rightCfg.Width / 2;
        render.rightY = rightCY - rightCfg.Height / 2;
        render.rightW = rightCfg.Width;
        render.rightH = rightCfg.Height;
        if (render.rightH < 1) render.rightH = 1;
    } else {
        render.rightW = 0;
        render.rightH = 0;
    }

    render.borderRadius = static_cast<uint8_t>(std::min(leftCfg.Radius_Top, leftCfg.Radius_Bottom));
    if (render.borderRadius < 2) render.borderRadius = 2;

    // Recompute mouth position now that render bounds are final
    int16_t eyeBottom = std::max(render.leftY + render.leftH, render.rightY + render.rightH);
    render.mouthX = (layout.screenW - layout.mouthWidth) / 2 + gazeOffsetX;
    render.mouthY = eyeBottom + 4;
    render.mouthW = layout.mouthWidth;
    int16_t openAdd = static_cast<int16_t>(params.mouthOpenness * 8);
    render.mouthH = layout.mouthHeight + openAdd + 6;
}

void MochiEyesEngine::drawMouth() {
  if (render.mouthY > layout.screenH - 8)
    return;

  int16_t mx = render.mouthX;
  int16_t my = render.mouthY;
  int16_t mw = render.mouthW;

  if (mx < 0)
    mx = 0;
  if (mx + mw > layout.screenW)
    mx = layout.screenW - mw;

  int16_t openH = (int16_t)(params.mouthOpenness * 8);
  int16_t centerX = mx + mw / 2;

  switch (params.mouthShape) {
  case MOUTH_SMILE:
    if (params.mouthOpenness > 0.1f) {
      int16_t openW = mw - 4;
      int16_t openHt = 4 + openH;
      fillRoundRect(mx + 2, my, openW, openHt, openHt / 2, MAINCOLOR);
      if (openH > 2) {
        fillRoundRect(mx + 4, my + 2, openW - 4, openHt - 4, (openHt - 4) / 2,
                      BGCOLOR);
      }
    } else {
      int16_t smileDepth = 5;
      int16_t thickness = 3;
      for (int16_t x = mx; x <= mx + mw; x++) {
        float dx = (float)(x - centerX);
        float normalizedX = dx / (mw / 2.0f);
        float curve = normalizedX * normalizedX;
        int16_t y = my + smileDepth - (int16_t)(curve * smileDepth);
        for (int16_t t = 0; t < thickness; t++) {
          drawPixel(x, y + t, MAINCOLOR);
        }
      }
    }
    break;

  case MOUTH_FROWN: {
    int16_t frownDepth = 4;
    int16_t thickness = 3;
    for (int16_t x = mx; x <= mx + mw; x++) {
      float dx = (float)(x - centerX);
      float normalizedX = dx / (mw / 2.0f);
      float curve = normalizedX * normalizedX;
      int16_t y = my + (int16_t)(curve * frownDepth);
      for (int16_t t = 0; t < thickness; t++) {
        drawPixel(x, y + t, MAINCOLOR);
      }
    }
    break;
  }

  case MOUTH_OPEN:
    fillRoundRect(mx + 4, my - 2, mw - 8, 10, 4, MAINCOLOR);
    fillRoundRect(mx + 6, my, mw - 12, 6, 3, BGCOLOR);
    break;

  case MOUTH_OOO:
    fillCircle(centerX, my + 3, 5, MAINCOLOR);
    fillCircle(centerX, my + 3, 3, BGCOLOR);
    break;

  case MOUTH_FLAT:
    fillRoundRect(mx + 2, my + 2, mw - 4, 3, 1, MAINCOLOR);
    break;

  case MOUTH_W: {
    int16_t bumpR = mw / 5;
    int16_t wHeight = 6;
    for (int16_t thick = 0; thick < 2; thick++) {
      for (int16_t angle = 0; angle <= 180; angle += 6) {
        float rad = angle * 3.14159f / 180.0f;
        int16_t px = centerX - bumpR - (int16_t)(bumpR * std::cos(rad));
        int16_t py = my + thick + (int16_t)(wHeight * std::sin(rad));
        drawPixel(px, py, MAINCOLOR);
        drawPixel(px + 1, py, MAINCOLOR);
      }
      for (int16_t angle = 0; angle <= 180; angle += 6) {
        float rad = angle * 3.14159f / 180.0f;
        int16_t px = centerX + bumpR + (int16_t)(bumpR * std::cos(rad));
        int16_t py = my + thick + (int16_t)(wHeight * std::sin(rad));
        drawPixel(px, py, MAINCOLOR);
        drawPixel(px - 1, py, MAINCOLOR);
      }
    }
    break;
  }

  case MOUTH_D: {
    int16_t radius = mw / 3;
    int16_t dHeight = 10;
    for (int16_t angle = 0; angle <= 180; angle++) {
      float rad = angle * 3.14159f / 180.0f;
      int16_t x = centerX + (int16_t)(radius * std::cos(rad));
      int16_t y = my + (int16_t)(dHeight * std::sin(rad));
      drawLine(centerX, my, x, y, MAINCOLOR);
    }
    fillTriangle(mx + mw / 2 - radius, my, mx + mw / 2 + radius, my, centerX,
                 my + dHeight, MAINCOLOR);
    break;
  }

  case MOUTH_SMIRK: {
    int16_t shiftY = 2;
    // Tilted flat line, slightly longer
    for (int16_t i=0; i<3; i++) {
        drawLine(mx, my + 3 + i, mx + mw, my - 1 + i, MAINCOLOR);
    }
    // Small hook at the right corner
    fillCircle(mx + mw, my - 2, 2, MAINCOLOR);
    break;
  }

  case MOUTH_ZIGZAG: {
    int16_t segs = 5;
    int16_t segW = mw / segs;
    int16_t px = mx;
    for (int16_t i=0; i<segs; i++) {
        int16_t nextX = (i == segs - 1) ? mx + mw : px + segW;
        int16_t sy = my + ((i % 2 == 0) ? 0 : 4);
        int16_t ny = my + (((i + 1) % 2 == 0) ? 0 : 4);
        for (int16_t thick=0; thick<3; thick++) {
            drawLine(px, sy + thick, nextX, ny + thick, MAINCOLOR);
        }
        px = nextX;
    }
    break;
  }

  case MOUTH_O: {
    fillCircle(centerX, my + 4, 8, MAINCOLOR);
    fillCircle(centerX, my + 4, 5, BGCOLOR);
    break;
  }
  }
}

void MochiEyesEngine::drawHeart(int16_t cx, int16_t cy, float scale) {
  if (scale < 0.1f)
    return;

  float pulse = 1.0f + std::sin(params.heartPulse) * 0.15f;
  scale *= pulse;

  constexpr int kSegments = 64;
  int16_t px[kSegments + 1];
  int16_t py[kSegments + 1];

  const float s = std::max(0.65f, scale * 0.92f);
  for (int i = 0; i <= kSegments; ++i) {
    const float t = (2.0f * 3.1415926f * static_cast<float>(i)) /
                    static_cast<float>(kSegments);
    const float st = std::sin(t);
    const float ct = std::cos(t);
    const float x = 16.0f * st * st * st;
    const float y = 13.0f * ct - 5.0f * std::cos(2.0f * t) -
                    2.0f * std::cos(3.0f * t) - std::cos(4.0f * t);
    px[i] = cx + static_cast<int16_t>(x * s);
    py[i] = cy - static_cast<int16_t>(y * s) + static_cast<int16_t>(2.0f * s);
  }

  for (int i = 0; i < kSegments; ++i) {
    fillTriangle(cx, cy, px[i], py[i], px[i + 1], py[i + 1], MAINCOLOR);
  }
}

void MochiEyesEngine::drawLoveOverlay() {
  if (params.love < 0.1f)
    return;

  int16_t leftCX = render.leftX + render.leftW / 2;
  int16_t leftCY = render.leftY + render.leftH / 2;
  int16_t rightCX = render.rightX + render.rightW / 2;
  int16_t rightCY = render.rightY + render.rightH / 2;

  if (params.heartScale >= 0.9f) {
    int16_t pad = 6;
    fillRect(render.leftX - pad, render.leftY - pad, render.leftW + pad*2,
                  render.leftH + pad*2, BGCOLOR);
    if (!params.cyclops) {
      fillRect(render.rightX - pad, render.rightY - pad, render.rightW + pad*2,
                    render.rightH + pad*2, BGCOLOR);
    }
  }

  drawHeart(leftCX, leftCY, params.heartScale);
  if (!params.cyclops) {
    drawHeart(rightCX, rightCY, params.heartScale);
  }

  if (params.love > 0.3f) {
    int16_t blushW = (int16_t)(10 * params.love);
    int16_t blushH = (int16_t)(5 * params.love);
    fillRoundRect(render.leftX - 12, render.leftY + render.leftH - 5, blushW,
                  blushH, 2, MAINCOLOR);
    if (!params.cyclops) {
      fillRoundRect(render.rightX + render.rightW + 2,
                    render.rightY + render.rightH - 5, blushW, blushH, 2,
                    MAINCOLOR);
    }
  }
}

void MochiEyesEngine::drawUwUOverlay() {
  if (params.uwuIntensity < 0.1f)
    return;

  float intensity = params.uwuIntensity;
  int16_t leftCX = render.leftX + render.leftW / 2;
  int16_t leftCY = render.leftY + render.leftH / 2;
  int16_t rightCX = render.rightX + render.rightW / 2;
  int16_t rightCY = render.rightY + render.rightH / 2;

  if (intensity > 0.5f) {
    int16_t pad = 6;
    fillRect(render.leftX - pad, render.leftY - pad, render.leftW + pad*2,
                  render.leftH + pad*2, BGCOLOR);
    if (!params.cyclops) {
      fillRect(render.rightX - pad, render.rightY - pad, render.rightW + pad*2,
                    render.rightH + pad*2, BGCOLOR);
    }
    fillRect(render.mouthX - 2, render.mouthY - 2, render.mouthW + 4,
             render.mouthH + 6, BGCOLOR);
  }

  int16_t uWidth = (int16_t)(render.leftW * 0.6f * intensity);
  int16_t uHeight = (int16_t)(render.leftH * 0.7f * intensity);
  if (uWidth < 12)
    uWidth = 12;
  if (uHeight < 14)
    uHeight = 14;

  auto drawU = [&](int16_t cx, int16_t cy, bool mirror) {
    int16_t halfW = uWidth / 2;
    int16_t legH = uHeight - halfW;

    float startR = 1.0f;
    float medR = 2.0f;
    float endR = 3.0f;

    int16_t totalSteps = legH * 2 + (int16_t)(3.14159f * halfW);

    for (int16_t step = 0; step <= totalSteps; step++) {
      float t = (float)step / (float)totalSteps;
      int16_t px, py;
      float radius;

      if (t < 0.35f) {
        float legT = t / 0.35f;
        px = cx - halfW;
        py = cy - uHeight / 2 + (int16_t)(legT * legH);
        if (!mirror)
          radius = startR + legT * (medR - startR);
        else
          radius = endR - legT * (endR - medR);
      } else if (t > 0.65f) {
        float legT = (t - 0.65f) / 0.35f;
        px = cx + halfW;
        py = cy - uHeight / 2 + legH - (int16_t)(legT * legH);
        if (!mirror)
          radius = medR + legT * (endR - medR);
        else
          radius = medR - legT * (medR - startR);
      } else {
        float curveT = (t - 0.35f) / 0.3f;
        float angle = 3.14159f * curveT;
        px = cx - (int16_t)(halfW * std::cos(angle));
        py = cy - uHeight / 2 + legH + (int16_t)(halfW * std::sin(angle));
        radius = medR;
      }

      if (radius >= 1.0f)
        fillCircle(px, py, (int16_t)radius, MAINCOLOR);
      else
        drawPixel(px, py, MAINCOLOR);
    }
  };

  drawU(leftCX, leftCY, false);
  if (!params.cyclops) {
    drawU(rightCX, rightCY, true);
  }

  int16_t mouthCX = layout.centerX;
  int16_t mouthY = render.mouthY + 1;
  int16_t wWidth = (int16_t)(26 * intensity);
  int16_t wHeight = (int16_t)(10 * intensity);
  if (wWidth < 14)
    wWidth = 14;
  if (wHeight < 6)
    wHeight = 6;
  int16_t bumpR = wWidth / 4;

  float edgeThick = 1.0f;
  float centerThick = 2.5f;

  for (int16_t angle = 0; angle <= 180; angle += 4) {
    float t = (float)(180 - angle) / 180.0f;
    float rad = angle * 3.14159f / 180.0f;
    int16_t px = mouthCX - bumpR - (int16_t)(bumpR * std::cos(rad));
    int16_t py = mouthY + (int16_t)(wHeight * std::sin(rad));
    float radius = edgeThick + t * (centerThick - edgeThick);
    if (radius > 1.0f)
      fillCircle(px, py, (int16_t)radius, MAINCOLOR);
    else
      drawPixel(px, py, MAINCOLOR);
  }

  for (int16_t angle = 0; angle <= 180; angle += 4) {
    float t = (float)(180 - angle) / 180.0f;
    float rad = angle * 3.14159f / 180.0f;
    int16_t px = mouthCX + bumpR + (int16_t)(bumpR * std::cos(rad));
    int16_t py = mouthY + (int16_t)(wHeight * std::sin(rad));
    float radius = edgeThick + t * (centerThick - edgeThick);
    if (radius > 1.0f)
      fillCircle(px, py, (int16_t)radius, MAINCOLOR);
    else
      drawPixel(px, py, MAINCOLOR);
  }

  if (intensity > 0.3f) {
    int16_t blushW = (int16_t)(14 * intensity);
    int16_t blushH = (int16_t)(5 * intensity);
    int16_t blushY = leftCY + uHeight / 2 + 2;
    fillRoundRect(render.leftX + render.leftW / 2 - uWidth / 2 - blushW / 2 - 4,
                  blushY, blushW, blushH, 10, MAINCOLOR);
    if (!params.cyclops) {
      fillRoundRect(render.rightX + render.rightW / 2 + uWidth / 2 -
                        blushW / 2 + 4,
                    blushY, blushW, blushH, 10, MAINCOLOR);
    }
  }
}

void MochiEyesEngine::drawXDOverlay() {
  if (params.xdIntensity < 0.1f)
    return;

  float intensity = params.xdIntensity;
  int16_t leftCX = render.leftX + render.leftW / 2;
  int16_t leftCY = render.leftY + render.leftH / 2;
  int16_t rightCX = render.rightX + render.rightW / 2;
  int16_t rightCY = render.rightY + render.rightH / 2;

  if (intensity > 0.5f) {
    int16_t pad = 6;
    fillRect(render.leftX - pad, render.leftY - pad, render.leftW + pad*2,
                  render.leftH + pad*2, BGCOLOR);
    if (!params.cyclops) {
      fillRect(render.rightX - pad, render.rightY - pad, render.rightW + pad*2,
                    render.rightH + pad*2, BGCOLOR);
    }
    fillRect(render.mouthX - 4, render.mouthY - 2, render.mouthW + 8,
             render.mouthH + 8, BGCOLOR);
  }

  int16_t eyeSize = (int16_t)(render.leftW * 0.7f * intensity);
  if (eyeSize < 12)
    eyeSize = 12;
  int16_t stroke = 3;

  auto drawChevron = [&](int16_t cx, int16_t cy, bool pointRight) {
    int16_t hSize = eyeSize / 2;
    int16_t vSize = eyeSize / 2;
    if (pointRight) {
      for (int i = 0; i < stroke; i++) {
        drawLine(cx - hSize, cy - vSize + i, cx + hSize, cy + i, MAINCOLOR);
        drawLine(cx - hSize, cy - vSize + i - 1, cx + hSize, cy + i - 1,
                 MAINCOLOR);
      }
      for (int i = 0; i < stroke; i++) {
        drawLine(cx - hSize, cy + vSize - i, cx + hSize, cy - i, MAINCOLOR);
        drawLine(cx - hSize, cy + vSize - i + 1, cx + hSize, cy - i + 1,
                 MAINCOLOR);
      }
    } else {
      for (int i = 0; i < stroke; i++) {
        drawLine(cx + hSize, cy - vSize + i, cx - hSize, cy + i, MAINCOLOR);
        drawLine(cx + hSize, cy - vSize + i - 1, cx - hSize, cy + i - 1,
                 MAINCOLOR);
      }
      for (int i = 0; i < stroke; i++) {
        drawLine(cx + hSize, cy + vSize - i, cx - hSize, cy - i, MAINCOLOR);
        drawLine(cx + hSize, cy + vSize - i + 1, cx - hSize, cy - i + 1,
                 MAINCOLOR);
      }
    }
  };

  drawChevron(leftCX, leftCY, true);
  if (!params.cyclops) {
    drawChevron(rightCX, rightCY, false);
  }

  int16_t mouthW = (int16_t)(20 * intensity);
  int16_t mouthH = (int16_t)(14 * intensity);
  int16_t mouthX = layout.centerX - mouthW / 2;
  int16_t mouthY = render.mouthY;
  int16_t radius = mouthW / 2;

  for (int16_t angle = 0; angle <= 180; angle++) {
    float rad = angle * 3.14159f / 180.0f;
    int16_t x = layout.centerX + (int16_t)(radius * std::cos(rad));
    int16_t y = mouthY + (int16_t)(mouthH * std::sin(rad));
    drawLine(layout.centerX, mouthY, x, y, MAINCOLOR);
  }
  fillTriangle(mouthX, mouthY, mouthX + mouthW, mouthY, layout.centerX,
               mouthY + mouthH, MAINCOLOR);
}

void MochiEyesEngine::drawTears() {
  if (params.tearProgress <= 0)
    return;

  int16_t tearLX = render.leftX + render.leftW / 2;
  int16_t tearRX = render.rightX + render.rightW / 2;
  int16_t startY = render.leftY + render.leftH;

  int16_t y1 =
      startY + (int16_t)std::fmod(params.tearProgress, layout.screenH - startY);
  int16_t y2 = startY + (int16_t)std::fmod(params.tearProgress + 10,
                                           layout.screenH - startY);

  int16_t tearSize = 4;
  if (y1 < layout.screenH - tearSize) {
    fillCircle(tearLX, y1 + tearSize, tearSize, MAINCOLOR);
    fillTriangle(tearLX - tearSize + 1, y1 + tearSize, tearLX + tearSize - 1,
                 y1 + tearSize, tearLX, y1, MAINCOLOR);
  }
  if (!params.cyclops && y2 < layout.screenH - tearSize) {
    fillCircle(tearRX, y2 + tearSize, tearSize, MAINCOLOR);
    fillTriangle(tearRX - tearSize + 1, y2 + tearSize, tearRX + tearSize - 1,
                 y2 + tearSize, tearRX, y2, MAINCOLOR);
  }
}

void MochiEyesEngine::drawSpiral(int16_t cx, int16_t cy, int16_t maxRadius) {
  float angle = params.spiralAngle;
  float radius = 3;
  int prevX = cx, prevY = cy;

  while (radius < maxRadius) {
    int x = cx + (int)(std::cos(angle) * radius);
    int y = cy + (int)(std::sin(angle) * radius);
    drawLine(prevX, prevY, x, y, MAINCOLOR);
    drawLine(prevX + 1, prevY, x + 1, y, MAINCOLOR);
    drawLine(prevX, prevY + 1, x, y + 1, MAINCOLOR);
    prevX = x;
    prevY = y;
    angle += 0.25f;
    radius += 0.5f;
  }
}

void MochiEyesEngine::drawKnockedOverlay() {
  if (params.knockedIntensity < 0.05f)
    return;

  int16_t spiralR = std::min(render.leftW, render.leftH) / 2 + 4;
  if (spiralR < 12)
    spiralR = 12;
  spiralR = (int16_t)(spiralR * params.knockedIntensity);
  if (spiralR < 6)
    spiralR = 6;

  if (params.knockedIntensity > 0.5f) {
    fillRoundRect(render.leftX - 1, render.leftY - 1, render.leftW + 2,
                  render.leftH + 2, render.borderRadius, BGCOLOR);
    if (!params.cyclops) {
      fillRoundRect(render.rightX - 1, render.rightY - 1, render.rightW + 2,
                    render.rightH + 2, render.borderRadius, BGCOLOR);
    }
  }

  drawSpiral(render.leftX + render.leftW / 2, render.leftY + render.leftH / 2,
             spiralR);
  if (!params.cyclops) {
    drawSpiral(render.rightX + render.rightW / 2,
               render.rightY + render.rightH / 2, spiralR);
  }
}

void MochiEyesEngine::drawSweat() {
  if (params.sweatIntensity < 0.1f)
    return;

  for (int i = 0; i < 3; i++) {
    sweatY[i] += 0.5f * params.sweatIntensity;
    if (sweatY[i] > 20 + (std::rand() % 10)) {
      if (i == 0)
        sweatX[i] = std::rand() % 30;
      else if (i == 1)
        sweatX[i] = 30 + (std::rand() % (layout.screenW - 60));
      else
        sweatX[i] = layout.screenW - 30 + (std::rand() % 30);
      sweatY[i] = 2;
      sweatSize[i] = 2;
    }

    if (sweatY[i] < 15)
      sweatSize[i] += 0.3f;
    else
      sweatSize[i] -= 0.1f;
    if (sweatSize[i] < 1)
      sweatSize[i] = 1;

    float scaledSize = sweatSize[i] * params.sweatIntensity;
    if (scaledSize >= 1.0f) {
      fillRoundRect((int16_t)sweatX[i], (int16_t)sweatY[i], (int16_t)scaledSize,
                    (int16_t)(scaledSize * 1.5f), 3, MAINCOLOR);
    }
  }
}

// ----------------------------------------------------------------------------
// API Mappings and Remaining Implementations
// ----------------------------------------------------------------------------

void MochiEyesEngine::setOpenness(float target, float speed) {
  targets.openness = clampf(target, 0.0f, 1.0f);
  targets.opennessSpeed = speed;
}

void MochiEyesEngine::setSquish(float target, float speed) {
  targets.squish = clampf(target, 0.5f, 1.5f);
  targets.squishSpeed = speed;
}

void MochiEyesEngine::setGaze(float x, float y, float speed) {
  targets.gazeX = clampf(x, -1.0f, 1.0f);
  targets.gazeY = clampf(y, -1.0f, 1.0f);
  targets.gazeSpeed = speed;
}

void MochiEyesEngine::setMouthShape(MouthShape shape) {
  if (params.targetMouthShape != shape) {
    params.targetMouthShape = shape;
    params.mouthTransition = 0.0f;
  }
}

void MochiEyesEngine::setMouthOpenness(float target, float speed) {
  targets.mouthOpenness = clampf(target, 0.0f, 1.0f);
  targets.mouthSpeed = speed;
}

void MochiEyesEngine::setJoy(float weight, float speed) {
  targets.joy = clampf(weight, 0.0f, 1.0f);
  targets.emotionSpeed = speed;
}

void MochiEyesEngine::setAnger(float weight, float speed) {
  targets.anger = clampf(weight, 0.0f, 1.0f);
  targets.emotionSpeed = speed;
}

void MochiEyesEngine::setFatigue(float weight, float speed) {
  targets.fatigue = clampf(weight, 0.0f, 1.0f);
  targets.emotionSpeed = speed;
}

void MochiEyesEngine::setLove(float weight, float speed) {
  targets.love = clampf(weight, 0.0f, 1.0f);
  targets.emotionSpeed = speed;
}

void MochiEyesEngine::resetEmotions() {
  clearAllOverlays();
  targets.joy = 0.0f;
  targets.anger = 0.0f;
  targets.fatigue = 0.0f;
  targets.love = 0.0f;
  targets.heartScale = 0.0f;
  targets.openness = 1.0f;
  targets.leftOpenness = 1.0f;
  targets.rightOpenness = 1.0f;
  targets.squish = 1.0f;
  targets.mouthOpenness = 0.0f;
  timers.mouthAnimRemaining = 0.0f;
  timers.mouthAnimType = 0;
  setExpression(EXPR_NORMAL);
}

void MochiEyesEngine::blink() {
  targets.openness = 0.0f;
  targets.openness = 1.0f;
  params.openness = 0.0f;
}

void MochiEyesEngine::wink(bool left) {
  if (left) {
    targets.leftOpenness = 0.0f;
    params.leftOpenness = 0.0f;
    targets.leftOpenness = 1.0f;
    params.rightOpenness = 0.7f;
    targets.rightOpenness = 1.0f;
  } else {
    targets.rightOpenness = 0.0f;
    params.rightOpenness = 0.0f;
    targets.rightOpenness = 1.0f;
    params.leftOpenness = 0.7f;
    targets.leftOpenness = 1.0f;
  }
  params.squish = 0.95f;
  targets.squish = 1.0f;
}

void MochiEyesEngine::close() { targets.openness = 0.0f; }

void MochiEyesEngine::open() { targets.openness = 1.0f; }

void MochiEyesEngine::clearTimedOverlays() {
  targets.knockedIntensity = 0.0f;
  params.knockedIntensity = 0.0f;
  targets.uwuIntensity = 0.0f;
  params.uwuIntensity = 0.0f;
  targets.xdIntensity = 0.0f;
  params.xdIntensity = 0.0f;
  targets.love = 0.0f;
  params.love = 0.0f;
  targets.fatigue = 0.0f;
  params.fatigue = 0.0f;
  params.tearProgress = 0.0f;
  params.laughIntensity = 0.0f;
  params.hFlicker = 0.0f;
  params.vFlicker = 0.0f;
}

void MochiEyesEngine::clearCuriousGaze() {
  targets.curiousIntensity = 0.0f;
  params.curiousPhase = 0.0f;
}

void MochiEyesEngine::clearAllOverlays() {
  clearTimedOverlays();
  targets.curiousIntensity = 0.0f;
  params.curiousIntensity = 0.0f;
  targets.sweatIntensity = 0.0f;
  params.sweatIntensity = 0.0f;
  targets.sleepIntensity = 0.0f;
  params.sleepIntensity = 0.0f;
  params.curiousPhase = 0.0f;
  params.confusedIntensity = 0.0f;
  params.confusedPhase = 0.0f;
  params.laughIntensity = 0.0f;
  params.laughPhase = 0.0f;
  targets.gazeX = 0.0f;
  targets.gazeY = 0.0f;
  targets.mouthOpenness = 0.0f;
}

void MochiEyesEngine::triggerLove(float durationSec) {
  clearAllOverlays();
  targets.love = 1.0f;
  targets.heartScale = 1.0f;
  params.heartPulse = 0.0f;
}

void MochiEyesEngine::triggerCry(float durationSec) {
  clearAllOverlays();
  setExpression(EXPR_SAD);
  targets.fatigue = 0.5f;
  params.tearProgress = 0.0f;
}

void MochiEyesEngine::triggerConfused(float durationSec) {
  clearAllOverlays();
  params.confusedIntensity = 1.0f;
  params.confusedPhase = 0.0f;
}

void MochiEyesEngine::triggerUwU(float duration) {
  clearAllOverlays();
  targets.uwuIntensity = 1.0f;
}

void MochiEyesEngine::triggerXD(float duration) {
  clearAllOverlays();
  targets.xdIntensity = 1.0f;
}

void MochiEyesEngine::triggerLaugh(float durationSec) {
  clearAllOverlays();
  params.laughIntensity = 1.0f;
  params.laughPhase = 0.0f;
}

void MochiEyesEngine::setKnocked(bool on) {
  if (on) {
    clearAllOverlays();
    targets.knockedIntensity = 1.0f;
    params.spiralAngle = 0.0f;
    blink();
  } else {
    targets.knockedIntensity = 0.0f;
  }
}

void MochiEyesEngine::setSweat(bool on) {
  if (on)
    clearTimedOverlays();
  targets.sweatIntensity = on ? 1.0f : 0.0f;
}

void MochiEyesEngine::setCyclops(bool on) { params.cyclops = on; }

void MochiEyesEngine::setAutoblinker(bool active, float interval,
                                     float variation) {
  timers.autoBlink = active;
  timers.blinkInterval = interval;
  timers.blinkVariation = variation;
}

void MochiEyesEngine::setIdleMode(bool active, float interval,
                                  float variation) {
  timers.idleMode = active;
  timers.idleInterval = interval;
  timers.idleVariation = variation;
  if (active)
    timers.idleCooldown = 0.5f;
}

void MochiEyesEngine::setBreathing(bool active, float intensity, float speed) {
  timers.breathingEnabled = active;
  timers.breathingIntensity = intensity;
  timers.breathingSpeed = speed;
  if (!active)
    targets.squish = 1.0f;
}

void MochiEyesEngine::setBreathingIntensity(float intensity) {
  timers.breathingIntensity = clampf(intensity, 0.0f, 0.2f);
}

void MochiEyesEngine::setBreathingSpeed(float speed) {
  timers.breathingSpeed = clampf(speed, 0.1f, 1.0f);
}

void MochiEyesEngine::setWidth(int16_t left, int16_t right) {
  layout.baseWidth = left;
  layout.recompute();
}

void MochiEyesEngine::setHeight(int16_t left, int16_t right) {
  layout.baseHeight = left;
  layout.recompute();
}

void MochiEyesEngine::setSpacebetween(int16_t space) {
  layout.spacing = space;
  layout.recompute();
}

void MochiEyesEngine::setBorderradius(int16_t left, int16_t right) {
  layout.borderRadius = left;
}

void MochiEyesEngine::setMouthSize(int16_t width, int16_t height) {
  layout.mouthWidth = width;
  layout.mouthHeight = height;
}

void MochiEyesEngine::setDisplayColors(uint8_t bg, uint8_t main) {
  BGCOLOR = bg;
  MAINCOLOR = main;
}

void MochiEyesEngine::setExpression(Expression expr) {
  if (expr >= EXPR_COUNT) expr = EXPR_NORMAL;
  params.currentExpression = expr;
  params.leftShapeTarget  = kPresets[expr].left;
  params.rightShapeTarget = kPresets[expr].right;
}

void MochiEyesEngine::set_expression(int expr) {
  if (expr >= 0 && expr < EXPR_COUNT)
    setExpression(static_cast<Expression>(expr));
}

void MochiEyesEngine::setMood(uint8_t mood) {
  resetEmotions();
  targets.mouthOpenness = 0.0f;
  params.mouthOpenness = 0.0f;
  timers.mouthAnimType = 0;
  timers.mouthAnimRemaining = 0.0f;
  setMouthShape(MOUTH_SMILE);
  switch (mood) {
  case 1: // TIRED
    setExpression(EXPR_SLEEPY);
    break;
  case 2: // ANGRY
    setExpression(EXPR_ANGRY);
    break;
  case 3: // HAPPY
    setExpression(EXPR_HAPPY);
    break;
  default: // DEFAULT
    setExpression(EXPR_NORMAL);
    break;
  }
}

void MochiEyesEngine::setPosition(uint8_t pos) {
  clearCuriousGaze();
  switch (pos) {
  case 1:
    setGaze(0, -1);
    break;
  case 2:
    setGaze(1, -1);
    break;
  case 3:
    setGaze(1, 0);
    break;
  case 4:
    setGaze(1, 1);
    break;
  case 5:
    setGaze(0, 1);
    break;
  case 6:
    setGaze(-1, 1);
    break;
  case 7:
    setGaze(-1, 0);
    break;
  case 8:
    setGaze(-1, -1);
    break;
  default:
    setGaze(0, 0);
    break;
  }
}

void MochiEyesEngine::setMouthType(int type) {
  MouthShape shape = MOUTH_SMILE;
  switch (type) {
  case 1:
    shape = MOUTH_SMILE;
    break;
  case 2:
    shape = MOUTH_FROWN;
    break;
  case 3:
    shape = MOUTH_OPEN;
    break;
  case 4:
    shape = MOUTH_OOO;
    break;
  case 5:
    shape = MOUTH_FLAT;
    break;
  case 6:
    shape = MOUTH_W;
    break;
  case 7:
    shape = MOUTH_D;
    break;
  case 8:
    shape = MOUTH_SMIRK;
    break;
  case 9:
    shape = MOUTH_ZIGZAG;
    break;
  case 10:
    shape = MOUTH_O;
    break;
  default:
    shape = MOUTH_SMILE;
    break;
  }
  setMouthShape(shape);
}

void MochiEyesEngine::setMouthEnabled(bool enabled) {}

void MochiEyesEngine::setCuriosity(bool on) {
  if (on) {
    clearTimedOverlays();
    params.curiousPhase = 0.0f;
  }
  targets.curiousIntensity = on ? 1.0f : 0.0f;
}

void MochiEyesEngine::setHFlicker(bool on, uint8_t amplitude) {
  params.hFlicker = on ? amplitude : 0;
}

void MochiEyesEngine::setVFlicker(bool on, uint8_t amplitude) {
  params.vFlicker = on ? amplitude : 0;
}

void MochiEyesEngine::setEyebrows(bool raised) {}

void MochiEyesEngine::startMouthAnim(int anim, unsigned long duration) {
  clearAllOverlays();
  timers.mouthAnimRemaining = duration / 1000.0f;
  timers.mouthAnimType = anim;
}

void MochiEyesEngine::triggerSleep() {
  clearAllOverlays();
  resetEmotions();
  // Disable auto-blink and idle mode — they set targets.openness = 1.0
  // which fights the closing animation and re-opens the eyes.
  timers.autoBlink = false;
  timers.idleMode = false;
  // Ensure eyes are fully open before the closing animation starts,
  // otherwise isSleepDone() may fire immediately if eyes were mid-blink.
  params.openness = 1.0f;
  params.leftOpenness = 1.0f;
  params.rightOpenness = 1.0f;
  setOpennessSpeed(1.5f);  // slow, graceful close
  close();
  targets.sleepIntensity = 1.0f;
  params.sleepPhase = 0.0f;
  setMouthShape(MOUTH_FLAT);
}

bool MochiEyesEngine::isSleepDone() const {
  return params.openness < 0.05f && params.sleepIntensity > 0.9f;
}

void MochiEyesEngine::drawSleepOverlay() {
  if (params.sleepIntensity < 0.3f)
    return;

  // Float "Zzz" from bottom-right of right eye upward
  int16_t baseX = render.rightX + render.rightW - 4;
  int16_t baseY = render.rightY;

  // Phase controls vertical drift and size cycling
  float phase = params.sleepPhase;
  int16_t drift = static_cast<int16_t>(phase * 12.0f);
  int16_t zY = baseY - drift;

  // Wrap the drift so it loops
  if (drift > 30) {
    params.sleepPhase = 0.0f;
    return;
  }

  // Fade opacity with height (draw when visible)
  if (zY > 0 && zY < layout.screenH) {
    display_.set_color(MAINCOLOR);
    display_.set_font_small();

    // Small "z" further along, bigger "Zz" near start
    if (drift < 10) {
      display_.draw_text(baseX + 2, zY, "z");
    } else if (drift < 20) {
      display_.draw_text(baseX, zY, "Zz");
    } else {
      display_.draw_text(baseX - 2, zY, "Zzz");
    }
  }
}

} // namespace leor
