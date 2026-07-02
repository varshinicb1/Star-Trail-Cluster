#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

// Initialize all sensors (MPU6050 + QMC5883L + BME280)
bool sensors_init();

// Update sensor readings (call from task at ~100Hz)
void sensors_update();

// Get processed sensor values
float sensors_get_heading();     // Degrees 0-360 true north (tilt-compensated,
                                 // mount-corrected, declination applied)
float sensors_get_pitch();       // Degrees, vehicle pitch (mount offset removed)
float sensors_get_roll();        // Degrees, vehicle roll (mount offset removed)
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

// --- Auto / boot calibration (kept for compatibility) ---
void sensors_auto_calibrate(int durationSec);      // gyro bias, device still
void sensors_auto_calibrate_gyro();                // 3s gyro bias at startup
void sensors_auto_calibrate_mag(int durationSec);  // boot mag cal if none saved
bool sensors_load_calibration();
void sensors_save_calibration();

// --- One-time FACTORY calibration (builder does once per unit) ---
// Capture the mount tilt: call with the car on level ground and the device in
// its final 25-30 deg enclosure. Records the pitch/roll offset so attitude
// reads true vehicle attitude and heading tilt-comp is correct. Persisted.
void sensors_factory_zero_orientation();

// Run a figure-8 magnetometer calibration for `durationSec`, forcing a fresh
// hard/soft-iron solve (overwrites any existing mag cal). Persisted.
void sensors_factory_mag_calibrate(int durationSec);

// Configure magnetic declination (degrees, +E/-W) for true-north output.
void sensors_set_declination(float declinationDeg);

// Fill a compact human-readable debug line (raw + fused values) for serial/BLE
// diagnostics. Returns bytes written.
int sensors_debug_string(char *out, int maxLen);

// Check if sensors were found
bool sensors_mpu_found();
bool sensors_bme_found();
bool sensors_mag_found();

#endif // SENSORS_H
