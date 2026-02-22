#include "led_controller.h"
#include "config.h"

// Global LED controller instance
LEDController ledController;

LEDController::LEDController() {
  _strip = nullptr;
  _currentState = LED_CLEAN;
  _lastUpdate = 0;
  _animationStep = 0;
}

void LEDController::begin(int pin, int numLEDs, int brightness) {
  _strip = new Adafruit_NeoPixel(numLEDs, pin, NEO_GRB + NEO_KHZ800);
  _strip->begin();
  _strip->setBrightness(brightness);
  _strip->clear();
  _strip->show();

  Serial.println("LED controller initialized");
}

void LEDController::setState(LEDState state) {
  if (state != _currentState) {
    _currentState = state;
    _animationStep = 0;
    _lastUpdate = millis();

    DEBUG_PRINT_LED("LED state changed to: ");
    DEBUG_PRINTLN_LED((int)state);
  }
}

LEDState LEDController::getState() { return _currentState; }

void LEDController::update() {
  if (!_strip)
    return;

  unsigned long now = millis();

  switch (_currentState) {
  case LED_CLEAN:
    // Slow blue breathing
    animateBreathing(0, 100, 255, LED_BREATHING_PERIOD);
    break;

  case LED_UNCOMMITTED:
    // Yellow pulsing
    animatePulse(255, 200, 0, LED_PULSE_PERIOD);
    break;

  case LED_CONFLICT:
    // Fast red pulse
    animatePulse(255, 0, 0, LED_FAST_PULSE_PERIOD);
    break;

  case LED_RUNNING:
    // White flash
    animateFlash(255, 255, 255);
    break;

  case LED_SUCCESS:
    // Solid green
    setSolid(0, 255, 0);
    break;

  case LED_WARNING:
    // Solid orange
    setSolid(255, 165, 0);
    break;

  case LED_ERROR:
    // Solid red
    setSolid(255, 0, 0);
    break;

  case LED_OFFLINE:
    // Gray breathing
    animateBreathing(100, 100, 100, LED_BREATHING_PERIOD);
    break;
  }
}

void LEDController::animateBreathing(uint8_t r, uint8_t g, uint8_t b,
                                     int period) {
  unsigned long now = millis();

  if (now - _lastUpdate > 20) { // Update every 20ms
    _lastUpdate = now;

    // Calculate brightness using sine wave
    float phase = (float)(now % period) / period * 2.0 * PI;
    float brightness = (sin(phase) + 1.0) / 2.0; // 0.0 to 1.0

    uint8_t br = r * brightness;
    uint8_t bg = g * brightness;
    uint8_t bb = b * brightness;

    for (int i = 0; i < _strip->numPixels(); i++) {
      _strip->setPixelColor(i, getColor(br, bg, bb));
    }
    _strip->show();
  }
}

void LEDController::animatePulse(uint8_t r, uint8_t g, uint8_t b, int period) {
  unsigned long now = millis();

  if (now - _lastUpdate > 20) { // Update every 20ms
    _lastUpdate = now;

    // Calculate brightness using triangle wave
    int timeInPeriod = now % period;
    float brightness;

    if (timeInPeriod < period / 2) {
      // Fade in
      brightness = (float)timeInPeriod / (period / 2);
    } else {
      // Fade out
      brightness = 1.0 - (float)(timeInPeriod - period / 2) / (period / 2);
    }

    uint8_t br = r * brightness;
    uint8_t bg = g * brightness;
    uint8_t bb = b * brightness;

    for (int i = 0; i < _strip->numPixels(); i++) {
      _strip->setPixelColor(i, getColor(br, bg, bb));
    }
    _strip->show();
  }
}

void LEDController::animateFlash(uint8_t r, uint8_t g, uint8_t b) {
  unsigned long now = millis();

  if (now - _lastUpdate > 100) { // Toggle every 100ms
    _lastUpdate = now;
    _animationStep = !_animationStep;

    if (_animationStep) {
      for (int i = 0; i < _strip->numPixels(); i++) {
        _strip->setPixelColor(i, getColor(r, g, b));
      }
    } else {
      _strip->clear();
    }
    _strip->show();
  }
}

void LEDController::setSolid(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < _strip->numPixels(); i++) {
    _strip->setPixelColor(i, getColor(r, g, b));
  }
  _strip->show();
}

void LEDController::setColor(uint8_t r, uint8_t g, uint8_t b) {
  setSolid(r, g, b);
}

void LEDController::clear() {
  if (_strip) {
    _strip->clear();
    _strip->show();
  }
}

void LEDController::setBrightness(uint8_t brightness) {
  if (_strip) {
    _strip->setBrightness(brightness);
    _strip->show();
  }
}

uint32_t LEDController::getColor(uint8_t r, uint8_t g, uint8_t b) {
  return _strip->Color(r, g, b);
}
