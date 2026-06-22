#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

// Initialize all sensors (MPU9250 + BME280)
bool sensors_init();

// Update sensor readings (call from task at ~50Hz)
void sensors_update();

// Get processed sensor values
float sensors_get_heading();     // Degrees 0-360 (tilt-compensated, declination
                                 // applied)
float sensors_get_pitch();       // Degrees -90 to +90
float sensors_get_roll();        // Degrees -180 to +180
float sensors_get_temperature(); // Celsius
float sensors_get_altitude();    // Feet
float sensors_get_pressure();    // hPa

// Raw magnetometer values for calibration
void sensors_get_mag_raw(int16_t *x, int16_t *y, int16_t *z);

// Raw accel/gyro for debug display
void sensors_get_accel(float *x, float *y, float *z);
void sensors_get_gyro(float *x, float *y, float *z);

// Set calibration values
void sensors_set_mag_calibration(float offsetX, float offsetY, float offsetZ,
                                 float scaleX, float scaleY, float scaleZ);

// Set sea-level pressure for altitude calculation
void sensors_set_sea_level_pressure(float pressure_hPa);

// Auto-calibration: run for N seconds, save to SPIFFS
void sensors_auto_calibrate(int durationSec);
void sensors_auto_calibrate_gyro(); // 3s gyro bias at startup
bool sensors_load_calibration();
void sensors_save_calibration();

// Check if sensors were found
bool sensors_mpu_found();
bool sensors_bme_found();

#endif // SENSORS_H
