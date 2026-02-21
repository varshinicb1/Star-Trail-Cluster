#include "sensors.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// ============== MPU9250 Registers ==============
#define MPU9250_WHO_AM_I 0x75
#define MPU9250_PWR_MGMT_1 0x6B
#define MPU9250_INT_PIN_CFG 0x37
#define MPU9250_ACCEL_XOUT 0x3B
#define MPU9250_GYRO_XOUT 0x43

// AK8963 Magnetometer (inside MPU9250)
#define AK8963_ADDR 0x0C
#define AK8963_WHO_AM_I 0x00
#define AK8963_CNTL1 0x0A
#define AK8963_ST1 0x02
#define AK8963_HXL 0x03

// ============== BME280 Registers ==============
#define BME280_CHIP_ID 0xD0
#define BME280_CTRL_MEAS 0xF4
#define BME280_CTRL_HUM 0xF2
#define BME280_CONFIG 0xF5
#define BME280_PRESS_MSB 0xF7

// ============== Calibration Data ==============
static float magOffsetX = 0, magOffsetY = 0, magOffsetZ = 0;
static float magScaleX = 1, magScaleY = 1, magScaleZ = 1;
static float seaLevelPressure = DEFAULT_SEA_LEVEL_PRESSURE;

// BME280 calibration parameters
static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

// ============== Sensor Flags ==============
static bool mpu9250_found = false;
static bool bme280_found = false;
static bool qmc5883l_found = false;

// ============== Sensor Data ==============
static float accelX, accelY, accelZ;
static float gyroX, gyroY, gyroZ;
static int16_t magRawX, magRawY, magRawZ;
static float magX, magY, magZ;
static float heading = 0, pitch = 0, roll = 0;
static float temperature = 25.0f, pressure = 1013.25f,
             altitude = LOCAL_ELEVATION_FEET;

// Smoothing filters
static float headingFiltered = 0;
static float pitchFiltered = 0;
static float rollFiltered = 0;
static float tempFiltered = 25.0f;
static const float ALPHA = 0.1f; // Low-pass filter coefficient

// ============== I2C Helpers ==============
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
  for (uint8_t i = 0; i < count && Wire.available(); i++) {
    dest[i] = Wire.read();
  }
}

// ============== MPU9250 Functions ==============
static bool mag_found = false;
static uint8_t mag_addr = AK8963_ADDR; // Will be updated if found elsewhere

