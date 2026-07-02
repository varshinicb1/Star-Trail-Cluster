#include "sensors.h"
#include "config.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <math.h>

// ============================================================================
// Sensor layer for the actual hardware:
//   MPU6050  @ 0x68  — 6-axis accel + gyro (no magnetometer)
//   QMC5883L @ 0x0D  — 3-axis magnetometer
//   BME280   @ 0x76  — pressure / temperature (altitude)
//
// Pitch/roll: complementary filter (accel absolute ref + gyro smoothness).
// Heading:    QMC5883L, hard/soft-iron corrected, tilt-compensated using the
//             fused gravity vector, + declination -> true north.
// Mount tilt: a one-time factory "orientation zero" records the 25-30 deg
//             enclosure angle so attitude reads true vehicle attitude and the
//             heading tilt-comp stays correct. End customers never calibrate.
// ============================================================================

#ifndef MPU6050_ADDR
#define MPU6050_ADDR 0x68
#endif

// ---- MPU6050 registers ----
#define MPU_PWR_MGMT_1 0x6B
#define MPU_SMPLRT_DIV 0x19
#define MPU_CONFIG 0x1A
#define MPU_GYRO_CONFIG 0x1B
#define MPU_ACCEL_CONFIG 0x1C
#define MPU_ACCEL_XOUT_H 0x3B
#define MPU_WHO_AM_I 0x75

// ---- QMC5883L registers ----
#define QMC_X_LSB 0x00
#define QMC_STATUS 0x06
#define QMC_CONFIG1 0x09
#define QMC_CONFIG2 0x0A
#define QMC_SETRESET 0x0B
#define QMC_CHIPID 0x0D

// ---- BME280 registers ----
#define BME280_CHIP_ID 0xD0
#define BME280_CTRL_MEAS 0xF4
#define BME280_CTRL_HUM 0xF2
#define BME280_CONFIG 0xF5
#define BME280_PRESS_MSB 0xF7

// Sensor full-scale: accel ±2g (16384 LSB/g), gyro ±250°/s (131 LSB/°/s)
static const float ACCEL_LSB = 16384.0f;
static const float GYRO_LSB = 131.0f;

// ============== State ==============
static bool mpu_found = false;
static bool mag_found = false;
static bool bme280_found = false;

static float accelX = 0, accelY = 0, accelZ = 1.0f;
static float gyroX = 0, gyroY = 0, gyroZ = 0;
static int16_t magRawX = 0, magRawY = 0, magRawZ = 0;

static float heading = 0, pitch = 0, roll = 0;     // published (vehicle-frame)
static float fusedPitch = 0, fusedRoll = 0;        // device-frame fused
static float temperature = 25.0f, pressure = 1013.25f,
             altitude = LOCAL_ELEVATION_FEET;

// Calibration (persisted)
static float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;
static float magOffsetX = 0, magOffsetY = 0, magOffsetZ = 0;
static float magScaleX = 1, magScaleY = 1, magScaleZ = 1;
static float mountPitch = 0, mountRoll = 0;        // factory orientation zero
static float declination = MAGNETIC_DECLINATION;
static float seaLevelPressure = DEFAULT_SEA_LEVEL_PRESSURE;

// Fusion / smoothing
static const float COMP_ALPHA = 0.98f;  // gyro weight in complementary filter
static uint32_t lastFuseMs = 0;
static float headingSmooth = 0;
static bool headingInit = false;
static float tempFiltered = 25.0f;

// BME280 compensation params
static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

// ============== I2C helpers ==============
static uint8_t readByte(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(addr, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}
static void writeByte(uint8_t addr, uint8_t reg, uint8_t data) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}
static void readBytes(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *dest) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(addr, count);
  for (uint8_t i = 0; i < count && Wire.available(); i++) dest[i] = Wire.read();
}

// ============== MPU6050 ==============
static bool initMPU6050() {
  uint8_t who = readByte(MPU6050_ADDR, MPU_WHO_AM_I);
  // MPU6050 -> 0x68, MPU9250 -> 0x71/0x73, ICM-20600 -> 0x11; accept common IMUs
  if (who == 0 || who == 0xFF) {
    Serial.printf("[MPU] Not found (WHO_AM_I=0x%02X)\n", who);
    return false;
  }
  Serial.printf("[MPU] Found (WHO_AM_I=0x%02X)\n", who);
  writeByte(MPU6050_ADDR, MPU_PWR_MGMT_1, 0x00);   // wake
  delay(10);
  writeByte(MPU6050_ADDR, MPU_PWR_MGMT_1, 0x01);   // PLL X gyro clock
  writeByte(MPU6050_ADDR, MPU_SMPLRT_DIV, 0x00);   // 1kHz
  writeByte(MPU6050_ADDR, MPU_CONFIG, 0x03);       // DLPF ~44Hz
  writeByte(MPU6050_ADDR, MPU_GYRO_CONFIG, 0x00);  // ±250°/s
  writeByte(MPU6050_ADDR, MPU_ACCEL_CONFIG, 0x00); // ±2g
  delay(10);
  return true;
}

