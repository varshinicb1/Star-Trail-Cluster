#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <lvgl.h>


// Gesture constants
#define GESTURE_NONE 0x00
#define GESTURE_SWIPE_UP 0x01
#define GESTURE_SWIPE_DOWN 0x02
#define GESTURE_SWIPE_LEFT 0x03
#define GESTURE_SWIPE_RIGHT 0x04
#define GESTURE_SINGLE_TAP 0x05
#define GESTURE_DOUBLE_TAP 0x0B
#define GESTURE_LONG_PRESS 0x0C

// Initialize display and LVGL
void display_init();

// Set backlight brightness (0-100)
void display_set_brightness(int percent);

// Get touch gesture
uint8_t touch_get_gesture();

// Get touch coordinates
bool touch_get_point(uint16_t *x, uint16_t *y);

// Encoder functions
int8_t encoder_read();
bool encoder_button_pressed();

#endif // DISPLAY_H
