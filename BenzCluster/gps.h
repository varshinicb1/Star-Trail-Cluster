#ifndef GPS_H
#define GPS_H

void gps_init();
void gps_update();

float gps_get_speed_kmh(); // Speed in km/h
float gps_get_course();    // True heading from GPS (degrees, 0-360)
float gps_get_latitude();  // Decimal degrees
float gps_get_longitude(); // Decimal degrees
bool gps_has_fix();        // Valid GPS fix
bool gps_has_course();     // Course heading is reliable (moving > 2 km/h)

#endif
