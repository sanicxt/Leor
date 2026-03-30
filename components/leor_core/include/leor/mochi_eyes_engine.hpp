#pragma once

#include "leor/display_backend.hpp"

#include <cmath>
#include <cstdint>

namespace leor {

// ---------------------------------------------------------------------------
// Parametric eye shape (ported from esp32-eyes EyeConfig)
// ---------------------------------------------------------------------------
struct EyeShapeConfig {
    int16_t OffsetX = 0;
    int16_t OffsetY = 0;
    int16_t Height  = 40;
    int16_t Width   = 40;
    float   Slope_Top    = 0.0f;
    float   Slope_Bottom = 0.0f;
    int16_t Radius_Top    = 8;
    int16_t Radius_Bottom = 8;
};

// ---------------------------------------------------------------------------
// Expression enum – all 18 esp32-eyes emotions
// ---------------------------------------------------------------------------
enum Expression : uint8_t {
    EXPR_NORMAL = 0,
    EXPR_ANGRY,
    EXPR_GLEE,
    EXPR_HAPPY,
    EXPR_SAD,
    EXPR_WORRIED,
    EXPR_FOCUSED,
    EXPR_ANNOYED,
    EXPR_SURPRISED,
    EXPR_SKEPTIC,
    EXPR_FRUSTRATED,
    EXPR_UNIMPRESSED,
    EXPR_SLEEPY,
    EXPR_SUSPICIOUS,
    EXPR_SQUINT,
    EXPR_FURIOUS,
    EXPR_SCARED,
    EXPR_AWE,
    EXPR_COUNT
};

// On/Off
#define ON 1
#define OFF 0

// Mood types
#define DEFAULT 0
#define TIRED 1
#define ANGRY 2
#define HAPPY 3

// Position constants
#define POS_N 1  // north
#define POS_NE 2 // north-east
#define POS_E 3  // east
#define POS_SE 4 // south-east
#define POS_S 5  // south
#define POS_SW 6 // south-west
#define POS_W 7  // west
#define POS_NW 8 // north-west

enum MouthShape : int8_t {
  MOUTH_SMILE = 0,
  MOUTH_FROWN = 1,
  MOUTH_OPEN = 2,
  MOUTH_OOO = 3,
  MOUTH_FLAT = 4,
  MOUTH_W = 5, // UwU cat mouth
  MOUTH_D = 6, // XD open mouth
  MOUTH_SMIRK = 7,  // 8: Tilted smirk
  MOUTH_ZIGZAG = 8, // 9: Jagged frustrated mouth
  MOUTH_O = 9       // 10: Large open circle
};

struct EyeLayout {
  int16_t screenW;
  int16_t screenH;
  int16_t baseWidth;
  int16_t baseHeight;
  int16_t spacing;
  int16_t borderRadius;

  int16_t mouthWidth;
  int16_t mouthHeight;

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

struct EyeParams {
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
  MouthShape mouthShape;
  MouthShape targetMouthShape;
  float mouthTransition;
  float heartScale;
  float heartPulse;
  float tearProgress;
  float spiralAngle;
  float knockedIntensity;
  float sweatIntensity;
  float curiousIntensity;
  float uwuIntensity;
  float xdIntensity;
  float confusedIntensity;
  float laughIntensity;
  bool cyclops;
  float curiousPhase;
  float confusedPhase;
  float laughPhase;
  float hFlicker;
  float vFlicker;
  float sleepIntensity;
  float sleepPhase;

  // Parametric eye shape state
  EyeShapeConfig leftShape;
  EyeShapeConfig rightShape;
  EyeShapeConfig leftShapeTarget;
  EyeShapeConfig rightShapeTarget;
  float shapeBlend;
  Expression currentExpression;

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
    mouthTransition = 1.0f;
    heartScale = 0.0f;
    heartPulse = 0.0f;
    tearProgress = 0.0f;
    spiralAngle = 0.0f;
    knockedIntensity = 0.0f;
    sweatIntensity = 0.0f;
    curiousIntensity = 0.0f;
    uwuIntensity = 0.0f;
    xdIntensity = 0.0f;
    confusedIntensity = 0.0f;
    laughIntensity = 0.0f;
    cyclops = false;
    curiousPhase = 0.0f;
    confusedPhase = 0.0f;
    laughPhase = 0.0f;
    hFlicker = 0.0f;
    vFlicker = 0.0f;
    sleepIntensity = 0.0f;
    sleepPhase = 0.0f;
    leftShape = {};
    rightShape = {};
    leftShapeTarget = {};
    rightShapeTarget = {};
    shapeBlend = 1.0f;
    currentExpression = EXPR_NORMAL;
  }
};

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
  float knockedIntensity;
  float sweatIntensity;
  float curiousIntensity;
  float uwuIntensity;
  float xdIntensity;
  float sleepIntensity;

  float opennessSpeed;
  float squishSpeed;
  float gazeSpeed;
  float emotionSpeed;
  float mouthSpeed;
  float heartSpeed;
  float effectSpeed;

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
    sleepIntensity = 0.0f;

