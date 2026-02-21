#ifndef SENSORVIEW_H
#define SENSORVIEW_H

#include <stdint.h>

void sensorview_init();
void sensorview_show();
void sensorview_hide();
void sensorview_update(float heading, float pitch, float roll, float temp,
                       float alt, float pressure, int16_t mx, int16_t my,
                       int16_t mz, float accelX, float accelY, float accelZ,
                       float gyroX, float gyroY, float gyroZ);

#endif // SENSORVIEW_H
