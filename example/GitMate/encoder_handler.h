#ifndef ENCODER_HANDLER_H
#define ENCODER_HANDLER_H

#include <Arduino.h>

// Encoder event types
enum EncoderEvent {
  ENCODER_NONE = 0,
  ENCODER_CW,           // Clockwise rotation
  ENCODER_CCW,          // Counter-clockwise rotation
  ENCODER_CLICK,        // Single click
  ENCODER_DOUBLE_CLICK, // Double click
  ENCODER_LONG_PRESS    // Long press
};

// Encoder handler class
class EncoderHandler {
public:
  EncoderHandler();

  // Initialize encoder with pin numbers
  void begin(int pinA, int pinB, int pinSwitch);

  // Get the latest encoder event (non-blocking)
  EncoderEvent getEvent();

  // Check if encoder was rotated
  bool hasRotation();

  // Get rotation direction (1 = CW, -1 = CCW, 0 = none)
  int getRotationDirection();

  // Clear all pending events
  void clearEvents();

  // ISR functions (must be public for attachInterrupt)
  void IRAM_ATTR handleButtonISR();
  void IRAM_ATTR handleEncoderISR();

private:
  int _pinA;
  int _pinB;
  int _pinSwitch;

  volatile int8_t _position;
  volatile int8_t _currentA;
  volatile int8_t _lastA;
  volatile int8_t _currentB;

  volatile unsigned long _lastPressTime;
  volatile unsigned long _pressStartTime;
  volatile bool _pressFlag;
  volatile int _clickCount;
  volatile bool _longPressDetected;

  volatile EncoderEvent _lastEvent;

  unsigned long _lastDebounceTime;

  void processClicks();
};

// Global encoder instance (for ISR access)
extern EncoderHandler encoder;

#endif // ENCODER_HANDLER_H
