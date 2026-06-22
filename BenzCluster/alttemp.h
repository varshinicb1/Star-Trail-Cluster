#ifndef ALTTEMP_H
#define ALTTEMP_H

// Initialize altitude/temperature widget
void alttemp_init();

// Show altitude/temperature widget
void alttemp_show();

// Hide altitude/temperature widget
void alttemp_hide();

// Update with temperature (Celsius) and altitude (feet)
void alttemp_update(float temperature, float altitude);

#endif // ALTTEMP_H
