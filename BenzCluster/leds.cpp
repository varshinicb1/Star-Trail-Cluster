#include "leds.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static int currentMode = LED_MODE_OFF;
static int brightness = 50;
static uint32_t animationFrame = 0;
static uint32_t customColor = 0xFFAA00;

// Retro amber color (warm vintage look)
#define RETRO_AMBER strip.Color(255, 140, 20)
#define RETRO_WARM strip.Color(255, 180, 60)
#define RETRO_GLOW strip.Color(200, 100, 10)
#define PURE_WHITE strip.Color(255, 255, 255)
#define WIFI_BLUE strip.Color(30, 100, 255)
#define ERROR_RED strip.Color(255, 0, 0)

void leds_init() {
  strip.begin();
  strip.setBrightness(50);
  strip.clear();
  strip.show();
}

void leds_set_mode(int mode) {
  currentMode = mode;
  animationFrame = 0;
}

void leds_set_brightness(int percent) {
  brightness = constrain(percent, 0, 100);
  strip.setBrightness(map(brightness, 0, 100, 0, 255));
}

void leds_set_color(uint32_t color) {
  customColor = color;
  currentMode = LED_MODE_CUSTOM;
}

static void effectBootPulse() {
  // Warm amber pulse - fade in and out
  int phase = animationFrame % 100;
  int intensity;

  if (phase < 50) {
    intensity = map(phase, 0, 50, 0, 255);
  } else {
    intensity = map(phase, 50, 100, 255, 0);
  }

  uint32_t color = strip.Color((255 * intensity) / 255, (140 * intensity) / 255,
                               (20 * intensity) / 255);

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
}

static void effectWelcome() {
  // Elegant sequential fill with warm amber
  int fillCount = (animationFrame / 10) % (LED_COUNT + 3);

  strip.clear();
  for (int i = 0; i < min(fillCount, LED_COUNT); i++) {
    // Gradient from center outward
    int distFromCenter = abs(i - LED_COUNT / 2);
    int warmth = map(distFromCenter, 0, LED_COUNT / 2, 255, 180);
    strip.setPixelColor(i, strip.Color(255, warmth, 40));
  }
}

static void effectNormalGlow() {
  // Subtle breathing effect with retro amber
  int phase = animationFrame % 200;
  int breathIntensity;

  if (phase < 100) {
    breathIntensity = map(phase, 0, 100, 180, 255);
  } else {
    breathIntensity = map(phase, 100, 200, 255, 180);
  }

  uint32_t color = strip.Color(breathIntensity, (breathIntensity * 140) / 255,
                               (breathIntensity * 20) / 255);

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
}

static void effectCalibration() {
  // Rotating white chase
  int pos = (animationFrame / 5) % LED_COUNT;

  strip.clear();
  strip.setPixelColor(pos, PURE_WHITE);
  strip.setPixelColor((pos + 1) % LED_COUNT, strip.Color(128, 128, 128));
}

static void effectWifiConnecting() {
  // Blue breathing
  int phase = animationFrame % 60;
  int intensity;

  if (phase < 30) {
    intensity = map(phase, 0, 30, 50, 255);
  } else {
    intensity = map(phase, 30, 60, 255, 50);
  }

  uint32_t color =
      strip.Color((30 * intensity) / 255, (100 * intensity) / 255, intensity);

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
}

static void effectError() {
  // Fast red flash
  bool on = (animationFrame / 10) % 2;

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, on ? ERROR_RED : 0);
  }
}

void leds_update() {
  animationFrame++;

  switch (currentMode) {
  case LED_MODE_OFF:
    strip.clear();
    break;
  case LED_MODE_BOOT:
    effectBootPulse();
    break;
  case LED_MODE_WELCOME:
    effectWelcome();
    break;
  case LED_MODE_NORMAL:
    effectNormalGlow();
    break;
  case LED_MODE_CALIBRATION:
    effectCalibration();
    break;
  case LED_MODE_WIFI:
    effectWifiConnecting();
    break;
  case LED_MODE_ERROR:
    effectError();
    break;
  case LED_MODE_CUSTOM:
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, customColor);
    }
    break;
  }

  strip.show();
}