static bool initMPU9250() {
  // Check WHO_AM_I - accept many variants
  uint8_t whoami = readByte(MPU9250_ADDR, MPU9250_WHO_AM_I);
  Serial.printf("[IMU] WHO_AM_I at 0x68: 0x%02X\n", whoami);

  if (whoami == 0x00 || whoami == 0xFF) {
    // Try alternate address 0x69
    whoami = readByte(0x69, MPU9250_WHO_AM_I);
    Serial.printf("[IMU] WHO_AM_I at 0x69: 0x%02X\n", whoami);
  }

  if (whoami != 0x70 && whoami != 0x71 && whoami != 0x73 && whoami != 0x68 &&
      whoami != 0x75 && whoami != 0x12 && whoami != 0xAF && whoami != 0xAC) {
    Serial.printf("[IMU] Not found (WHO_AM_I: 0x%02X)\n", whoami);
    return false;
  }
  Serial.printf("[IMU] Found! (WHO_AM_I: 0x%02X)\n", whoami);

  // Full reset
  writeByte(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x80); // Reset device
  delay(100);

  // Wake up - auto select best clock
  writeByte(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x01);
  delay(100);

  // Enable I2C bypass mode (VERY aggressive - multiple attempts)
  // Method 1: Standard bypass
  writeByte(MPU9250_ADDR, 0x6A, 0x00); // Disable I2C master mode
  delay(10);
  writeByte(MPU9250_ADDR, MPU9250_INT_PIN_CFG,
            0x22); // BYPASS_EN + LATCH_INT_EN
  delay(50);

  // Now scan I2C bus to see what appeared after bypass mode
  Serial.println("[IMU] Scanning I2C after bypass enable...");
  for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("[IMU]   Post-bypass: 0x%02X found\n", addr);
    }
  }

  // Try to find magnetometer at multiple addresses
  uint8_t mag_addrs[] = {0x0C, 0x0D, 0x0E, 0x0F};
  for (int i = 0; i < 4; i++) {
    uint8_t akId = readByte(mag_addrs[i], 0x00); // WHO_AM_I register
    Serial.printf("[MAG] Check 0x%02X -> WHO_AM_I: 0x%02X\n", mag_addrs[i],
                  akId);
    if (akId == 0x48) { // AK8963
      Serial.printf("[MAG] AK8963 found at 0x%02X!\n", mag_addrs[i]);
      mag_addr = mag_addrs[i];
      mag_found = true;

      writeByte(mag_addr, AK8963_CNTL1, 0x00); // Power down
      delay(10);
      writeByte(mag_addr, AK8963_CNTL1,
                0x16); // 16-bit, 100Hz continuous mode 2
      delay(10);
      break;
    }
    // Check for QMC5883L at 0x0D
    if (mag_addrs[i] == 0x0D) {
      Wire.beginTransmission(0x0D);
      uint8_t err = Wire.endTransmission();
      if (err == 0) {
        // QMC5883L found! Init it
        Serial.println("[MAG] QMC5883L detected at 0x0D!");
        writeByte(0x0D, 0x0B, 0x01); // SET/RESET period
        writeByte(0x0D, 0x09, 0x1D); // Continuous, 200Hz, 8G, OSR512
        delay(10);
        mag_addr = 0x0D;
        mag_found = true;
        qmc5883l_found = true;
        break;
      }
    }
  }

  if (!mag_found) {
    Serial.println("[MAG] No magnetometer found on any address");
    Serial.println("[MAG] Heading will use gyro integration (drift expected)");

    // Full register dump from IMU to search for hidden magnetometer data
    Serial.println("[REG] Dumping ALL registers from 0x68:");
    Serial.print("[REG]     ");
    for (int c = 0; c < 16; c++)
      Serial.printf(" %02X", c);
    Serial.println();
    for (int row = 0; row < 8; row++) {
      Serial.printf("[REG] %02X: ", row * 16);
      for (int col = 0; col < 16; col++) {
        uint8_t reg = row * 16 + col;
        uint8_t val = readByte(MPU9250_ADDR, reg);
        Serial.printf(" %02X", val);
      }
      Serial.println();
    }
    Serial.println("[REG] Register dump complete");
  }

  return true;
}

// ============== Madgwick-style Complementary Filter ==============
static float q0 = 1.0f, q1 = 0, q2 = 0, q3 = 0; // Quaternion
static float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;
static bool gyroCalibrated = false;
static unsigned long lastFusionTime = 0;
#define BETA 0.05f // Madgwick filter gain (lower = more gyro trust)

static float invSqrt(float x) {
  float half = 0.5f * x;
  int i = *(int *)&x;
  i = 0x5f3759df - (i >> 1); // Fast inverse sqrt
  x = *(float *)&i;
  x = x * (1.5f - half * x * x);
  return x;
}

void sensors_auto_calibrate_gyro() {
  Serial.println("[CAL] Auto-calibrating gyro (keep still 3s)...");
  float sumX = 0, sumY = 0, sumZ = 0;
  int n = 0;
  unsigned long start = millis();
  while (millis() - start < 3000) {
    uint8_t rawData[14];
    readBytes(MPU9250_ADDR, MPU9250_ACCEL_XOUT, 14, rawData);
    int16_t gx = (rawData[8] << 8) | rawData[9];
    int16_t gy = (rawData[10] << 8) | rawData[11];
    int16_t gz = (rawData[12] << 8) | rawData[13];
    sumX += gx / 131.0f;
    sumY += gy / 131.0f;
    sumZ += gz / 131.0f;
    n++;
    delay(5);
  }
  gyroBiasX = sumX / n;
  gyroBiasY = sumY / n;
  gyroBiasZ = sumZ / n;
  gyroCalibrated = true;
  Serial.printf("[CAL] Gyro bias: %.3f, %.3f, %.3f (%d samples)\n", gyroBiasX,
                gyroBiasY, gyroBiasZ, n);
}

