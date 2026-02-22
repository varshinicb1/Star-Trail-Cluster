#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Adafruit_NeoPixel.h>

// LED state enum
enum LEDState {
  LED_CLEAN,       // Clean repo - slow blue breathing
  LED_UNCOMMITTED, // Uncommitted changes - yellow pulsing
  LED_CONFLICT,    // Merge conflict - fast red pulse
  LED_RUNNING,     // Command running - white flash
  LED_SUCCESS,     // Success - solid green
  LED_WARNING,     // Warning - solid orange
  LED_ERROR,       // Error - solid red
  LED_OFFLINE      // Connection lost - gray breathing
};

// LED controller class
class LEDController {
public:
  LEDController();

  // Initialize LED strip
  void begin(int pin, int numLEDs, int brightness);

  // Set LED state (will animate automatically)
  void setState(LEDState state);

  // Get current state
  LEDState getState();

  // Update animation (call in loop)
  void update();

  // Set custom color for all LEDs
  void setColor(uint8_t r, uint8_t g, uint8_t b);

  // Clear all LEDs
  void clear();

  // Set brightness (0-255)
  void setBrightness(uint8_t brightness);

private:
  Adafruit_NeoPixel *_strip;
  LEDState _currentState;
  unsigned long _lastUpdate;
  int _animationStep;

  // Animation functions
  void animateBreathing(uint8_t r, uint8_t g, uint8_t b, int period);
  void animatePulse(uint8_t r, uint8_t g, uint8_t b, int period);
  void animateFlash(uint8_t r, uint8_t g, uint8_t b);
  void setSolid(uint8_t r, uint8_t g, uint8_t b);

  // Color conversion
  uint32_t getColor(uint8_t r, uint8_t g, uint8_t b);
};

// Global LED controller instance
extern LEDController ledController;

#endif // LED_CONTROLLER_H
