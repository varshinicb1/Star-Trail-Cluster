#include "calibration.h"
#include "config.h"
#include "sensors.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Calibration state
static bool calibrating = false;
static uint32_t calibrationStart = 0;
static const uint32_t CALIBRATION_DURATION = 30000; // 30 seconds

// Min/max values during calibration
static int16_t magMinX = 32767, magMaxX = -32768;
static int16_t magMinY = 32767, magMaxY = -32768;
static int16_t magMinZ = 32767, magMaxZ = -32768;

bool calibration_load() {
  if (!SPIFFS.exists(CALIBRATION_FILE)) {
    Serial.println("[CAL] No calibration file found");
    return false;
  }

  File file = SPIFFS.open(CALIBRATION_FILE, "r");
  if (!file) {
    Serial.println("[CAL] Failed to open calibration file");
    return false;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("[CAL] Failed to parse calibration file");
    return false;
  }

  float offsetX = doc["offsetX"] | 0.0f;
  float offsetY = doc["offsetY"] | 0.0f;
  float offsetZ = doc["offsetZ"] | 0.0f;
  float scaleX = doc["scaleX"] | 1.0f;
  float scaleY = doc["scaleY"] | 1.0f;
  float scaleZ = doc["scaleZ"] | 1.0f;

  sensors_set_mag_calibration(offsetX, offsetY, offsetZ, scaleX, scaleY,
                              scaleZ);

  Serial.printf(
      "[CAL] Loaded: offset(%.1f, %.1f, %.1f) scale(%.3f, %.3f, %.3f)\n",
      offsetX, offsetY, offsetZ, scaleX, scaleY, scaleZ);

  return true;
}

bool calibration_save() {
  // Calculate offsets (hard-iron)
  float offsetX = (magMaxX + magMinX) / 2.0f;
  float offsetY = (magMaxY + magMinY) / 2.0f;
  float offsetZ = (magMaxZ + magMinZ) / 2.0f;

  // Calculate scales (soft-iron) — guard against zero range (saturated axis)
  float rangeX = max(1.0f, (magMaxX - magMinX) / 2.0f);
  float rangeY = max(1.0f, (magMaxY - magMinY) / 2.0f);
  float rangeZ = max(1.0f, (magMaxZ - magMinZ) / 2.0f);
  float avgRange = (rangeX + rangeY + rangeZ) / 3.0f;

  float scaleX = avgRange / rangeX;
  float scaleY = avgRange / rangeY;
  float scaleZ = avgRange / rangeZ;

  // Create directory if needed
  if (!SPIFFS.exists("/calibration")) {
    SPIFFS.mkdir("/calibration");
  }

  File file = SPIFFS.open(CALIBRATION_FILE, "w");
  if (!file) {
    Serial.println("[CAL] Failed to create calibration file");
    return false;
  }

  StaticJsonDocument<256> doc;
  doc["offsetX"] = offsetX;
  doc["offsetY"] = offsetY;
  doc["offsetZ"] = offsetZ;
  doc["scaleX"] = scaleX;
  doc["scaleY"] = scaleY;
  doc["scaleZ"] = scaleZ;
  doc["timestamp"] = millis();

  serializeJson(doc, file);
  file.close();

  // Apply calibration
  sensors_set_mag_calibration(offsetX, offsetY, offsetZ, scaleX, scaleY,
                              scaleZ);

  Serial.printf(
      "[CAL] Saved: offset(%.1f, %.1f, %.1f) scale(%.3f, %.3f, %.3f)\n",
      offsetX, offsetY, offsetZ, scaleX, scaleY, scaleZ);

  return true;
}

void calibration_start() {
  calibrating = true;
  calibrationStart = millis();

  // Reset min/max
  magMinX = magMinY = magMinZ = 32767;
  magMaxX = magMaxY = magMaxZ = -32768;

  Serial.println("[CAL] Started - rotate device slowly in all directions");
}

void calibration_update() {
  if (!calibrating)
    return;

  // Get raw magnetometer values
  int16_t mx, my, mz;
  sensors_get_mag_raw(&mx, &my, &mz);

  // Update min/max
  if (mx < magMinX)
    magMinX = mx;
  if (mx > magMaxX)
    magMaxX = mx;
  if (my < magMinY)
    magMinY = my;
  if (my > magMaxY)
    magMaxY = my;
  if (mz < magMinZ)
    magMinZ = mz;
  if (mz > magMaxZ)
    magMaxZ = mz;

  // Check if complete
  if (millis() - calibrationStart >= CALIBRATION_DURATION) {
    calibrating = false;
    calibration_save();
    Serial.println("[CAL] Complete!");
  }
}

bool calibration_is_complete() {
  return !calibrating && (millis() - calibrationStart >= CALIBRATION_DURATION);
}

int calibration_get_progress() {
  if (!calibrating)
    return 100;

  uint32_t elapsed = millis() - calibrationStart;
  return min(100, (int)(elapsed * 100 / CALIBRATION_DURATION));
}
