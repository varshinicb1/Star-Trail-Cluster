#ifndef CALIBRATION_H
#define CALIBRATION_H

// Load calibration from SPIFFS
bool calibration_load();

// Save calibration to SPIFFS
bool calibration_save();

// Start calibration mode (collect samples)
void calibration_start();

// Update calibration (call during calibration mode)
void calibration_update();

// Check if calibration is complete
bool calibration_is_complete();

// Get calibration progress (0-100%)
int calibration_get_progress();

#endif // CALIBRATION_H
