#ifndef ATTITUDE_H
#define ATTITUDE_H

// Initialize attitude indicator widget
void attitude_init();

// Show attitude indicator widget
void attitude_show();

// Hide attitude indicator widget
void attitude_hide();

// Update with pitch and roll (degrees)
void attitude_update(float pitch, float roll);

#endif // ATTITUDE_H