static void readMPU9250() {
  uint8_t rawData[14];
  readBytes(MPU9250_ADDR, MPU9250_ACCEL_XOUT, 14, rawData);

  // Accelerometer (±2g = 16384 LSB/g)
  int16_t ax = (rawData[0] << 8) | rawData[1];
  int16_t ay = (rawData[2] << 8) | rawData[3];
  int16_t az = (rawData[4] << 8) | rawData[5];
  accelX = ax / 16384.0f;
  accelY = ay / 16384.0f;
  accelZ = az / 16384.0f;

  // Gyroscope (±250 dps = 131 LSB/dps) — subtract bias
  int16_t gx = (rawData[8] << 8) | rawData[9];
  int16_t gy = (rawData[10] << 8) | rawData[11];
  int16_t gz = (rawData[12] << 8) | rawData[13];
  gyroX = gx / 131.0f - gyroBiasX;
  gyroY = gy / 131.0f - gyroBiasY;
  gyroZ = gz / 131.0f - gyroBiasZ;

  // Delta time
  unsigned long now = micros();
  float dt = (lastFusionTime == 0) ? 0.02f : (now - lastFusionTime) * 1e-6f;
  lastFusionTime = now;
  if (dt > 0.1f)
    dt = 0.02f; // Clamp

  // Convert gyro to rad/s
  float gxr = gyroX * DEG_TO_RAD;
  float gyr = gyroY * DEG_TO_RAD;
  float gzr = gyroZ * DEG_TO_RAD;

  // Madgwick IMU filter (6DOF — accel + gyro only)
  float recipNorm;
  float s0, s1, s2, s3;
  float qDot1, qDot2, qDot3, qDot4;
  float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2;
  float _8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

  // Rate of change of quaternion from gyroscope
  qDot1 = 0.5f * (-q1 * gxr - q2 * gyr - q3 * gzr);
  qDot2 = 0.5f * (q0 * gxr + q2 * gzr - q3 * gyr);
  qDot3 = 0.5f * (q0 * gyr - q1 * gzr + q3 * gxr);
  qDot4 = 0.5f * (q0 * gzr + q1 * gyr - q2 * gxr);

  // Compute feedback only if accel is valid (not zero)
  float aNorm = accelX * accelX + accelY * accelY + accelZ * accelZ;
  if (aNorm > 0.01f) {
    recipNorm = invSqrt(aNorm);
    float aax = accelX * recipNorm;
    float aay = accelY * recipNorm;
    float aaz = accelZ * recipNorm;

    // Auxiliary variables
    _2q0 = 2.0f * q0;
    _2q1 = 2.0f * q1;
    _2q2 = 2.0f * q2;
    _2q3 = 2.0f * q3;
    _4q0 = 4.0f * q0;
    _4q1 = 4.0f * q1;
    _4q2 = 4.0f * q2;
    _8q1 = 8.0f * q1;
    _8q2 = 8.0f * q2;
    q0q0 = q0 * q0;
    q1q1 = q1 * q1;
    q2q2 = q2 * q2;
    q3q3 = q3 * q3;

    // Gradient descent corrective step
    s0 = _4q0 * q2q2 + _2q2 * aax + _4q0 * q1q1 - _2q1 * aay;
    s1 = _4q1 * q3q3 - _2q3 * aax + 4.0f * q0q0 * q1 - _2q0 * aay - _4q1 +
         _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * aaz;
    s2 = 4.0f * q0q0 * q2 + _2q0 * aax + _4q2 * q3q3 - _2q3 * aay - _4q2 +
         _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * aaz;
    s3 = 4.0f * q1q1 * q3 - _2q1 * aax + 4.0f * q2q2 * q3 - _2q2 * aay;
    recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    // Apply feedback
    qDot1 -= BETA * s0;
    qDot2 -= BETA * s1;
    qDot3 -= BETA * s2;
    qDot4 -= BETA * s3;
  }

  // Integrate rate of change to get quaternion
  q0 += qDot1 * dt;
  q1 += qDot2 * dt;
  q2 += qDot3 * dt;
  q3 += qDot4 * dt;

  // Normalize quaternion
  recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
  q0 *= recipNorm;
  q1 *= recipNorm;
  q2 *= recipNorm;
  q3 *= recipNorm;

  // Convert quaternion to Euler angles
  pitch = asin(-2.0f * (q1 * q3 - q0 * q2)) * RAD_TO_DEG;
  roll = atan2(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) *
         RAD_TO_DEG;
  // Yaw from quaternion (used when no magnetometer)
  // heading = atan2(2.0f * (q0*q3 + q1*q2), 1.0f - 2.0f * (q2*q2 + q3*q3)) *
  // RAD_TO_DEG;
}