static void readMPU6050() {
  uint8_t d[14];
  readBytes(MPU6050_ADDR, MPU_ACCEL_XOUT_H, 14, d);
  int16_t ax = (d[0] << 8) | d[1];
  int16_t ay = (d[2] << 8) | d[3];
  int16_t az = (d[4] << 8) | d[5];
  // d[6..7] = temp
  int16_t gx = (d[8] << 8) | d[9];
  int16_t gy = (d[10] << 8) | d[11];
  int16_t gz = (d[12] << 8) | d[13];

  accelX = ax / ACCEL_LSB;
  accelY = ay / ACCEL_LSB;
  accelZ = az / ACCEL_LSB;
  gyroX = gx / GYRO_LSB - gyroOffsetX;
  gyroY = gy / GYRO_LSB - gyroOffsetY;
  gyroZ = gz / GYRO_LSB - gyroOffsetZ;
}

// Complementary-filter pitch/roll from the latest accel+gyro.
static void fusePitchRoll() {
  uint32_t now = millis();
  float dt = (lastFuseMs == 0) ? 0.01f : (now - lastFuseMs) / 1000.0f;
  lastFuseMs = now;
  if (dt <= 0 || dt > 0.5f) dt = 0.01f;

  // Absolute reference from gravity (degrees)
  float pitchAcc = atan2f(-accelX, sqrtf(accelY * accelY + accelZ * accelZ)) * RAD_TO_DEG;
  float rollAcc = atan2f(accelY, accelZ) * RAD_TO_DEG;

  // Integrate gyro, blend with accel
  fusedPitch = COMP_ALPHA * (fusedPitch + gyroY * dt) + (1 - COMP_ALPHA) * pitchAcc;
  fusedRoll = COMP_ALPHA * (fusedRoll + gyroX * dt) + (1 - COMP_ALPHA) * rollAcc;

  // Remove the fixed mount tilt -> vehicle attitude
  pitch = fusedPitch - mountPitch;
  roll = fusedRoll - mountRoll;
}

// ============== QMC5883L ==============
static bool initQMC5883L() {
  uint8_t id = readByte(QMC5883L_ADDR, QMC_CHIPID);  // usually 0xFF
  writeByte(QMC5883L_ADDR, QMC_SETRESET, 0x01);      // recommended
  // OSR=512, RNG=8G, ODR=200Hz, MODE=continuous  (0x1D)
  writeByte(QMC5883L_ADDR, QMC_CONFIG1, 0x1D);
  delay(10);
  uint8_t st = readByte(QMC5883L_ADDR, QMC_STATUS);
  mag_found = (id == 0xFF) || (st != 0xFF);
  Serial.printf("[QMC5883L] %s (id=0x%02X)\n", mag_found ? "OK" : "NOT FOUND", id);
  return mag_found;
}

static void readMagnetometer() {
  uint8_t st = readByte(QMC5883L_ADDR, QMC_STATUS);
  if (!(st & 0x01)) return;  // DRDY not set
  uint8_t d[6];
  readBytes(QMC5883L_ADDR, QMC_X_LSB, 6, d);
  magRawX = (int16_t)(d[0] | (d[1] << 8));
  magRawY = (int16_t)(d[2] | (d[3] << 8));
  magRawZ = (int16_t)(d[4] | (d[5] << 8));
}

