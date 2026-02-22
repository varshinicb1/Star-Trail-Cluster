#include "touch_handler.h"
#include "config.h"
#include <Arduino.h>

// Global touch handler instance
TouchHandler touchHandler;

TouchHandler::TouchHandler() {
  _touch = nullptr;
  _lastGesture = TOUCH_NONE;
  _touched = false;
  _touchX = 0;
  _touchY = 0;
  _touchStartTime = 0;
  _longPressDetected = false;
}

void TouchHandler::begin(CST816D *touchController) {
  _touch = touchController;
  Serial.println("Touch handler initialized");
}

void TouchHandler::update() {
  if (!_touch)
    return;

  uint8_t rawGesture;
  bool touched = _touch->getTouch(&_touchX, &_touchY, &rawGesture);

  unsigned long now = millis();

  if (touched) {
    if (!_touched) {
      // Touch just started
      _touchStartTime = now;
      _longPressDetected = false;
      _touched = true;
      DEBUG_PRINT_TOUCH("Touch at: ");
      DEBUG_PRINT_TOUCH(_touchX);
      DEBUG_PRINT_TOUCH(", ");
      DEBUG_PRINTLN_TOUCH(_touchY);
    } else {
      // Touch continuing - check for long press
      if (!_longPressDetected && (now - _touchStartTime > LONG_PRESS_TIME)) {
        _lastGesture = TOUCH_LONG_PRESS;
        _longPressDetected = true;
        DEBUG_PRINTLN_TOUCH("Long press detected");
      }
    }

    // Process gesture from CST816D
    if (rawGesture != None) {
      TouchGesture gesture = mapGesture(rawGesture);
      if (gesture != TOUCH_NONE) {
        _lastGesture = gesture;
        DEBUG_PRINT_TOUCH("Gesture detected: ");
        DEBUG_PRINTLN_TOUCH((int)gesture);
      }
    }
  } else {
    if (_touched && !_longPressDetected &&
        (now - _touchStartTime < LONG_PRESS_TIME)) {
      // Short tap detected
      _lastGesture = TOUCH_TAP;
      DEBUG_PRINTLN_TOUCH("Tap detected");
    }
    _touched = false;
  }
}

TouchGesture TouchHandler::getGesture() {
  TouchGesture gesture = _lastGesture;
  _lastGesture = TOUCH_NONE; // Clear after reading
  return gesture;
}

bool TouchHandler::isTouched() { return _touched; }

void TouchHandler::getCoordinates(uint16_t *x, uint16_t *y) {
  if (x)
    *x = _touchX;
  if (y)
    *y = _touchY;
}

void TouchHandler::clearGestures() { _lastGesture = TOUCH_NONE; }

TouchGesture TouchHandler::mapGesture(uint8_t rawGesture) {
  switch (rawGesture) {
  case SingleTap:
    return TOUCH_TAP;
  case SlideUp:
    return TOUCH_SWIPE_UP;
  case SlideDown:
    return TOUCH_SWIPE_DOWN;
  case SlideLeft:
    return TOUCH_SWIPE_LEFT;
  case SlideRight:
    return TOUCH_SWIPE_RIGHT;
  case LongPress:
    return TOUCH_LONG_PRESS;
  default:
    return TOUCH_NONE;
  }
}
