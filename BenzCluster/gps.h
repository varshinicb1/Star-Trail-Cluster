#ifndef GPS_H
#define GPS_H

void gps_init();
void gps_update();
float gps_get_speed_kmh();
bool gps_has_fix();

#endif