// Tilt-compensated true-north heading from mag + fused attitude.
static void computeHeading() {
  // Hard-iron + soft-iron correction
  float mx = (magRawX - magOffsetX) * magScaleX;
  float my = (magRawY - magOffsetY) * magScaleY;
  float mz = (magRawZ - magOffsetZ) * magScaleZ;

  // Tilt-compensate with vehicle pitch/roll (radians)
  float p = pitch * DEG_TO_RAD;
  float r = roll * DEG_TO_RAD;
  float Xh = mx * cosf(p) + my * sinf(r) * sinf(p) + mz * cosf(r) * sinf(p);
  float Yh = my * cosf(r) - mz * sinf(r);

  float h = atan2f(-Yh, Xh) * RAD_TO_DEG + declination;
  while (h < 0) h += 360.0f;
  while (h >= 360.0f) h -= 360.0f;

  // Wrap-aware low-pass so the scale glides without a 359->0 jump.
  if (!headingInit) { headingSmooth = h; headingInit = true; }
  float diff = h - headingSmooth;
  while (diff > 180) diff -= 360;
  while (diff < -180) diff += 360;
  headingSmooth += diff * 0.15f;
  while (headingSmooth < 0) headingSmooth += 360.0f;
  while (headingSmooth >= 360.0f) headingSmooth -= 360.0f;
  heading = headingSmooth;
}

// ============== BME280 ==============
static bool initBME280() {
  uint8_t chipId = readByte(BME280_ADDR, BME280_CHIP_ID);
  if (chipId != 0x60 && chipId != 0x58) {
    Serial.printf("[BME280] Not found (0x%02X)\n", chipId);
    return false;
  }
  Serial.printf("[BME280] Found (0x%02X)\n", chipId);
  uint8_t cal[26];
  readBytes(BME280_ADDR, 0x88, 26, cal);
  dig_T1 = (cal[1] << 8) | cal[0];
  dig_T2 = (cal[3] << 8) | cal[2];
  dig_T3 = (cal[5] << 8) | cal[4];
  dig_P1 = (cal[7] << 8) | cal[6];
  dig_P2 = (cal[9] << 8) | cal[8];
  dig_P3 = (cal[11] << 8) | cal[10];
  dig_P4 = (cal[13] << 8) | cal[12];
  dig_P5 = (cal[15] << 8) | cal[14];
  dig_P6 = (cal[17] << 8) | cal[16];
  dig_P7 = (cal[19] << 8) | cal[18];
  dig_P8 = (cal[21] << 8) | cal[20];
  dig_P9 = (cal[23] << 8) | cal[22];
  writeByte(BME280_ADDR, BME280_CTRL_HUM, 0x01);
  writeByte(BME280_ADDR, BME280_CONFIG, 0x00);
  writeByte(BME280_ADDR, BME280_CTRL_MEAS, 0x27);  // normal, x1
  return true;
}

static void readBME280() {
  uint8_t raw[6];
  readBytes(BME280_ADDR, BME280_PRESS_MSB, 6, raw);
  int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
  int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
  if (adc_T == 0 || adc_T == 0xFFFFF || adc_P == 0 || adc_P == 0xFFFFF) return;

  int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
                  ((int32_t)dig_T3)) >> 14;
  int32_t t_fine = var1 + var2;
  float rawTemp = (float)((t_fine * 5 + 128) >> 8) / 100.0f;
  if (rawTemp < -40.0f || rawTemp > 85.0f) return;
  tempFiltered = tempFiltered * 0.9f + rawTemp * 0.1f;
  temperature = tempFiltered;

  int64_t v1 = ((int64_t)t_fine) - 128000;
  int64_t v2 = v1 * v1 * (int64_t)dig_P6;
  v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
  v2 = v2 + (((int64_t)dig_P4) << 35);
  v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
  v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;
  if (v1 != 0) {
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    v2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);
    pressure = (float)p / 256.0f / 100.0f;
  }

  if (pressure > 0) {
    extern float wifi_get_sea_level_pressure();
    float weatherSLP = wifi_get_sea_level_pressure();
    static float calibratedSLP = 0;
    if (calibratedSLP == 0 && pressure > 800 && pressure < 1200)
      calibratedSLP = pressure / pow(1.0f - LOCAL_ELEVATION_METERS / 44330.0f, 5.255f);
    float slp = (weatherSLP > 900 && weatherSLP < 1100) ? weatherSLP
                : (calibratedSLP > 0) ? calibratedSLP : 1013.25f;
    float rawAltM = 44330.0f * (1.0f - pow(pressure / slp, 0.1903f));
    static float altFiltered = -9999;
    if (altFiltered < -9000) altFiltered = rawAltM;
    altFiltered = altFiltered * 0.85f + rawAltM * 0.15f;
    altitude = altFiltered * 3.28084f;  // feet
  }
}