static void readMagnetometer() {
  if (!mag_found) {
    // Gyro-based heading integration (drifts but works without mag)
    static unsigned long lastGyroTime = 0;
    unsigned long now = millis();
    if (lastGyroTime > 0) {
      float dt = (now - lastGyroTime) / 1000.0f;
      heading += gyroZ * dt;
      if (heading < 0)
        heading += 360;
      if (heading >= 360)
        heading -= 360;
    }
    lastGyroTime = now;
    return;
  }

  if (qmc5883l_found) {
    // QMC5883L: read 6 bytes from register 0x00
    uint8_t status = readByte(0x0D, 0x06);
    if (status & 0x01) { // DRDY
      uint8_t rawData[6];
      readBytes(0x0D, 0x00, 6, rawData);
      magRawX = (int16_t)((rawData[1] << 8) | rawData[0]);
      magRawY = (int16_t)((rawData[3] << 8) | rawData[2]);
      magRawZ = (int16_t)((rawData[5] << 8) | rawData[4]);

      magX = (magRawX - magOffsetX) * magScaleX;
      magY = (magRawY - magOffsetY) * magScaleY;
      magZ = (magRawZ - magOffsetZ) * magScaleZ;

      // Tilt-compensated heading using accelerometer
      float cosRoll = cos(roll * DEG_TO_RAD);
      float sinRoll = sin(roll * DEG_TO_RAD);
      float cosPitch = cos(pitch * DEG_TO_RAD);
      float sinPitch = sin(pitch * DEG_TO_RAD);
      float Xh = magX * cosPitch + magZ * sinPitch;
      float Yh = magX * sinRoll * sinPitch + magY * cosRoll -
                 magZ * sinRoll * cosPitch;
      float rawHeading = atan2(-Yh, Xh) * RAD_TO_DEG;
      if (rawHeading < 0)
        rawHeading += 360;
      // Smooth
      float diff = rawHeading - heading;
      if (diff > 180)
        diff -= 360;
      if (diff < -180)
        diff += 360;
      heading += diff * 0.1f;
      if (heading < 0)
        heading += 360;
      if (heading >= 360)
        heading -= 360;
    }
    return;
  }

  // AK8963 path
  if (readByte(mag_addr, AK8963_ST1) & 0x01) {
    uint8_t rawData[7];
    readBytes(mag_addr, AK8963_HXL, 7, rawData);

    if (!(rawData[6] & 0x08)) {
      magRawX = (rawData[1] << 8) | rawData[0];
      magRawY = (rawData[3] << 8) | rawData[2];
      magRawZ = (rawData[5] << 8) | rawData[4];

      magX = (magRawX - magOffsetX) * magScaleX;
      magY = (magRawY - magOffsetY) * magScaleY;
      magZ = (magRawZ - magOffsetZ) * magScaleZ;

      float rawHeading = atan2(magY, magX) * RAD_TO_DEG;
      rawHeading += MAGNETIC_DECLINATION;

      // Normalize to 0-360
      if (rawHeading < 0)
        rawHeading += 360.0f;
      if (rawHeading >= 360)
        rawHeading -= 360.0f;

      // Low-pass filter with wrap-around handling
      float diff = rawHeading - headingFiltered;
      if (diff > 180)
        diff -= 360;
      if (diff < -180)
        diff += 360;
      headingFiltered += diff * ALPHA;
      if (headingFiltered < 0)
        headingFiltered += 360;
      if (headingFiltered >= 360)
        headingFiltered -= 360;
      heading = headingFiltered;
    }
  }
}

