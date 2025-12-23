/*
 * FluxGarage RoboEyes for OLED Displays V 1.1.1
 * Draws smoothly animated robot eyes on OLED displays, based on the Adafruit GFX 
 * library's graphics primitives, such as rounded rectangles and triangles.
 *   
 * Copyright (C) 2024-2025 Dennis Hoelscher
 * www.fluxgarage.com
 * www.youtube.com/@FluxGarage
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef _FLUXGARAGE_ROBOEYES_H
#define _FLUXGARAGE_ROBOEYES_H


// Display colors
uint8_t BGCOLOR = 0; // background and overlays
uint8_t MAINCOLOR = 1; // drawings

// For mood type switch
#define DEFAULT 0
#define TIRED 1
#define ANGRY 2
#define HAPPY 3

// For turning things on or off
#define ON 1
#define OFF 0

// For switch "predefined positions"
#define N 1 // north, top center
#define NE 2 // north-east, top right
#define E 3 // east, middle right
#define SE 4 // south-east, bottom right
#define S 5 // south, bottom center
#define SW 6 // south-west, bottom left
#define W 7 // west, middle left
#define NW 8 // north-west, top left 
// for middle center set "DEFAULT"

// Heart bitmap for love animation (32x32 pixels, scaled from Tigris Li's design)
#define HEART_BMP_WIDTH 32
#define HEART_BMP_HEIGHT 32
const static unsigned char heart_bitmap[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x03, 0xE0, 0x07, 0xC0,
  0x0F, 0xF8, 0x1F, 0xF0,
  0x1F, 0xFC, 0x3F, 0xF8,
  0x3F, 0xFE, 0x7F, 0xFC,
  0x7F, 0xFF, 0xFF, 0xFE,
  0x7F, 0xFF, 0xFF, 0xFE,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0x7F, 0xFF, 0xFF, 0xFE,
  0x7F, 0xFF, 0xFF, 0xFE,
  0x3F, 0xFF, 0xFF, 0xFC,
  0x3F, 0xFF, 0xFF, 0xFC,
  0x1F, 0xFF, 0xFF, 0xF8,
  0x0F, 0xFF, 0xFF, 0xF0,
  0x07, 0xFF, 0xFF, 0xE0,
  0x03, 0xFF, 0xFF, 0xC0,
  0x01, 0xFF, 0xFF, 0x80,
  0x00, 0xFF, 0xFF, 0x00,
  0x00, 0x7F, 0xFE, 0x00,
  0x00, 0x3F, 0xFC, 0x00,
  0x00, 0x1F, 0xF8, 0x00,
  0x00, 0x0F, 0xF0, 0x00,
  0x00, 0x07, 0xE0, 0x00,
  0x00, 0x03, 0xC0, 0x00,
  0x00, 0x01, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x00
};


// Constructor: takes a reference to the active Adafruit display object (e.g., Adafruit_SSD1327)
// Eg: roboEyes<Adafruit_SSD1327> = eyes(display);
template<typename AdafruitDisplay>
class RoboEyes
{
private:

// Yes, everything is currently still accessible. Be responsible and don't mess things up :)

public:

// Reference to Adafruit display object
AdafruitDisplay *display;

// For general setup - screen size and max. frame rate
int screenWidth = 128; // OLED display width, in pixels
int screenHeight = 64; // OLED display height, in pixels
int frameInterval = 20; // default value for 50 frames per second (1000/50 = 20 milliseconds)
unsigned long fpsTimer = 0; // for timing the frames per second

// For controlling mood types and expressions
bool tired = 0;
bool angry = 0;
bool happy = 0;
bool curious = 0; // if true, draw the outer eye larger when looking left or right
bool cyclops = 0; // if true, draw only one eye
bool eyeL_open = 0; // left eye opened or closed?
bool eyeR_open = 0; // right eye opened or closed?


//*********************************************************************************************
//  Eyes Geometry
//*********************************************************************************************

// EYE LEFT - size and border radius
int eyeLwidthDefault = 36;
int eyeLheightDefault = 36;
int eyeLwidthCurrent = eyeLwidthDefault;
int eyeLheightCurrent = 1; // start with closed eye, otherwise set to eyeLheightDefault
int eyeLwidthNext = eyeLwidthDefault;
int eyeLheightNext = eyeLheightDefault;
int eyeLheightOffset = 0;
// Border Radius
byte eyeLborderRadiusDefault = 8;
byte eyeLborderRadiusCurrent = eyeLborderRadiusDefault;
byte eyeLborderRadiusNext = eyeLborderRadiusDefault;

// EYE RIGHT - size and border radius
int eyeRwidthDefault = eyeLwidthDefault;
int eyeRheightDefault = eyeLheightDefault;
int eyeRwidthCurrent = eyeRwidthDefault;
int eyeRheightCurrent = 1; // start with closed eye, otherwise set to eyeRheightDefault
int eyeRwidthNext = eyeRwidthDefault;
int eyeRheightNext = eyeRheightDefault;
int eyeRheightOffset = 0;
// Border Radius
byte eyeRborderRadiusDefault = 8;
byte eyeRborderRadiusCurrent = eyeRborderRadiusDefault;
byte eyeRborderRadiusNext = eyeRborderRadiusDefault;

// EYE LEFT - Coordinates
int eyeLxDefault = ((screenWidth)-(eyeLwidthDefault+spaceBetweenDefault+eyeRwidthDefault))/2;
int eyeLyDefault = ((screenHeight-eyeLheightDefault)/2);
int eyeLx = eyeLxDefault;
int eyeLy = eyeLyDefault;
int eyeLxNext = eyeLx;
int eyeLyNext = eyeLy;

// EYE RIGHT - Coordinates
int eyeRxDefault = eyeLx+eyeLwidthCurrent+spaceBetweenDefault;
int eyeRyDefault = eyeLy;
int eyeRx = eyeRxDefault;
int eyeRy = eyeRyDefault;
int eyeRxNext = eyeRx;
int eyeRyNext = eyeRy;

// BOTH EYES 
// Eyelid top size
byte eyelidsHeightMax = eyeLheightDefault/2; // top eyelids max height
byte eyelidsTiredHeight = 0;
byte eyelidsTiredHeightNext = eyelidsTiredHeight;
byte eyelidsAngryHeight = 0;
byte eyelidsAngryHeightNext = eyelidsAngryHeight;
// Bottom happy eyelids offset
byte eyelidsHappyBottomOffsetMax = (eyeLheightDefault/2)+3;
byte eyelidsHappyBottomOffset = 0;
byte eyelidsHappyBottomOffsetNext = 0;
// Eyebrows
bool eyebrowsRaised = false;
int eyebrowsOffset = 6;  // How far above eyes the eyebrows appear
int eyebrowsHeight = 3;  // Thickness of eyebrows
// Space between eyes
int spaceBetweenDefault = 10;
int spaceBetweenCurrent = spaceBetweenDefault;
int spaceBetweenNext = 10;


//*********************************************************************************************
//  Macro Animations
//*********************************************************************************************

// Animation - horizontal flicker/shiver
bool hFlicker = 0;
bool hFlickerAlternate = 0;
byte hFlickerAmplitude = 2;

// Animation - vertical flicker/shiver
bool vFlicker = 0;
bool vFlickerAlternate = 0;
byte vFlickerAmplitude = 10;

// Animation - auto blinking
bool autoblinker = 0; // activate auto blink animation
int blinkInterval = 1; // basic interval between each blink in full seconds
int blinkIntervalVariation = 4; // interval variaton range in full seconds, random number inside of given range will be add to the basic blinkInterval, set to 0 for no variation
unsigned long blinktimer = 0; // for organising eyeblink timing

// Animation - idle mode: eyes looking in random directions
bool idle = 0;
int idleInterval = 1; // basic interval between each eye repositioning in full seconds
int idleIntervalVariation = 3; // interval variaton range in full seconds, random number inside of given range will be add to the basic idleInterval, set to 0 for no variation
unsigned long idleAnimationTimer = 0; // for organising eyeblink timing

// Animation - eyes confused: eyes shaking left and right
bool confused = 0;
unsigned long confusedAnimationTimer = 0;
int confusedAnimationDuration = 500;
bool confusedToggle = 1;

// Animation - eyes laughing: eyes shaking up and down
bool laugh = 0;
unsigned long laughAnimationTimer = 0;
int laughAnimationDuration = 1000;  // 1 second for visible mouth animation
bool laughToggle = 1;

// Animation - sweat on the forehead
bool sweat = 0;
byte sweatBorderradius = 3;

// Sweat drop 1
int sweat1XPosInitial = 2;
int sweat1XPos;
float sweat1YPos = 2;
int sweat1YPosMax;
float sweat1Height = 2;
float sweat1Width = 1;

// Sweat drop 2
int sweat2XPosInitial = 2;
int sweat2XPos;
float sweat2YPos = 2;
int sweat2YPosMax;
float sweat2Height = 2;
float sweat2Width = 1;

// Sweat drop 3
int sweat3XPosInitial = 2;
int sweat3XPos;
float sweat3YPos = 2;
int sweat3YPosMax;
float sweat3Height = 2;
float sweat3Width = 1;

// Mouth settings
bool mouthEnabled = true;
int mouthType = 1;  // 0=none, 1=smile, 2=frown, 3=open, 4=small ooo, 5=flat
int mouthTypeSaved = 1;  // Saved mouth type to restore after animations
int mouthWidth = 20;
int mouthHeight = 6;

// Smooth mouth animation
float mouthOpenness = 0.0;        // 0.0 = closed/smile, 1.0 = fully open
float mouthOpennessTarget = 0.0;  // Target openness to animate toward
float mouthOpennessSpeed = 0.15;  // How fast mouth opens/closes

// Mouth animation
int mouthAnim = 0;  // 0=none, 1=talking, 2=chewing, 3=wobble, 4=laughing
unsigned long mouthAnimStart = 0;
unsigned long mouthAnimDuration = 0;
unsigned long lastMouthAnimFrame = 0;
int mouthAnimState = 0;

// Love animation (like laugh/confused)
bool love = false;
bool loveToggle = true;
unsigned long loveAnimationTimer = 0;
unsigned long loveAnimationDuration = 2000;  // 2 seconds
// Love transition animation
float loveTransition = 0.0;  // 0.0 = normal eyes, 1.0 = full hearts
float loveTransitionSpeed = 0.1;  // Speed of transition
bool loveTransitionIn = true;  // true = transitioning to hearts, false = transitioning back

// Cry animation
bool cry = false;
bool cryToggle = true;
unsigned long cryAnimationTimer = 0;
unsigned long cryAnimationDuration = 3000;  // 3 seconds
float tearLY = 0;  // Left tear Y position
float tearRY = 0;  // Right tear Y position
float tearSpeed = 1.5;  // Tear fall speed

// Knocked animation (spiral dizzy eyes)
bool knocked = false;
bool knockedToggle = true;
unsigned long knockedAnimationTimer = 0;
unsigned long knockedAnimationDuration = 3000;  // 3 seconds
float spiralAngle = 0;  // For rotating spiral effect

// Wink tracking
bool isWinking = false;
unsigned long winkStartTime = 0;
unsigned long winkDuration = 300;


//*********************************************************************************************
//  GENERAL METHODS
//*********************************************************************************************

RoboEyes(AdafruitDisplay &disp) : display(&disp) {};

// Startup RoboEyes with defined screen-width, screen-height and max. frames per second
void begin(int width, int height, byte frameRate) {
	screenWidth = width; // OLED display width, in pixels
	screenHeight = height; // OLED display height, in pixels
  display->clearDisplay(); // clear the display buffer
  display->display(); // show empty screen
  eyeLheightCurrent = 1; // start with closed eyes
  eyeRheightCurrent = 1; // start with closed eyes
  setFramerate(frameRate); // calculate frame interval based on defined frameRate
}

void update(){
  // Limit drawing updates to defined max framerate
  if(millis()-fpsTimer >= frameInterval){
    drawEyes();
    fpsTimer = millis();
  }
}


//*********************************************************************************************
//  SETTERS METHODS
//*********************************************************************************************

// Calculate frame interval based on defined frameRate
void setFramerate(byte fps){
  frameInterval = 1000/fps;
}

// Set color values
void setDisplayColors(uint8_t background, uint8_t main) {
  BGCOLOR = background; // background and overlays, choose 0 for monochrome displays and 0x00 for grayscale displays such as SSD1322
  MAINCOLOR = main; // drawings, choose 1 for monochrome displays and 0x0F for grayscale displays such as SSD1322 (0x0F = maximum brightness)
}

void setWidth(byte leftEye, byte rightEye) {
	eyeLwidthNext = leftEye;
	eyeRwidthNext = rightEye;
  eyeLwidthDefault = leftEye;
  eyeRwidthDefault = rightEye;
  eyeLwidthCurrent = leftEye;
  eyeRwidthCurrent = rightEye;
  // Recalculate centered X position
  eyeLxDefault = (screenWidth - (eyeLwidthDefault + spaceBetweenDefault + eyeRwidthDefault)) / 2;
  eyeRxDefault = eyeLxDefault + eyeLwidthDefault + spaceBetweenDefault;
  eyeLxNext = eyeLxDefault;
  eyeRxNext = eyeRxDefault;
  eyeLx = eyeLxDefault;
  eyeRx = eyeRxDefault;
}

void setHeight(byte leftEye, byte rightEye) {
	eyeLheightNext = leftEye;
	eyeRheightNext = rightEye;
  eyeLheightDefault = leftEye;
  eyeRheightDefault = rightEye;
  eyeLheightCurrent = leftEye;
  eyeRheightCurrent = rightEye;
  // Recalculate centered Y position
  eyeLyDefault = (screenHeight - eyeLheightDefault) / 2;
  eyeRyDefault = eyeLyDefault;
  eyeLyNext = eyeLyDefault;
  eyeRyNext = eyeRyDefault;
  eyeLy = eyeLyDefault;
  eyeRy = eyeRyDefault;
  // Update eyelid max heights
  eyelidsHeightMax = eyeLheightDefault / 2;
  eyelidsHappyBottomOffsetMax = (eyeLheightDefault / 2) + 3;
}

// Set border radius for left and right eye
void setBorderradius(byte leftEye, byte rightEye) {
	eyeLborderRadiusNext = leftEye;
	eyeRborderRadiusNext = rightEye;
  eyeLborderRadiusDefault = leftEye;
  eyeRborderRadiusDefault = rightEye;
}

// Set space between the eyes, can also be negative
void setSpacebetween(int space) {
  spaceBetweenNext = space;
  spaceBetweenDefault = space;
  spaceBetweenCurrent = space;
  // Recalculate centered X position
  eyeLxDefault = (screenWidth - (eyeLwidthDefault + spaceBetweenDefault + eyeRwidthDefault)) / 2;
  eyeRxDefault = eyeLxDefault + eyeLwidthDefault + spaceBetweenDefault;
  eyeLxNext = eyeLxDefault;
  eyeRxNext = eyeRxDefault;
  eyeLx = eyeLxDefault;
  eyeRx = eyeRxDefault;
}

// Set mood expression
void setMood(unsigned char mood)
  {
    switch (mood)
    {
    case TIRED:
      tired=1; 
      angry=0; 
      happy=0;
      break;
    case ANGRY:
      tired=0; 
      angry=1; 
      happy=0;
      break;
    case HAPPY:
      tired=0; 
      angry=0; 
      happy=1;
      break;
    default:
      tired=0; 
      angry=0; 
      happy=0;
      break;
    }
  }

// Set predefined position
void setPosition(unsigned char position)
  {
    switch (position)
    {
    case N:
      // North, top center
      eyeLxNext = getScreenConstraint_X()/2;
      eyeLyNext = 0;
      break;
    case NE:
      // North-east, top right
      eyeLxNext = getScreenConstraint_X();
      eyeLyNext = 0;
      break;
    case E:
      // East, middle right
      eyeLxNext = getScreenConstraint_X();
      eyeLyNext = getScreenConstraint_Y()/2;
      break;
    case SE:
      // South-east, bottom right
      eyeLxNext = getScreenConstraint_X();
      eyeLyNext = getScreenConstraint_Y();
      break;
    case S:
      // South, bottom center
      eyeLxNext = getScreenConstraint_X()/2;
      eyeLyNext = getScreenConstraint_Y();
      break;
    case SW:
      // South-west, bottom left
      eyeLxNext = 0;
      eyeLyNext = getScreenConstraint_Y();
      break;
    case W:
      // West, middle left
      eyeLxNext = 0;
      eyeLyNext = getScreenConstraint_Y()/2;
      break;
    case NW:
      // North-west, top left
      eyeLxNext = 0;
      eyeLyNext = 0;
      break;
    default:
      // Middle center
      eyeLxNext = getScreenConstraint_X()/2;
      eyeLyNext = getScreenConstraint_Y()/2;
      break;
    }
  }

// Set automated eye blinking, minimal blink interval in full seconds and blink interval variation range in full seconds
void setAutoblinker(bool active, int interval, int variation){
  autoblinker = active;
  blinkInterval = interval;
  blinkIntervalVariation = variation;
}
void setAutoblinker(bool active){
  autoblinker = active;
}

// Set idle mode - automated eye repositioning, minimal time interval in full seconds and time interval variation range in full seconds
void setIdleMode(bool active, int interval, int variation){
  idle = active;
  idleInterval = interval;
  idleIntervalVariation = variation;
}
void setIdleMode(bool active) {
  idle = active;
}

// Set curious mode - the respectively outer eye gets larger when looking left or right
void setCuriosity(bool curiousBit) {
  curious = curiousBit;
}

// Set cyclops mode - show only one eye 
void setCyclops(bool cyclopsBit) {
  cyclops = cyclopsBit;
}

// Set horizontal flickering (displacing eyes left/right)
void setHFlicker (bool flickerBit, byte Amplitude) {
  hFlicker = flickerBit; // turn flicker on or off
  hFlickerAmplitude = Amplitude; // define amplitude of flickering in pixels
}
void setHFlicker (bool flickerBit) {
  hFlicker = flickerBit; // turn flicker on or off
}

// Set vertical flickering (displacing eyes up/down)
void setVFlicker (bool flickerBit, byte Amplitude) {
  vFlicker = flickerBit; // turn flicker on or off
  vFlickerAmplitude = Amplitude; // define amplitude of flickering in pixels
}
void setVFlicker (bool flickerBit) {
  vFlicker = flickerBit; // turn flicker on or off
}

void setSweat (bool sweatBit) {
  sweat = sweatBit; // turn sweat on or off
}

// Set raised eyebrows
void setEyebrows(bool raised) {
  eyebrowsRaised = raised;
}

// Set mouth type: 0=none, 1=smile, 2=frown, 3=open, 4=small ooo, 5=flat
void setMouthType(int type) {
  mouthType = type;
  mouthAnim = 0;  // Stop any animation when setting mouth type
}

// Enable/disable mouth
void setMouthEnabled(bool enabled) {
  mouthEnabled = enabled;
}

// Set mouth dimensions
void setMouthSize(int width, int height) {
  mouthWidth = width;
  mouthHeight = height;
}

// Set animation durations
void setLaughDuration(int ms) {
  laughAnimationDuration = ms;
}

void setLoveDuration(int ms) {
  loveAnimationDuration = ms;
}

void setCryDuration(int ms) {
  cryAnimationDuration = ms;
}

void setConfusedDuration(int ms) {
  confusedAnimationDuration = ms;
}

// Get current eye dimensions (for UI feedback)
int getEyeWidth() { return eyeLwidthDefault; }
int getEyeHeight() { return eyeLheightDefault; }
int getSpaceBetween() { return spaceBetweenDefault; }
int getBorderRadius() { return eyeLborderRadiusDefault; }
int getMouthWidth() { return mouthWidth; }

// Start mouth animation: 1=talking, 2=chewing, 3=wobble
void startMouthAnim(int anim, unsigned long duration) {
  mouthAnim = anim;
  mouthAnimStart = millis();
  mouthAnimDuration = duration;
  mouthAnimState = 0;
  lastMouthAnimFrame = 0;
}

// Play love animation - heart eyes with blush for loveAnimationDuration
void anim_love() {
  love = 1;
  loveToggle = 1;  // Reset toggle so timer starts fresh
}

// Play cry animation - tears falling from eyes
void anim_cry() {
  cry = 1;
  cryToggle = 1;  // Reset toggle so timer starts fresh
  tearLY = 0;
  tearRY = 0;
}

// Set knocked expression - spiral dizzy eyes (stays until turned off)
void setKnocked(bool state) {
  if (state && !knocked) {
    knocked = 1;
    spiralAngle = 0;
    mouthTypeSaved = mouthType;
    mouthType = 4; // ooo mouth
  } else if (!state && knocked) {
    knocked = 0;
    spiralAngle = 0;
    mouthType = mouthTypeSaved;
  }
}

// Play knocked animation - spiral dizzy eyes (legacy, now just sets expression)
void anim_knocked() {
  setKnocked(true);
}

// Wink with squint on opposite eye
void wink(bool leftEye) {
  if (leftEye) {
    blink(true, false);  // Close left eye
    eyeRheightNext = eyeRheightDefault * 0.7;  // Squint right eye
  } else {
    blink(false, true);  // Close right eye
    eyeLheightNext = eyeLheightDefault * 0.7;  // Squint left eye
  }
  isWinking = true;
  winkStartTime = millis();
}


//*********************************************************************************************
//  GETTERS METHODS
//*********************************************************************************************

// Returns the max x position for left eye
int getScreenConstraint_X(){
  return screenWidth-eyeLwidthCurrent-spaceBetweenCurrent-eyeRwidthCurrent;
} 

// Returns the max y position for left eye
int getScreenConstraint_Y(){
 return screenHeight-eyeLheightDefault; // using default height here, because height will vary when blinking and in curious mode
}


//*********************************************************************************************
//  BASIC ANIMATION METHODS
//*********************************************************************************************

// BLINKING FOR BOTH EYES AT ONCE
// Close both eyes
void close() {
	eyeLheightNext = 1; // closing left eye
  eyeRheightNext = 1; // closing right eye
  eyeL_open = 0; // left eye not opened (=closed)
	eyeR_open = 0; // right eye not opened (=closed)
}

// Open both eyes
void open() {
  eyeL_open = 1; // left eye opened - if true, drawEyes() will take care of opening eyes again
	eyeR_open = 1; // right eye opened
}

// Trigger eyeblink animation
void blink() {
  close();
  open();
}

// BLINKING FOR SINGLE EYES, CONTROL EACH EYE SEPARATELY
// Close eye(s)
void close(bool left, bool right) {
  if(left){
    eyeLheightNext = 1; // blinking left eye
    eyeL_open = 0; // left eye not opened (=closed)
  }
  if(right){
      eyeRheightNext = 1; // blinking right eye
      eyeR_open = 0; // right eye not opened (=closed)
  }
}

// Open eye(s)
void open(bool left, bool right) {
  if(left){
    eyeL_open = 1; // left eye opened - if true, drawEyes() will take care of opening eyes again
  }
  if(right){
    eyeR_open = 1; // right eye opened
  }
}

// Trigger eyeblink(s) animation
void blink(bool left, bool right) {
  close(left, right);
  open(left, right);
}


//*********************************************************************************************
//  MACRO ANIMATION METHODS
//*********************************************************************************************

// Play confused animation - one shot animation of eyes shaking left and right
void anim_confused() {
	confused = 1;
}

// Play laugh animation - one shot animation of eyes shaking up and down
void anim_laugh() {
  laugh = 1;
}


//*********************************************************************************************
//  INTERNAL DRAWING HELPERS
//*********************************************************************************************

// Update smooth mouth openness
void updateMouthOpenness() {
  // Smoothly interpolate mouth openness toward target
  if (mouthOpenness < mouthOpennessTarget) {
    mouthOpenness += mouthOpennessSpeed;
    if (mouthOpenness > mouthOpennessTarget) mouthOpenness = mouthOpennessTarget;
  } else if (mouthOpenness > mouthOpennessTarget) {
    mouthOpenness -= mouthOpennessSpeed;
    if (mouthOpenness < mouthOpennessTarget) mouthOpenness = mouthOpennessTarget;
  }
}

// Update mouth animation state
void updateMouthAnim() {
  // Always update smooth mouth openness
  updateMouthOpenness();
  
  if (mouthAnim == 0) return;
  
  unsigned long now = millis();
  
  // Check if animation finished
  if (mouthAnimDuration > 0 && (now - mouthAnimStart >= mouthAnimDuration)) {
    mouthAnim = 0;
    mouthOpennessTarget = 0.0;  // Close mouth smoothly
    mouthType = 1;  // Return to smile
    return;
  }
  
  switch (mouthAnim) {
    case 1:  // Talking - alternate between open and smile
      if (now - lastMouthAnimFrame >= 100) {
        lastMouthAnimFrame = now;
        mouthAnimState = (mouthAnimState + 1) % 3;
        if (mouthAnimState == 0) mouthType = 3;       // Open
        else if (mouthAnimState == 1) mouthType = 4;  // Small
        else mouthType = 1;                           // Smile
      }
      break;
    case 2:  // Chewing - alternate between small and flat
      if (now - lastMouthAnimFrame >= 100) {
        lastMouthAnimFrame = now;
        mouthAnimState = (mouthAnimState + 1) % 2;
        if (mouthAnimState == 0) mouthType = 4;  // Small
        else mouthType = 5;                      // Flat
      }
      break;
    case 3:  // Wobble - cycle through expressions
      if (now - lastMouthAnimFrame >= 100) {
        lastMouthAnimFrame = now;
        mouthAnimState = (mouthAnimState + 1) % 4;
        if (mouthAnimState == 0) mouthType = 1;       // Smile
        else if (mouthAnimState == 1) mouthType = 5;  // Flat
        else if (mouthAnimState == 2) mouthType = 2;  // Sad
        else mouthType = 5;                           // Flat
      }
      break;
    case 4:  // Laughing - natural open/close cycle with smooth transitions
      // Use sine wave for natural "ha ha ha" rhythm - about 4 laughs per second
      {
        float phase = (now - mouthAnimStart) * 0.012;  // Speed of laugh cycle
        float wave = (sin(phase) + 1.0) / 2.0;  // 0.0 to 1.0
        mouthOpennessTarget = wave;
        mouthType = 1;  // Base is smile, openness adds the "ha"
      }
      break;
  }
}

// Draw mouth below the eyes - with smooth openness support
void drawMouth() {
  if (!mouthEnabled || mouthType == 0) return;
  
  // Calculate mouth position based on eye positions
  int eyesCenterX = (eyeLx + eyeRx + eyeRwidthCurrent) / 2;
  int mouthX = eyesCenterX - (mouthWidth / 2);
  
  // Get the bottom of the eyes
  int eyeBottomY = eyeLy + eyeLheightCurrent;
  
  // Mouth stays below the eyes with minimum gap
  int minGap = 4;
  int mouthY = eyeBottomY + minGap;
  
  // Don't draw if it would go off screen
  if (mouthY > screenHeight - 8) return;
  
  // Clamp X to screen bounds
  if (mouthX < 0) mouthX = 0;
  if (mouthX + mouthWidth > screenWidth) mouthX = screenWidth - mouthWidth;
  
  // Calculate openness effect - how much the mouth opens for laugh animation
  int openHeight = (int)(mouthOpenness * 8);  // Max 8 pixels open
  int smileDrop = 4 - (int)(mouthOpenness * 4);  // Smile flattens as mouth opens
  
  switch (mouthType) {
    case 1:  // Smile - arc curving up (thick) - with openness support for laughing
      if (mouthOpenness > 0.1) {
        // Laughing open mouth - smile that opens in the middle
        int openW = mouthWidth - 8;
        int openH = 2 + openHeight;
        int openX = mouthX + 4;
        int openY = mouthY + smileDrop - 1;
        
        // Draw the open mouth oval
        display->fillRoundRect(openX, openY, openW, openH, 3, MAINCOLOR);
        // Black inside for "open" look
        if (openH > 3) {
          display->fillRoundRect(openX + 2, openY + 2, openW - 4, openH - 3, 2, BGCOLOR);
        }
        
        // Draw smile corners going up
        for (int i = 0; i < 2; i++) {
          display->drawLine(mouthX, mouthY + i, mouthX + 4, openY + i, MAINCOLOR);
          display->drawLine(mouthX + mouthWidth - 4, openY + i, mouthX + mouthWidth, mouthY + i, MAINCOLOR);
        }
      } else {
        // Normal smile
        for (int i = 0; i < 2; i++) {
          display->drawLine(mouthX, mouthY + i, mouthX + 4, mouthY + 4 + i, MAINCOLOR);
          display->drawLine(mouthX + 4, mouthY + 4 + i, mouthX + mouthWidth - 4, mouthY + 4 + i, MAINCOLOR);
          display->drawLine(mouthX + mouthWidth - 4, mouthY + 4 + i, mouthX + mouthWidth, mouthY + i, MAINCOLOR);
        }
      }
      break;
    case 2:  // Frown - arc curving down (thick)
      for (int i = 0; i < 2; i++) {
        display->drawLine(mouthX, mouthY + 4 + i, mouthX + 4, mouthY + i, MAINCOLOR);
        display->drawLine(mouthX + 4, mouthY + i, mouthX + mouthWidth - 4, mouthY + i, MAINCOLOR);
        display->drawLine(mouthX + mouthWidth - 4, mouthY + i, mouthX + mouthWidth, mouthY + 4 + i, MAINCOLOR);
      }
      break;
    case 3:  // Open - oval/circle (thick)
      display->drawRoundRect(mouthX + 4, mouthY - 2, mouthWidth - 8, mouthHeight + 2, 3, MAINCOLOR);
      display->drawRoundRect(mouthX + 5, mouthY - 1, mouthWidth - 10, mouthHeight, 2, MAINCOLOR);
      break;
    case 4:  // Small/pursed ooo
      display->fillCircle(mouthX + mouthWidth / 2, mouthY + 2, 4, MAINCOLOR);
      break;
    case 5:  // Flat - straight line (thick)
      for (int i = 0; i < 2; i++) {
        display->drawLine(mouthX, mouthY + 2 + i, mouthX + mouthWidth, mouthY + 2 + i, MAINCOLOR);
      }
      break;
  }
}

// Draw heart using primitives at given position and size (for smooth scaling)
void drawHeartScaled(int cx, int cy, int size) {
  if (size < 4) return;  // Too small to draw
  
  // Heart made of two circles and a triangle
  int circleRadius = size / 4;
  int circleOffset = size / 4;
  
  // Top left circle
  display->fillCircle(cx - circleOffset, cy - circleOffset/2, circleRadius, MAINCOLOR);
  // Top right circle
  display->fillCircle(cx + circleOffset, cy - circleOffset/2, circleRadius, MAINCOLOR);
  // Bottom triangle pointing down
  display->fillTriangle(
    cx - size/2, cy,           // Left point
    cx + size/2, cy,           // Right point
    cx, cy + size/2 + 2,       // Bottom point
    MAINCOLOR
  );
  // Fill center rectangle to connect
  display->fillRect(cx - circleOffset, cy - circleOffset/2, circleOffset * 2, circleRadius, MAINCOLOR);
}

// Draw heart using bitmap at given position and size
void drawHeart(int x, int y, int size) {
  // Calculate scale factor and draw position
  // The bitmap is 32x32, we scale it to fit the requested size
  int drawX = x - size / 2;
  int drawY = y - size / 2;
  
  // For Adafruit GFX, we use drawBitmap with the heart_bitmap
  // Scale by drawing the 32x32 bitmap centered on x,y
  // If size matches 32, draw 1:1, otherwise we draw at native size
  // and position it centered
  int bmpSize = 32;
  drawX = x - bmpSize / 2;
  drawY = y - bmpSize / 2 + 2; // offset down a bit
  
  display->drawBitmap(drawX, drawY, heart_bitmap, HEART_BMP_WIDTH, HEART_BMP_HEIGHT, MAINCOLOR);
}

// Draw blush cheeks with fade support
void drawBlush(float intensity) {
  if (intensity < 0.3) return;  // Don't draw if too faint
  
  int blushWidth = (int)(10 * intensity);
  int blushHeight = (int)(5 * intensity);
  if (blushWidth < 3) return;
  
  int leftX = eyeLx - 5;
  int cheekY = eyeLy + eyeLheightCurrent - 5;
  display->fillRoundRect(leftX - 8, cheekY, blushWidth, blushHeight, 2, MAINCOLOR);
  
  int rightX = eyeRx + eyeRwidthCurrent + 3;
  display->fillRoundRect(rightX, cheekY, blushWidth, blushHeight, 2, MAINCOLOR);
}

// Draw blush cheeks (full intensity, legacy)
void drawBlush() {
  drawBlush(1.0);
}

// Draw heart eyes for love animation - with smooth transition
void drawLoveEyes() {
  if (!love) return;
  
  // Only draw if eyes are sufficiently open (not blinking)
  int minOpenHeight = 12;
  if (eyeLheightCurrent < minOpenHeight) return;
  if (eyeRheightCurrent < minOpenHeight) return;
  
  // Calculate heart center positions
  int leftHeartX = eyeLx + eyeLwidthCurrent / 2;
  int leftHeartY = eyeLy + eyeLheightCurrent / 2;
  int rightHeartX = eyeRx + eyeRwidthCurrent / 2;
  int rightHeartY = eyeRy + eyeRheightCurrent / 2;
  
  // During transition, scale hearts from small to full size
  // loveTransition goes from 0.0 (no heart) to 1.0 (full heart)
  
  if (loveTransition >= 1.0) {
    // Full hearts - use bitmap for crisp look
    display->fillRoundRect(eyeLx - 2, eyeLy - 2, eyeLwidthCurrent + 4, eyeLheightCurrent + 4, eyeLborderRadiusCurrent, BGCOLOR);
    display->fillRoundRect(eyeRx - 2, eyeRy - 2, eyeRwidthCurrent + 4, eyeRheightCurrent + 4, eyeRborderRadiusCurrent, BGCOLOR);
    drawHeart(leftHeartX, leftHeartY, 32);
    drawHeart(rightHeartX, rightHeartY, 32);
  } else if (loveTransition > 0.0) {
    // Transitioning - draw scaled hearts over the eyes
    // Clear eye areas
    display->fillRoundRect(eyeLx - 2, eyeLy - 2, eyeLwidthCurrent + 4, eyeLheightCurrent + 4, eyeLborderRadiusCurrent, BGCOLOR);
    display->fillRoundRect(eyeRx - 2, eyeRy - 2, eyeRwidthCurrent + 4, eyeRheightCurrent + 4, eyeRborderRadiusCurrent, BGCOLOR);
    
    // Draw eyes at reduced size during transition (eyes shrink as hearts grow)
    float eyeScale = 1.0 - (loveTransition * 0.7);  // Eyes shrink to 30% of original
    int scaledEyeW = (int)(eyeLwidthCurrent * eyeScale);
    int scaledEyeH = (int)(eyeLheightCurrent * eyeScale);
    if (scaledEyeW > 4 && scaledEyeH > 4) {
      int eyeLxOffset = (eyeLwidthCurrent - scaledEyeW) / 2;
      int eyeLyOffset = (eyeLheightCurrent - scaledEyeH) / 2;
      display->fillRoundRect(eyeLx + eyeLxOffset, eyeLy + eyeLyOffset, scaledEyeW, scaledEyeH, eyeLborderRadiusCurrent * eyeScale, MAINCOLOR);
      display->fillRoundRect(eyeRx + eyeLxOffset, eyeRy + eyeLyOffset, scaledEyeW, scaledEyeH, eyeRborderRadiusCurrent * eyeScale, MAINCOLOR);
    }
    
    // Draw growing hearts
    int heartSize = (int)(32 * loveTransition);
    if (heartSize >= 6) {
      drawHeartScaled(leftHeartX, leftHeartY, heartSize);
      drawHeartScaled(rightHeartX, rightHeartY, heartSize);
    }
  }
  
  // Draw blush cheeks with fade
  drawBlush(loveTransition);
}

// Draw tears falling from eyes for cry animation
void drawTears() {
  if (!cry) return;
  
  // Calculate tear positions - start from bottom center of each eye
  int tearLX = eyeLx + eyeLwidthCurrent / 2;
  int tearRX = eyeRx + eyeRwidthCurrent / 2;
  int tearStartY = eyeLy + eyeLheightCurrent;
  
  // Update tear positions - they fall down
  tearLY += tearSpeed;
  tearRY += tearSpeed + 0.3;  // Slightly different speed for natural look
  
  // Reset tears when they go off screen
  if (tearLY > screenHeight) tearLY = 0;
  if (tearRY > screenHeight) tearRY = 0;
  
  // Draw tear drops - teardrop shape (circle + triangle pointing up)
  int tearSize = 4;
  
  // Left tear
  int leftTearY = tearStartY + (int)tearLY;
  if (leftTearY < screenHeight - tearSize) {
    display->fillCircle(tearLX, leftTearY + tearSize, tearSize, MAINCOLOR);
    display->fillTriangle(tearLX - tearSize + 1, leftTearY + tearSize, 
                          tearLX + tearSize - 1, leftTearY + tearSize, 
                          tearLX, leftTearY, MAINCOLOR);
  }
  
  // Right tear
  int rightTearY = tearStartY + (int)tearRY;
  if (rightTearY < screenHeight - tearSize) {
    display->fillCircle(tearRX, rightTearY + tearSize, tearSize, MAINCOLOR);
    display->fillTriangle(tearRX - tearSize + 1, rightTearY + tearSize, 
                          tearRX + tearSize - 1, rightTearY + tearSize, 
                          tearRX, rightTearY, MAINCOLOR);
  }
}

// Draw a spiral at given center position
void drawSpiral(int cx, int cy, int maxRadius) {
  float angle = spiralAngle;
  float radius = 2;
  float radiusStep = 0.4;
  float angleStep = 0.3;
  
  int prevX = cx;
  int prevY = cy;
  
  while (radius < maxRadius) {
    int x = cx + (int)(cos(angle) * radius);
    int y = cy + (int)(sin(angle) * radius);
    
    // Draw line segment for thicker spiral
    display->drawLine(prevX, prevY, x, y, MAINCOLOR);
    display->drawLine(prevX+1, prevY, x+1, y, MAINCOLOR);
    
    prevX = x;
    prevY = y;
    angle += angleStep;
    radius += radiusStep;
  }
}

// Draw spiral eyes for knocked animation
void drawKnockedEyes() {
  if (!knocked) return;
  
  // Only draw spirals if eyes are sufficiently open
  int minOpenHeight = 12;
  if (eyeLheightCurrent < minOpenHeight) return;
  if (eyeRheightCurrent < minOpenHeight) return;
  
  // Update spiral rotation
  spiralAngle += 0.15;
  
  // Calculate spiral size based on eye size
  int spiralRadius = min(eyeLwidthCurrent, eyeLheightCurrent) / 2 - 2;
  if (spiralRadius < 8) spiralRadius = 8;
  
  // Black out eye areas
  display->fillRoundRect(eyeLx - 1, eyeLy - 1, eyeLwidthCurrent + 2, eyeLheightCurrent + 2, eyeLborderRadiusCurrent, BGCOLOR);
  display->fillRoundRect(eyeRx - 1, eyeRy - 1, eyeRwidthCurrent + 2, eyeRheightCurrent + 2, eyeRborderRadiusCurrent, BGCOLOR);
  
  // Draw spirals centered on eyes
  int leftCX = eyeLx + eyeLwidthCurrent / 2;
  int leftCY = eyeLy + eyeLheightCurrent / 2;
  drawSpiral(leftCX, leftCY, spiralRadius);
  
  int rightCX = eyeRx + eyeRwidthCurrent / 2;
  int rightCY = eyeRy + eyeRheightCurrent / 2;
  drawSpiral(rightCX, rightCY, spiralRadius);
}

// Handle wink reset
void updateWink() {
  if (isWinking && (millis() - winkStartTime >= winkDuration)) {
    isWinking = false;
    eyeLheightNext = eyeLheightDefault;
    eyeRheightNext = eyeRheightDefault;
  }
}


//*********************************************************************************************
//  PRE-CALCULATIONS AND ACTUAL DRAWINGS
//*********************************************************************************************

void drawEyes(){

  //// PRE-CALCULATIONS - EYE SIZES AND VALUES FOR ANIMATION TWEENINGS ////

  // Vertical size offset for larger eyes when looking left or right (curious gaze)
  if(curious){
    if(eyeLxNext<=10){eyeLheightOffset=8;}
    else if (eyeLxNext>=(getScreenConstraint_X()-10) && cyclops){eyeLheightOffset=8;}
    else{eyeLheightOffset=0;} // left eye
    if(eyeRxNext>=screenWidth-eyeRwidthCurrent-10){eyeRheightOffset=8;}
    else{eyeRheightOffset=0;} // right eye
  } else {
    eyeLheightOffset=0; // reset height offset for left eye
    eyeRheightOffset=0; // reset height offset for right eye
  }

  // Left eye height
  eyeLheightCurrent = (eyeLheightCurrent + eyeLheightNext + eyeLheightOffset)/2;
  eyeLy+= ((eyeLheightDefault-eyeLheightCurrent)/2); // vertical centering of eye when closing
  eyeLy-= eyeLheightOffset/2;
  // Right eye height
  eyeRheightCurrent = (eyeRheightCurrent + eyeRheightNext + eyeRheightOffset)/2;
  eyeRy+= (eyeRheightDefault-eyeRheightCurrent)/2; // vertical centering of eye when closing
  eyeRy-= eyeRheightOffset/2;


  // Open eyes again after closing them
	if(eyeL_open){
  	if(eyeLheightCurrent <= 1 + eyeLheightOffset){eyeLheightNext = eyeLheightDefault;} 
  }
  if(eyeR_open){
  	if(eyeRheightCurrent <= 1 + eyeRheightOffset){eyeRheightNext = eyeRheightDefault;} 
  }

  // Left eye width
  eyeLwidthCurrent = (eyeLwidthCurrent + eyeLwidthNext)/2;
  // Right eye width
  eyeRwidthCurrent = (eyeRwidthCurrent + eyeRwidthNext)/2;


  // Space between eyes
  spaceBetweenCurrent = (spaceBetweenCurrent + spaceBetweenNext)/2;

  // Left eye coordinates
  eyeLx = (eyeLx + eyeLxNext)/2;
  eyeLy = (eyeLy + eyeLyNext)/2;
  // Right eye coordinates
  eyeRxNext = eyeLxNext+eyeLwidthCurrent+spaceBetweenCurrent; // right eye's x position depends on left eyes position + the space between
  eyeRyNext = eyeLyNext; // right eye's y position should be the same as for the left eye
  eyeRx = (eyeRx + eyeRxNext)/2;
  eyeRy = (eyeRy + eyeRyNext)/2;

  // Left eye border radius
  eyeLborderRadiusCurrent = (eyeLborderRadiusCurrent + eyeLborderRadiusNext)/2;
  // Right eye border radius
  eyeRborderRadiusCurrent = (eyeRborderRadiusCurrent + eyeRborderRadiusNext)/2;
  

  //// APPLYING MACRO ANIMATIONS ////

	if(autoblinker && !knocked){
		if(millis() >= blinktimer){
		blink();
		blinktimer = millis()+(blinkInterval*1000)+(random(blinkIntervalVariation)*1000); // calculate next time for blinking
		}
	}

  // Laughing - eyes shaking up and down for the duration defined by laughAnimationDuration (default = 1000ms)
  if(laugh){
    if(laughToggle){
      setVFlicker(1, 3);  // Gentle eye shake
      laughAnimationTimer = millis();
      laughToggle = 0;
      // Save mouth and start smooth laugh animation
      mouthTypeSaved = mouthType;
      mouthType = 1;  // Smile base
      mouthAnim = 4;  // Laugh mouth animation (uses sine wave)
      mouthAnimStart = millis();
      mouthAnimDuration = 0;  // No auto-stop, we control it
      mouthOpenness = 0.0;
      mouthOpennessTarget = 0.0;
    } else if(millis() >= laughAnimationTimer+laughAnimationDuration){
      setVFlicker(0, 0);
      laughToggle = 1;
      laugh=0;
      // Smoothly close mouth when laugh ends
      mouthAnim = 0;
      mouthOpennessTarget = 0.0;
      mouthType = mouthTypeSaved;
    }
  }

  // Confused - eyes shaking left and right for the duration defined by confusedAnimationDuration (default = 500ms)
  if(confused){
    if(confusedToggle){
      setHFlicker(1, 20);
      confusedAnimationTimer = millis();
      confusedToggle = 0;
      // Save and set mouth when confused starts
      mouthTypeSaved = mouthType;
      mouthType = 6; // wavy
    } else if(millis() >= confusedAnimationTimer+confusedAnimationDuration){
      setHFlicker(0, 0);
      confusedToggle = 1;
      confused=0;
      // Restore mouth when confused ends
      mouthType = mouthTypeSaved;
    }
  }

  // Love - heart eyes animation for loveAnimationDuration (default = 2000ms)
  if(love){
    if(loveToggle){
      loveAnimationTimer = millis();
      loveToggle = 0;
      loveTransitionIn = true;  // Start transitioning to hearts
      loveTransition = 0.0;
      // Save and set mouth when love starts
      mouthTypeSaved = mouthType;
      mouthType = 1; // smile
    } else if(millis() >= loveAnimationTimer+loveAnimationDuration){
      // Start transitioning out
      loveTransitionIn = false;
    }
    
    // Handle transition animation
    if (loveTransitionIn) {
      // Transitioning to hearts
      loveTransition += loveTransitionSpeed;
      if (loveTransition > 1.0) loveTransition = 1.0;
    } else {
      // Transitioning back to normal eyes
      loveTransition -= loveTransitionSpeed;
      if (loveTransition <= 0.0) {
        loveTransition = 0.0;
        loveToggle = 1;
        love = 0;
        // Restore mouth when love ends
        mouthType = mouthTypeSaved;
      }
    }
  }

  // Cry - tears falling animation for cryAnimationDuration (default = 3000ms)
  if(cry){
    if(cryToggle){
      cryAnimationTimer = millis();
      cryToggle = 0;
      // Save and set mouth when cry starts
      mouthTypeSaved = mouthType;
      mouthType = 2; // frown
    } else if(millis() >= cryAnimationTimer+cryAnimationDuration){
      cryToggle = 1;
      cry = 0;
      tearLY = 0;
      tearRY = 0;
      // Restore mouth when cry ends
      mouthType = mouthTypeSaved;
    }
  }

  // Knocked - spiral dizzy eyes (expression, stays until turned off)
  // No timer needed - setKnocked() handles state changes

  // Idle - eyes moving to random positions on screen
  if(idle){
    if(millis() >= idleAnimationTimer){
      eyeLxNext = random(getScreenConstraint_X());
      eyeLyNext = random(getScreenConstraint_Y());
      idleAnimationTimer = millis()+(idleInterval*1000)+(random(idleIntervalVariation)*1000); // calculate next time for eyes repositioning
    }
  }

  // Adding offsets for horizontal flickering/shivering
  if(hFlicker){
    if(hFlickerAlternate) {
      eyeLx += hFlickerAmplitude;
      eyeRx += hFlickerAmplitude;
    } else {
      eyeLx -= hFlickerAmplitude;
      eyeRx -= hFlickerAmplitude;
    }
    hFlickerAlternate = !hFlickerAlternate;
  }

  // Adding offsets for horizontal flickering/shivering
  if(vFlicker){
    if(vFlickerAlternate) {
      eyeLy += vFlickerAmplitude;
      eyeRy += vFlickerAmplitude;
    } else {
      eyeLy -= vFlickerAmplitude;
      eyeRy -= vFlickerAmplitude;
    }
    vFlickerAlternate = !vFlickerAlternate;
  }

  // Cyclops mode, set second eye's size and space between to 0
  if(cyclops){
    eyeRwidthCurrent = 0;
    eyeRheightCurrent = 0;
    spaceBetweenCurrent = 0;
  }

  //// ACTUAL DRAWINGS ////

  display->clearDisplay(); // start with a blank screen

  // Draw basic eye rectangles
  display->fillRoundRect(eyeLx, eyeLy, eyeLwidthCurrent, eyeLheightCurrent, eyeLborderRadiusCurrent, MAINCOLOR); // left eye
  if (!cyclops){
    display->fillRoundRect(eyeRx, eyeRy, eyeRwidthCurrent, eyeRheightCurrent, eyeRborderRadiusCurrent, MAINCOLOR); // right eye
  }

  // Draw raised eyebrows above eyes
  if (eyebrowsRaised) {
    int browY = eyeLy - eyebrowsOffset;
    if (browY >= 0) {  // Only draw if on screen
      // Left eyebrow - slight arc upward
      display->fillRoundRect(eyeLx, browY, eyeLwidthCurrent, eyebrowsHeight, 1, MAINCOLOR);
      if (!cyclops) {
        // Right eyebrow
        display->fillRoundRect(eyeRx, browY, eyeRwidthCurrent, eyebrowsHeight, 1, MAINCOLOR);
      }
    }
  }

  // Prepare mood type transitions
  if (tired){eyelidsTiredHeightNext = eyeLheightCurrent/2; eyelidsAngryHeightNext = 0;} else{eyelidsTiredHeightNext = 0;}
  if (angry){eyelidsAngryHeightNext = eyeLheightCurrent/2; eyelidsTiredHeightNext = 0;} else{eyelidsAngryHeightNext = 0;}
  if (happy){eyelidsHappyBottomOffsetNext = eyeLheightCurrent/2;} else{eyelidsHappyBottomOffsetNext = 0;}

  // Draw tired top eyelids 
    eyelidsTiredHeight = (eyelidsTiredHeight + eyelidsTiredHeightNext)/2;
    if (!cyclops){
      display->fillTriangle(eyeLx, eyeLy-1, eyeLx+eyeLwidthCurrent, eyeLy-1, eyeLx, eyeLy+eyelidsTiredHeight-1, BGCOLOR); // left eye 
      display->fillTriangle(eyeRx, eyeRy-1, eyeRx+eyeRwidthCurrent, eyeRy-1, eyeRx+eyeRwidthCurrent, eyeRy+eyelidsTiredHeight-1, BGCOLOR); // right eye
    } else {
      // Cyclops tired eyelids
      display->fillTriangle(eyeLx, eyeLy-1, eyeLx+(eyeLwidthCurrent/2), eyeLy-1, eyeLx, eyeLy+eyelidsTiredHeight-1, BGCOLOR); // left eyelid half
      display->fillTriangle(eyeLx+(eyeLwidthCurrent/2), eyeLy-1, eyeLx+eyeLwidthCurrent, eyeLy-1, eyeLx+eyeLwidthCurrent, eyeLy+eyelidsTiredHeight-1, BGCOLOR); // right eyelid half
    }

  // Draw angry top eyelids 
    eyelidsAngryHeight = (eyelidsAngryHeight + eyelidsAngryHeightNext)/2;
    if (!cyclops){ 
      display->fillTriangle(eyeLx, eyeLy-1, eyeLx+eyeLwidthCurrent, eyeLy-1, eyeLx+eyeLwidthCurrent, eyeLy+eyelidsAngryHeight-1, BGCOLOR); // left eye
      display->fillTriangle(eyeRx, eyeRy-1, eyeRx+eyeRwidthCurrent, eyeRy-1, eyeRx, eyeRy+eyelidsAngryHeight-1, BGCOLOR); // right eye
    } else {
      // Cyclops angry eyelids
      display->fillTriangle(eyeLx, eyeLy-1, eyeLx+(eyeLwidthCurrent/2), eyeLy-1, eyeLx+(eyeLwidthCurrent/2), eyeLy+eyelidsAngryHeight-1, BGCOLOR); // left eyelid half
      display->fillTriangle(eyeLx+(eyeLwidthCurrent/2), eyeLy-1, eyeLx+eyeLwidthCurrent, eyeLy-1, eyeLx+(eyeLwidthCurrent/2), eyeLy+eyelidsAngryHeight-1, BGCOLOR); // right eyelid half
    }

  // Draw happy bottom eyelids (skip if love animation active - hearts will cover eyes)
  if (!love) {
    eyelidsHappyBottomOffset = (eyelidsHappyBottomOffset + eyelidsHappyBottomOffsetNext)/2;
    display->fillRoundRect(eyeLx-1, (eyeLy+eyeLheightCurrent)-eyelidsHappyBottomOffset+1, eyeLwidthCurrent+2, eyeLheightDefault, eyeLborderRadiusCurrent, BGCOLOR); // left eye
    if (!cyclops){ 
      display->fillRoundRect(eyeRx-1, (eyeRy+eyeRheightCurrent)-eyelidsHappyBottomOffset+1, eyeRwidthCurrent+2, eyeRheightDefault, eyeRborderRadiusCurrent, BGCOLOR); // right eye
    }
  }

  // Add sweat drops
    if (sweat){
      // Sweat drop 1 -> left corner
      if(sweat1YPos <= sweat1YPosMax){sweat1YPos+=0.5;} // vertical movement from initial to max
      else {sweat1XPosInitial = random(30); sweat1YPos = 2; sweat1YPosMax = (random(10)+10); sweat1Width = 1; sweat1Height = 2;} // if max vertical position is reached: reset all values for next drop
      if(sweat1YPos <= sweat1YPosMax/2){sweat1Width+=0.5; sweat1Height+=0.5;} // shape grows in first half of animation ...
      else {sweat1Width-=0.1; sweat1Height-=0.5;} // ... and shrinks in second half of animation
      sweat1XPos = sweat1XPosInitial-(sweat1Width/2); // keep the growing shape centered to initial x position
      display->fillRoundRect(sweat1XPos, sweat1YPos, sweat1Width, sweat1Height, sweatBorderradius, MAINCOLOR); // draw sweat drop


      // Sweat drop 2 -> center area
      if(sweat2YPos <= sweat2YPosMax){sweat2YPos+=0.5;} // vertical movement from initial to max
      else {sweat2XPosInitial = random((screenWidth-60))+30; sweat2YPos = 2; sweat2YPosMax = (random(10)+10); sweat2Width = 1; sweat2Height = 2;} // if max vertical position is reached: reset all values for next drop
      if(sweat2YPos <= sweat2YPosMax/2){sweat2Width+=0.5; sweat2Height+=0.5;} // shape grows in first half of animation ...
      else {sweat2Width-=0.1; sweat2Height-=0.5;} // ... and shrinks in second half of animation
      sweat2XPos = sweat2XPosInitial-(sweat2Width/2); // keep the growing shape centered to initial x position
      display->fillRoundRect(sweat2XPos, sweat2YPos, sweat2Width, sweat2Height, sweatBorderradius, MAINCOLOR); // draw sweat drop


      // Sweat drop 3 -> right corner
      if(sweat3YPos <= sweat3YPosMax){sweat3YPos+=0.5;} // vertical movement from initial to max
      else {sweat3XPosInitial = (screenWidth-30)+(random(30)); sweat3YPos = 2; sweat3YPosMax = (random(10)+10); sweat3Width = 1; sweat3Height = 2;} // if max vertical position is reached: reset all values for next drop
      if(sweat3YPos <= sweat3YPosMax/2){sweat3Width+=0.5; sweat3Height+=0.5;} // shape grows in first half of animation ...
      else {sweat3Width-=0.1; sweat3Height-=0.5;} // ... and shrinks in second half of animation
      sweat3XPos = sweat3XPosInitial-(sweat3Width/2); // keep the growing shape centered to initial x position
      display->fillRoundRect(sweat3XPos, sweat3YPos, sweat3Width, sweat3Height, sweatBorderradius, MAINCOLOR); // draw sweat drop
    }

  // Update and draw mouth
  updateMouthAnim();
  drawMouth();
  
  // Draw love mode overlays (hearts and blush)
  drawLoveEyes();
  
  // Draw tears for cry animation
  drawTears();
  
  // Draw spiral eyes for knocked animation
  drawKnockedEyes();
  
  // Handle wink reset
  updateWink();

  display->display(); // show drawings on display

} // end of drawEyes method


}; // end of class roboEyes


#endif
