#ifndef ATTITUDE_H
#define ATTITUDE_H

#include <stdint.h>

// Selectable attitude-indicator styles (user picks in the companion app,
// like a watch face). All render from the same pitch/roll inputs.
enum AttitudeStyle : uint8_t {
  ATT_STYLE_CLASSIC = 0,   // ICAO round: sky/ground, pitch ladder, bank arc
  ATT_STYLE_FULLSCREEN,    // immersive sky/ground horizon, no bezel/arc
  ATT_STYLE_MINIMAL,       // black bg, white line horizon + ladder + arc
  ATT_STYLE_TAPE,          // EFIS: vertical pitch tape + roll scale on top
  ATT_STYLE_COUNT
};

// Initialize attitude indicator widget
void attitude_init();

// Show attitude indicator widget
void attitude_show();

// Hide attitude indicator widget
void attitude_hide();

// Update with pitch and roll (degrees)
void attitude_update(float pitch, float roll);

// Style selection (persistence is handled by the caller/firmware layer —
// this module keeps it in RAM so it also builds in the PC simulator).
void attitude_set_style(uint8_t style);
uint8_t attitude_get_style();

#endif // ATTITUDE_H
