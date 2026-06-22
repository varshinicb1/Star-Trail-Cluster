#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

// LED modes
#define LED_MODE_OFF 0
#define LED_MODE_BOOT 1
#define LED_MODE_WELCOME 2
#define LED_MODE_NORMAL 3
#define LED_MODE_CALIBRATION 4
#define LED_MODE_WIFI 5
#define LED_MODE_ERROR 6
#define LED_MODE_CUSTOM 7

// Initialize NeoPixel LEDs
void leds_init();

// Set LED mode
void leds_set_mode(int mode);

// Set brightness (0-100)
void leds_set_brightness(int percent);

// Set custom color (0xRRGGBB)
void leds_set_color(uint32_t color);

// Update LED animation (call from task)
void leds_update();

#endif // LEDS_H