// ============== Public API ==============
bool sensors_init() {
  mpu_found = initMPU6050();
  initQMC5883L();
  bme280_found = initBME280();

  Serial.println("========================================");
  Serial.printf("[SENSORS] MPU6050:  %s\n", mpu_found ? "OK" : "NOT FOUND");
  Serial.printf("[SENSORS] QMC5883L: %s\n", mag_found ? "OK" : "NOT FOUND");
  Serial.printf("[SENSORS] BME280:   %s\n", bme280_found ? "OK" : "NOT FOUND");
  Serial.println("========================================");
  return mpu_found || bme280_found;
}

void sensors_update() {
  if (mpu_found) {
    readMPU6050();
    fusePitchRoll();
    if (mag_found) {
      readMagnetometer();
      computeHeading();
    }
  } else {
    // Demo values if no IMU present
    static float sim = 0;
    sim += 0.5f;
    if (sim >= 360) sim = 0;
    heading = sim;
    pitch = sinf(millis() / 2000.0f) * 5.0f;
    roll = cosf(millis() / 2500.0f) * 5.0f;
  }
  if (bme280_found) readBME280();
}

float sensors_get_heading() { return heading; }
float sensors_get_pitch() { return pitch; }
float sensors_get_roll() { return roll; }
float sensors_get_temperature() { return temperature; }
float sensors_get_altitude() { return altitude; }
float sensors_get_pressure() { return pressure; }
bool sensors_mpu_found() { return mpu_found; }
bool sensors_bme_found() { return bme280_found; }
bool sensors_mag_found() { return mag_found; }

void sensors_get_mag_raw(int16_t *x, int16_t *y, int16_t *z) {
  *x = magRawX; *y = magRawY; *z = magRawZ;
}
void sensors_get_accel(float *x, float *y, float *z) {
  *x = accelX; *y = accelY; *z = accelZ;
}
void sensors_get_gyro(float *x, float *y, float *z) {
  *x = gyroX; *y = gyroY; *z = gyroZ;
}
void sensors_set_mag_calibration(float ox, float oy, float oz, float sx, float sy, float sz) {
  magOffsetX = ox; magOffsetY = oy; magOffsetZ = oz;
  magScaleX = sx; magScaleY = sy; magScaleZ = sz;
}
void sensors_set_sea_level_pressure(float hPa) { seaLevelPressure = hPa; }
void sensors_set_declination(float d) { declination = d; sensors_save_calibration(); }

// ---- Gyro bias ----
void sensors_auto_calibrate(int durationSec) {
  if (!mpu_found) return;
  Serial.println("[CAL] Gyro bias - keep still");
  float sx = 0, sy = 0, sz = 0;
  int n = 0;
  uint32_t start = millis();
  // temporarily zero offsets so we measure raw bias
  gyroOffsetX = gyroOffsetY = gyroOffsetZ = 0;
  while (millis() - start < (uint32_t)(durationSec * 1000)) {
    readMPU6050();
    sx += gyroX; sy += gyroY; sz += gyroZ;
    n++;
    delay(5);
  }
  if (n > 0) { gyroOffsetX = sx / n; gyroOffsetY = sy / n; gyroOffsetZ = sz / n; }
  Serial.printf("[CAL] Gyro bias: %.3f %.3f %.3f (%d)\n", gyroOffsetX, gyroOffsetY, gyroOffsetZ, n);
  sensors_save_calibration();
}
void sensors_auto_calibrate_gyro() { sensors_auto_calibrate(3); }

// ---- Boot mag cal (only if none saved) ----
void sensors_auto_calibrate_mag(int durationSec) {
  if (!mag_found) return;
  if (magOffsetX != 0.0f || magOffsetY != 0.0f || magOffsetZ != 0.0f) {
    Serial.println("[MAG_CAL] Existing cal found, skipping");
    return;
  }
  sensors_factory_mag_calibrate(durationSec);
}

// ---- FACTORY: orientation zero (mount tilt) ----
void sensors_factory_zero_orientation() {
  if (!mpu_found) return;
  Serial.println("[FACTORY] Orientation zero - hold still on level ground");
  float sp = 0, sr = 0;
  int n = 0;
  uint32_t start = millis();
  while (millis() - start < 1500) {
    readMPU6050();
    sp += atan2f(-accelX, sqrtf(accelY * accelY + accelZ * accelZ)) * RAD_TO_DEG;
    sr += atan2f(accelY, accelZ) * RAD_TO_DEG;
    n++;
    delay(5);
  }
  if (n > 0) { mountPitch = sp / n; mountRoll = sr / n; }
  // Reset the fused state to the new zero so it settles instantly.
  fusedPitch = mountPitch;
  fusedRoll = mountRoll;
  Serial.printf("[FACTORY] Mount offset pitch=%.2f roll=%.2f\n", mountPitch, mountRoll);
  sensors_save_calibration();
}