    opennessSpeed = 12.0f;
    squishSpeed = 10.0f;
    gazeSpeed = 6.0f;
    emotionSpeed = 5.0f;
    mouthSpeed = 15.0f;
    heartSpeed = 8.0f;
    effectSpeed = 4.0f;
  }
};

struct RenderState {
  int16_t leftX, leftY, leftW, leftH;
  int16_t rightX, rightY, rightW, rightH;
  int16_t mouthX, mouthY, mouthW, mouthH;
  uint8_t borderRadius;

  int16_t minX, minY, maxX, maxY;
  int16_t oldMinX, oldMinY, oldMaxX, oldMaxY;

  void resetDirty() {
    minX = 1000;
    minY = 1000;
    maxX = -1000;
    maxY = -1000;
  }

  void expandDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (x < minX)
      minX = x;
    if (y < minY)
      minY = y;
    if (x + w > maxX)
      maxX = x + w;
    if (y + h > maxY)
      maxY = y + h;
  }

  void saveOldDirty() {
    oldMinX = minX;
    oldMinY = minY;
    oldMaxX = maxX;
    oldMaxY = maxY;
  }
};

struct AnimationTimers {
  float mouthAnimRemaining;
  int mouthAnimType;

  float blinkCooldown;
  float blinkInterval;
  float blinkVariation;
  bool autoBlink;

  float idleCooldown;
  float idleInterval;
  float idleVariation;
  bool idleMode;

  float breathingPhase;
  float breathingSpeed;
  float breathingIntensity;
  bool breathingEnabled;

  int nextBlinkType;

  void reset() {
    mouthAnimType = 0;
    blinkCooldown = 2.0f;
    blinkInterval = 3.0f;
    blinkVariation = 3.0f;
    autoBlink = true;
    idleCooldown = 0.0f;
    idleInterval = 2.0f;
    idleVariation = 3.0f;
    idleMode = false;
    breathingPhase = 0.0f;
    breathingSpeed = 0.3f;
    breathingIntensity = 0.08f;
    breathingEnabled = true;
    nextBlinkType = 0;
  }
};

class MochiEyesEngine {
public:
  explicit MochiEyesEngine(DisplayBackend &display);

  void begin();
  void update(uint32_t now_ms);

  void setOpenness(float target, float speed = 8.0f);
  void setSquish(float target, float speed = 6.0f);
  void setGaze(float x, float y, float speed = 4.0f);
  void setMouthShape(MouthShape shape);
  void setMouthOpenness(float target, float speed = 10.0f);

  void setJoy(float weight, float speed = 3.0f);
  void setAnger(float weight, float speed = 3.0f);
  void setFatigue(float weight, float speed = 3.0f);
  void setLove(float weight, float speed = 3.0f);
  void resetEmotions();

  // Parametric expression system (18 esp32-eyes emotions)
  void setExpression(Expression expr);
  void set_expression(int expr);

  void blink();
  void wink(bool left);
  void close();
  void open();

  void clearTimedOverlays();
  void clearCuriousGaze();
  void clearAllOverlays();

  void triggerLove(float durationSec = 2.0f);
  void triggerCry(float durationSec = 0);
  void triggerConfused(float durationSec = 0);
  void triggerUwU(float duration = 0);
  void triggerXD(float duration = 0);
  void triggerLaugh(float durationSec = 0);
  void triggerSleep();
  bool isSleepDone() const;
  void setKnocked(bool on);
  void setSweat(bool on);
  void setCyclops(bool on);

  void setAutoblinker(bool active, float interval = 3.0f,
                      float variation = 3.0f);
  void setIdleMode(bool active, float interval = 2.0f, float variation = 3.0f);
  void setBreathing(bool active, float intensity, float speed);
  void setBreathing(bool active) {
    setBreathing(active, timers.breathingIntensity, timers.breathingSpeed);
  }
  void setBreathingIntensity(float intensity);
  void setBreathingSpeed(float speed);

  bool getBreathingEnabled() const { return timers.breathingEnabled; }
  float getBreathingIntensity() const { return timers.breathingIntensity; }
  float getBreathingSpeed() const { return timers.breathingSpeed; }

  void setGazeSpeed(float speed) { targets.gazeSpeed = speed; }
  void setOpennessSpeed(float speed) { targets.opennessSpeed = speed; }
  void setSquishSpeed(float speed) { targets.squishSpeed = speed; }

  void setWidth(int16_t left, int16_t right);
  void setHeight(int16_t left, int16_t right);
  void setSpacebetween(int16_t space);
  void setBorderradius(int16_t left, int16_t right);
  void setMouthSize(int16_t width, int16_t height);
  void setDisplayColors(uint8_t bg, uint8_t main);

  int16_t getEyeWidth() const { return layout.baseWidth; }
  int16_t getEyeHeight() const { return layout.baseHeight; }
  int16_t getSpaceBetween() const { return layout.spacing; }
  int16_t getBorderRadius() const { return layout.borderRadius; }
  int16_t getMouthWidth() const { return layout.mouthWidth; }