// ============== BME280 Functions ==============
static bool initBME280() {
  uint8_t chipId = readByte(BME280_ADDR, BME280_CHIP_ID);
  if (chipId != 0x60 && chipId != 0x58) { // 0x60=BME280, 0x58=BMP280
    Serial.printf("[BME280] Not found (Chip ID: 0x%02X)\n", chipId);
    return false;
  }
  Serial.printf("[BME280] Found (Chip ID: 0x%02X)\n", chipId);

  // Read calibration data
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

  // Configure: Normal mode, oversampling x1
  writeByte(BME280_ADDR, BME280_CTRL_HUM, 0x01);
  writeByte(BME280_ADDR, BME280_CONFIG, 0x00);
  writeByte(BME280_ADDR, BME280_CTRL_MEAS, 0x27);

  return true;
}

static void readBME280() {
  uint8_t rawData[6];
  readBytes(BME280_ADDR, BME280_PRESS_MSB, 6, rawData);

  int32_t adc_P = ((int32_t)rawData[0] << 12) | ((int32_t)rawData[1] << 4) |
                  (rawData[2] >> 4);
  int32_t adc_T = ((int32_t)rawData[3] << 12) | ((int32_t)rawData[4] << 4) |
                  (rawData[5] >> 4);

  // Temperature compensation
  int32_t var1 =
      ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
                    ((adc_T >> 4) - ((int32_t)dig_T1))) >>
                   12) *
                  ((int32_t)dig_T3)) >>
                 14;
  int32_t t_fine = var1 + var2;
  float rawTemp = (t_fine * 5 + 128) >> 8;
  rawTemp /= 100.0f;

  // Low-pass filter for temperature
  tempFiltered = tempFiltered * 0.9f + rawTemp * 0.1f;
  temperature = tempFiltered;

  // Pressure compensation
  int64_t var1_p = ((int64_t)t_fine) - 128000;
  int64_t var2_p = var1_p * var1_p * (int64_t)dig_P6;
  var2_p = var2_p + ((var1_p * (int64_t)dig_P5) << 17);
  var2_p = var2_p + (((int64_t)dig_P4) << 35);
  var1_p = ((var1_p * var1_p * (int64_t)dig_P3) >> 8) +
           ((var1_p * (int64_t)dig_P2) << 12);
  var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)dig_P1) >> 33;

  if (var1_p != 0) {
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2_p) * 3125) / var1_p;
    var1_p = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2_p = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1_p + var2_p) >> 8) + (((int64_t)dig_P7) << 4);
    pressure = (float)p / 256.0f / 100.0f;
  }

  // Calculate altitude in feet
  altitude = 44330.0f * (1.0f - pow(pressure / seaLevelPressure, 0.1903f));
  altitude *= 3.28084f;
}

