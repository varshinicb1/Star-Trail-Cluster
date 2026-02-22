#include "encoder_handler.h"
#include "config.h"
#include <Arduino.h>

// Global encoder instance
EncoderHandler encoder;

// ISR wrapper functions
void IRAM_ATTR buttonISR() { encoder.handleButtonISR(); }

void IRAM_ATTR encoderISR() { encoder.handleEncoderISR(); }

EncoderHandler::EncoderHandler() {
  _pinA = -1;
  _pinB = -1;
  _pinSwitch = -1;
  _position = 0;
  _currentA = 0;
  _lastA = 0;
  _currentB = 0;
  _lastPressTime = 0;
  _pressStartTime = 0;
  _pressFlag = false;
  _clickCount = 0;
  _longPressDetected = false;
  _lastEvent = ENCODER_NONE;
  _lastDebounceTime = 0;
}

void EncoderHandler::begin(int pinA, int pinB, int pinSwitch) {
  _pinA = pinA;
  _pinB = pinB;
  _pinSwitch = pinSwitch;

  // Configure encoder pins
  pinMode(_pinA, INPUT);
  pinMode(_pinB, INPUT);
  pinMode(_pinSwitch, INPUT_PULLUP);

  // Read initial states
  _lastA = digitalRead(_pinA);
  _currentA = _lastA;
  _currentB = digitalRead(_pinB);

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(_pinA), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(_pinSwitch), buttonISR, CHANGE);

  Serial.println("Encoder initialized");
}

// IRAM_ATTR is already in header, removing from here to avoid conflict
void EncoderHandler::handleButtonISR() {
  unsigned long now = millis();

  // Debounce
  if (now - _lastDebounceTime < DEBOUNCE_TIME) {
    return;
  }
  _lastDebounceTime = now;

  bool buttonState = digitalRead(_pinSwitch);

  if (buttonState == LOW) { // Button pressed
    _pressFlag = true;
    _pressStartTime = now;
    _longPressDetected = false;
  } else { // Button released
    if (_pressFlag && !_longPressDetected) {
      // Valid press/release cycle
      _clickCount = _clickCount + 1;
      _lastPressTime = now;
    }
    _pressFlag = false;
  }
}

// IRAM_ATTR is already in header, removing from here to avoid conflict
void EncoderHandler::handleEncoderISR() {
  _currentA = digitalRead(_pinA);

  // Only process on rising edge of A
  if (_currentA != _lastA && _currentA == 1) {
    _currentB = digitalRead(_pinB);

    // Clockwise: A and B are different
    // Counter-clockwise: A and B are same
    if (_currentB != _currentA) {
      _position = _position + 1;
      _lastEvent = ENCODER_CW;
    } else {
      _position = _position - 1;
      _lastEvent = ENCODER_CCW;
    }
  }

  _lastA = _currentA;
}

void EncoderHandler::processClicks() {
  unsigned long now = millis();

  // Check for long press
  if (_pressFlag && !_longPressDetected) {
    if (now - _pressStartTime > LONG_PRESS_TIME) {
      _lastEvent = ENCODER_LONG_PRESS;
      _longPressDetected = true;
      _clickCount = 0; // Clear click count
      _pressFlag = false;
      DEBUG_PRINTLN_ENCODER("Long press detected");
    }
  }

  // Process click count after timeout
  if (_clickCount > 0 && now - _lastPressTime > DOUBLE_CLICK_TIME) {
    if (_clickCount == 1) {
      _lastEvent = ENCODER_CLICK;
      DEBUG_PRINTLN_ENCODER("Single click");
    } else if (_clickCount >= 2) {
      _lastEvent = ENCODER_DOUBLE_CLICK;
      DEBUG_PRINTLN_ENCODER("Double click");
    }
    _clickCount = 0;
  }
}

EncoderEvent EncoderHandler::getEvent() {
  processClicks();

  EncoderEvent event = _lastEvent;

  // Clear rotation events immediately (they're consumed)
  if (event == ENCODER_CW || event == ENCODER_CCW) {
    _lastEvent = ENCODER_NONE;
  }
  // Clear button events after they're read
  else if (event != ENCODER_NONE) {
    _lastEvent = ENCODER_NONE;
  }

  return event;
}

bool EncoderHandler::hasRotation() {
  processClicks();
  return (_lastEvent == ENCODER_CW || _lastEvent == ENCODER_CCW);
}

int EncoderHandler::getRotationDirection() {
  processClicks();

  if (_lastEvent == ENCODER_CW) {
    _lastEvent = ENCODER_NONE;
    return 1;
  } else if (_lastEvent == ENCODER_CCW) {
    _lastEvent = ENCODER_NONE;
    return -1;
  }
  return 0;
}

void EncoderHandler::clearEvents() {
  _lastEvent = ENCODER_NONE;
  _clickCount = 0;
  _position = 0;
}