// ---- FACTORY: figure-8 mag cal (forced) ----
void sensors_factory_mag_calibrate(int durationSec) {
  if (!mag_found) return;
  Serial.printf("[FACTORY] Figure-8 mag cal for %ds...\n", durationSec);
  int16_t minX = 32767, maxX = -32768, minY = 32767, maxY = -32768, minZ = 32767, maxZ = -32768;
  int samples = 0;
  uint32_t start = millis();
  while (millis() - start < (uint32_t)(durationSec * 1000)) {
    readMagnetometer();
    int16_t rx = magRawX, ry = magRawY, rz = magRawZ;
    if ((rx || ry || rz) && abs(rx) < 32760 && abs(ry) < 32760 && abs(rz) < 32760) {
      if (rx < minX) minX = rx; if (rx > maxX) maxX = rx;
      if (ry < minY) minY = ry; if (ry > maxY) maxY = ry;
      if (rz < minZ) minZ = rz; if (rz > maxZ) maxZ = rz;
      samples++;
    }
    delay(15);
  }
  if (samples < 50) { Serial.printf("[FACTORY] Only %d samples\n", samples); return; }
  magOffsetX = (maxX + minX) / 2.0f;
  magOffsetY = (maxY + minY) / 2.0f;
  magOffsetZ = (maxZ + minZ) / 2.0f;
  float rX = max(1.0f, (maxX - minX) / 2.0f);
  float rY = max(1.0f, (maxY - minY) / 2.0f);
  float rZ = max(1.0f, (maxZ - minZ) / 2.0f);
  float avg = (rX + rY + rZ) / 3.0f;
  magScaleX = avg / rX; magScaleY = avg / rY; magScaleZ = avg / rZ;
  Serial.printf("[FACTORY] Mag off(%.0f,%.0f,%.0f) scale(%.2f,%.2f,%.2f) n=%d\n",
                magOffsetX, magOffsetY, magOffsetZ, magScaleX, magScaleY, magScaleZ, samples);
  sensors_save_calibration();
}

int sensors_debug_string(char *out, int maxLen) {
  return snprintf(out, maxLen,
                  "A:%.2f,%.2f,%.2f G:%.1f,%.1f,%.1f M:%d,%d,%d P:%.1f R:%.1f H:%.1f",
                  accelX, accelY, accelZ, gyroX, gyroY, gyroZ,
                  magRawX, magRawY, magRawZ, pitch, roll, heading);
}

// ============== Persistence (SPIFFS /cal.dat) ==============
void sensors_save_calibration() {
  File f = SPIFFS.open("/cal.dat", "w");
  if (!f) { Serial.println("[CAL] save failed"); return; }
  f.printf("%.6f %.6f %.6f\n", gyroOffsetX, gyroOffsetY, gyroOffsetZ);
  f.printf("%.6f %.6f %.6f\n", magOffsetX, magOffsetY, magOffsetZ);
  f.printf("%.6f %.6f %.6f\n", magScaleX, magScaleY, magScaleZ);
  f.printf("%.6f %.6f %.6f\n", mountPitch, mountRoll, declination);
  f.close();
  Serial.println("[CAL] saved");
}

bool sensors_load_calibration() {
  File f = SPIFFS.open("/cal.dat", "r");
  if (!f) {
    Serial.println("[CAL] none found; running gyro auto-cal");
    sensors_auto_calibrate(3);
    return false;
  }
  String l1 = f.readStringUntil('\n');
  String l2 = f.readStringUntil('\n');
  String l3 = f.readStringUntil('\n');
  String l4 = f.readStringUntil('\n');
  f.close();
  sscanf(l1.c_str(), "%f %f %f", &gyroOffsetX, &gyroOffsetY, &gyroOffsetZ);
  sscanf(l2.c_str(), "%f %f %f", &magOffsetX, &magOffsetY, &magOffsetZ);
  sscanf(l3.c_str(), "%f %f %f", &magScaleX, &magScaleY, &magScaleZ);
  if (l4.length() > 0) sscanf(l4.c_str(), "%f %f %f", &mountPitch, &mountRoll, &declination);
  Serial.printf("[CAL] loaded gyro(%.3f,%.3f,%.3f) mount(p%.1f,r%.1f) decl%.2f\n",
                gyroOffsetX, gyroOffsetY, gyroOffsetZ, mountPitch, mountRoll, declination);
  return true;
}