// ============== I2C Bus Scanner ==============
static void scanI2CBus() {
  // Scan Wire (external sensor bus: SDA=38, SCL=39)
  Serial.println("[I2C] Scanning Wire bus (SDA=38, SCL=39)...");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("[I2C]   Wire: Device at 0x%02X", addr);
      if (addr == 0x68)
        Serial.print(" (MPU9250/MPU6050/ICM-20600)");
      else if (addr == 0x69)
        Serial.print(" (MPU9250 alt addr)");
      else if (addr == 0x0C)
        Serial.print(" (AK8963 Magnetometer)");
      else if (addr == 0x0D)
        Serial.print(" (QMC5883L Magnetometer)");
      else if (addr == 0x76)
        Serial.print(" (BME280/BMP280)");
      else if (addr == 0x77)
        Serial.print(" (BME280/BMP280 alt)");
      Serial.println();
      found++;
    }
  }
  Serial.printf("[I2C] Wire scan: %d device(s)\n", found);

  // Scan Wire1 (touch bus: SDA=6, SCL=7) - BME280 might be here
  Serial.println("[I2C] Scanning Wire1 bus (SDA=6, SCL=7)...");
  int found1 = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire1.beginTransmission(addr);
    uint8_t err = Wire1.endTransmission();
    if (err == 0) {
      Serial.printf("[I2C]   Wire1: Device at 0x%02X", addr);
      if (addr == 0x15)
        Serial.print(" (CST816D Touch)");
      else if (addr == 0x76)
        Serial.print(" (BME280/BMP280)");
      else if (addr == 0x77)
        Serial.print(" (BME280/BMP280 alt)");
      else if (addr == 0x68)
        Serial.print(" (MPU9250/ICM-20600)");
      Serial.println();
      found1++;
    }
  }
  Serial.printf("[I2C] Wire1 scan: %d device(s)\n", found1);
}

// ============== Public Functions ==============
bool sensors_init() {
  // Scan I2C bus first for diagnostics
  scanI2CBus();

  mpu9250_found = initMPU9250();

  // Try BME280 on primary address 0x76
  bme280_found = initBME280();

  // If not found at 0x76, try alternate address 0x77
  if (!bme280_found) {
    Serial.println("[BME280] Trying alternate address 0x77...");
    // Temporarily override the address define is tricky, so just do inline
    // check
    uint8_t chipId77 = readByte(0x77, BME280_CHIP_ID);
    Serial.printf("[BME280] 0x77 Chip ID: 0x%02X\n", chipId77);
    if (chipId77 == 0x60 || chipId77 == 0x58) {
      Serial.println("[BME280] Found at 0x77! (Not supported yet - update "
                     "BME280_ADDR in config.h)");
    }
  }

  Serial.println("========================================");
  Serial.printf("[SENSORS] MPU9250: %s\n", mpu9250_found ? "OK" : "NOT FOUND");
  Serial.printf("[SENSORS] BME280:  %s\n", bme280_found ? "OK" : "NOT FOUND");
  Serial.println("========================================");

  if (!mpu9250_found && !bme280_found) {
    Serial.println("[SENSORS] No sensors found - using simulated values");
  }

  return mpu9250_found || bme280_found;
}

void sensors_update() {
  if (mpu9250_found) {
    readMPU9250();
    readMagnetometer();
  } else {
    // Simulated demo values
    static float simHeading = 0;
    simHeading += 0.5f;
    if (simHeading >= 360)
      simHeading = 0;
    heading = simHeading;
    pitch = sin(millis() / 2000.0f) * 5.0f;
    roll = cos(millis() / 2500.0f) * 5.0f;
  }

  if (bme280_found) {
    readBME280();
  }
}

float sensors_get_heading() { return heading; }
float sensors_get_pitch() { return pitch; }
float sensors_get_roll() { return roll; }
float sensors_get_temperature() { return temperature; }
float sensors_get_altitude() { return altitude; }
float sensors_get_pressure() { return pressure; }
bool sensors_mpu_found() { return mpu9250_found; }
bool sensors_bme_found() { return bme280_found; }

