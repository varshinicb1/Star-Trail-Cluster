#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include "CST816D.h"

// Touch gesture enum
enum TouchGesture {
  TOUCH_NONE = 0,
  TOUCH_TAP,         // Single tap - show diff
  TOUCH_SWIPE_UP,    // Swipe up - stage all
  TOUCH_SWIPE_DOWN,  // Swipe down - stash
  TOUCH_SWIPE_LEFT,  // Swipe left - prev branch
  TOUCH_SWIPE_RIGHT, // Swipe right - next branch
  TOUCH_LONG_PRESS   // Long press - push
};

// Touch handler class
class TouchHandler {
public:
  TouchHandler();

  // Initialize touch controller
  void begin(CST816D *touchController);

  // Update touch state (call in loop)
  void update();

  // Get latest gesture
  TouchGesture getGesture();

  // Check if touched
  bool isTouched();

  // Get touch coordinates
  void getCoordinates(uint16_t *x, uint16_t *y);

  // Clear pending gestures
  void clearGestures();

private:
  CST816D *_touch;
  TouchGesture _lastGesture;
  bool _touched;
  uint16_t _touchX;
  uint16_t _touchY;
  unsigned long _touchStartTime;
  bool _longPressDetected;

  // Map CST816D gestures to our gesture enum
  TouchGesture mapGesture(uint8_t rawGesture);
};

// Global touch handler instance
extern TouchHandler touchHandler;

#endif // TOUCH_HANDLER_H