  void setMood(uint8_t mood);
  void setPosition(uint8_t pos);
  void setMouthType(int type);
  void setMouthEnabled(bool enabled);
  void setCuriosity(bool on);
  void setHFlicker(bool on, uint8_t amplitude = 2);
  void setVFlicker(bool on, uint8_t amplitude = 2);
  void setEyebrows(bool raised);

  void anim_love() { triggerLove(2.0f); }
  void anim_cry() { triggerCry(3.0f); }
  void anim_confused() { triggerConfused(0.5f); }
  void anim_laugh() { triggerLaugh(1.0f); }
  void anim_knocked() { setKnocked(true); }

  void startMouthAnim(int anim, unsigned long duration);

  // Wrapper methods matching command conventions
  void set_mood(int mood) { setMood(mood); }
  void set_position(int pos) { setPosition(pos); }
  void set_breathing(bool enabled, float intensity, float speed) {
    setBreathing(enabled, intensity, speed);
  }
  void set_breathing(bool enabled) { setBreathing(enabled); }
  void set_breathing_intensity(float val) { setBreathingIntensity(val); }
  void set_breathing_speed(float val) { setBreathingSpeed(val); }

  int16_t eye_width() const { return getEyeWidth(); }
  int16_t eye_height() const { return getEyeHeight(); }
  int16_t space_between() const { return getSpaceBetween(); }
  int16_t border_radius() const { return getBorderRadius(); }
  int16_t mouth_width() const { return getMouthWidth(); }

  bool get_breathing_enabled() const { return getBreathingEnabled(); }
  float get_breathing_intensity() const { return getBreathingIntensity(); }
  float get_breathing_speed() const { return getBreathingSpeed(); }

  void trigger_uwu() { triggerUwU(); }
  void trigger_xd() { triggerXD(); }

  void set_idle_mode(bool active) { setIdleMode(active, 2.0f, 3.0f); }
  void set_idle_mode(bool active, float i, float v) {
    setIdleMode(active, i, v);
  }

  void set_width(int left, int right) { setWidth(left, right); }
  void set_height(int left, int right) { setHeight(left, right); }
  void set_space_between(int space) { setSpacebetween(space); }
  void set_border_radius(int left, int right) { setBorderradius(left, right); }
  void set_mouth_size(int w, int h) { setMouthSize(w, h); }
  void set_autoblinker(bool active, float i, float v) {
    setAutoblinker(active, i, v);
  }
  void set_sweat(bool on) { setSweat(on); }
  void set_knocked(bool on) { setKnocked(on); }
  void set_cyclops(bool on) { setCyclops(on); }
  void set_curiosity(bool on) { setCuriosity(on); }
  void set_hflicker(bool on, int amp) { setHFlicker(on, amp); }
  void set_vflicker(bool on, int amp) { setVFlicker(on, amp); }
  void set_eyebrows(bool raised) { setEyebrows(raised); }
  void set_mouth_enabled(bool enabled) { setMouthEnabled(enabled); }
  void set_mouth_type(int type) { setMouthType(type); }
  void reset_emotions() { resetEmotions(); }
  void trigger_sleep() { triggerSleep(); }
  bool is_sleep_done() const { return isSleepDone(); }
  void start_mouth_anim(int anim, uint32_t duration) {
    startMouthAnim(anim, duration);
  }

private:
  DisplayBackend &display_;

  EyeLayout layout;
  EyeParams params;
  ImpulseTargets targets;
  RenderState render;
  AnimationTimers timers;

  uint32_t lastFrameMs;
  uint32_t frameInterval;

  float sweatY[3];
  float sweatX[3];
  float sweatSize[3];

  uint8_t BGCOLOR = 0;
  uint8_t MAINCOLOR = 1;

  static float lerp(float current, float target, float speed, float dt);
  static float smoothDamp(float current, float target, float speed, float dt);
  static float clampf(float v, float lo, float hi);

  void updateParams(float dt);
  void updateTimers(float dt);
  void computeRenderState();
  void lerpShape(EyeShapeConfig& current, const EyeShapeConfig& target, float speed, float dt);

  void drawEyes();
  void drawEyeShape(int16_t centerX, int16_t centerY, EyeShapeConfig* config);
  enum CornerType { T_R, T_L, B_L, B_R };
  void fillEllipseCorner(CornerType corner, int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint8_t color);
  void drawMouth();
  void drawHeart(int16_t cx, int16_t cy, float scale);
  void drawLoveOverlay();
  void drawUwUOverlay();
  void drawXDOverlay();
  void drawTears();
  void drawSpiral(int16_t cx, int16_t cy, int16_t maxRadius);
  void drawKnockedOverlay();
  void drawSweat();
  void drawSleepOverlay();

  // Graphic helpers
  void fillRoundRect(int x, int y, int w, int h, int r, uint8_t color);
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2,
                    uint8_t color);
  void drawPixel(int x, int y, uint8_t color);
  void drawLine(int x0, int y0, int x1, int y1, uint8_t color);
  void fillRect(int x, int y, int w, int h, uint8_t color);
  void fillCircle(int x, int y, int r, uint8_t color);
};

} // namespace leor