void sensors_get_mag_raw(int16_t *x, int16_t *y, int16_t *z) {
  *x = magRawX;
  *y = magRawY;
  *z = magRawZ;
}

void sensors_get_accel(float *x, float *y, float *z) {
  *x = accelX;
  *y = accelY;
  *z = accelZ;
}

void sensors_get_gyro(float *x, float *y, float *z) {
  *x = gyroX;
  *y = gyroY;
  *z = gyroZ;
}

void sensors_set_mag_calibration(float offsetX, float offsetY, float offsetZ,
                                 float scaleX, float scaleY, float scaleZ) {
  magOffsetX = offsetX;
  magOffsetY = offsetY;
  magOffsetZ = offsetZ;
  magScaleX = scaleX;
  magScaleY = scaleY;
  magScaleZ = scaleZ;
}

void sensors_set_sea_level_pressure(float pressure_hPa) {
  seaLevelPressure = pressure_hPa;
}

// ============== Auto-Calibration ==============
// Gyro offset calibration: keep device still for N seconds
static float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;

void sensors_auto_calibrate(int durationSec) {
  if (!mpu9250_found) {
    Serial.println("[CAL] No IMU found, skipping calibration");
    return;
  }

  Serial.println("[CAL] Auto-calibrating gyro - KEEP DEVICE STILL!");
  float sumGx = 0, sumGy = 0, sumGz = 0;
  int count = 0;
  unsigned long start = millis();

  while (millis() - start < (unsigned long)(durationSec * 1000)) {
    readMPU9250();
    sumGx += gyroX;
    sumGy += gyroY;
    sumGz += gyroZ;
    count++;
    delay(10);
  }

  gyroOffsetX = sumGx / count;
  gyroOffsetY = sumGy / count;
  gyroOffsetZ = sumGz / count;

  Serial.printf("[CAL] Gyro offsets: X=%.3f Y=%.3f Z=%.3f (from %d samples)\n",
                gyroOffsetX, gyroOffsetY, gyroOffsetZ, count);

  // Save to SPIFFS
  sensors_save_calibration();
}

#include <SPIFFS.h>

void sensors_save_calibration() {
  File f = SPIFFS.open("/cal.dat", "w");
  if (!f) {
    Serial.println("[CAL] Failed to save calibration");
    return;
  }
  f.printf("%.6f %.6f %.6f\n", gyroOffsetX, gyroOffsetY, gyroOffsetZ);
  f.printf("%.6f %.6f %.6f\n", magOffsetX, magOffsetY, magOffsetZ);
  f.printf("%.6f %.6f %.6f\n", magScaleX, magScaleY, magScaleZ);
  f.close();
  Serial.println("[CAL] Calibration saved to SPIFFS");
}

bool sensors_load_calibration() {
  File f = SPIFFS.open("/cal.dat", "r");
  if (!f) {
    Serial.println("[CAL] No calibration file found, running auto-cal...");
    sensors_auto_calibrate(3); // 3 second gyro cal on first boot
    return false;
  }
  // Read line by line using Arduino String
  String line1 = f.readStringUntil('\n');
  String line2 = f.readStringUntil('\n');
  String line3 = f.readStringUntil('\n');
  f.close();

  sscanf(line1.c_str(), "%f %f %f", &gyroOffsetX, &gyroOffsetY, &gyroOffsetZ);
  sscanf(line2.c_str(), "%f %f %f", &magOffsetX, &magOffsetY, &magOffsetZ);
  sscanf(line3.c_str(), "%f %f %f", &magScaleX, &magScaleY, &magScaleZ);

  Serial.printf("[CAL] Loaded: gyro=%.3f,%.3f,%.3f mag_off=%.1f,%.1f,%.1f\n",
                gyroOffsetX, gyroOffsetY, gyroOffsetZ, magOffsetX, magOffsetY,
                magOffsetZ);
  return true;
}
