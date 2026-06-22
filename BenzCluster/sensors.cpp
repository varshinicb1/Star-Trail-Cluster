#include "sensors.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// ============== MPU9250 Registers ==============
#define MPU9250_WHO_AM_I 0x75
#define MPU9250_PWR_MGMT_1 0x6B
#define MPU9250_PWR_MGMT_2 0x6C
#define MPU9250_INT_PIN_CFG 0x37
#define MPU9250_ACCEL_XOUT 0x3B
#define MPU9250_GYRO_XOUT 0x43
#define MPU9250_USER_CTRL 0x6A
#define MPU9250_I2C_MST_CTRL 0x24
#define MPU9250_I2C_SLV0_ADDR 0x25
#define MPU9250_I2C_SLV0_REG 0x26
#define MPU9250_I2C_SLV0_CTRL 0x27
#define MPU9250_I2C_SLV4_ADDR 0x31
#define MPU9250_I2C_SLV4_REG 0x32
#define MPU9250_I2C_SLV4_DO 0x33
#define MPU9250_I2C_SLV4_CTRL 0x34
#define MPU9250_I2C_SLV4_DI 0x35
#define MPU9250_I2C_MST_STATUS 0x36
#define MPU9250_EXT_SENS_DATA_00 0x49

// AK8963 Magnetometer (inside MPU9250)
#define AK8963_ADDR 0x0C
#define AK8963_WIA 0x00
#define AK8963_ST1 0x02
#define AK8963_HXL 0x03
#define AK8963_ST2 0x09
#define AK8963_CNTL1 0x0A
#define AK8963_CNTL2 0x0B
#define AK8963_ASAX 0x10

// AK8963 sensitivity adjustment values (from fuse ROM)
static float magASA[3] = {1.0f, 1.0f, 1.0f};

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
static bool ak8963_found = false;

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

// Auto-calibration offsets (computed from first 100 samples at boot)
static float pitchOffset = 0, rollOffset = 0;
static float pitchAccum = 0, rollAccum = 0;
static int calCount = 0;
static bool calDone = false;
#define CAL_SAMPLES 100
static float tempFiltered = 25.0f;
static const float ALPHA =
    0.2f; // Low-pass filter coefficient (faster response)

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

// ============== I2C Master Helpers ==============
// Write a single byte to an external I2C slave via MPU9250 SLV4 (single-shot)
static bool mpu_slv4_write(uint8_t slaveAddr, uint8_t reg, uint8_t data) {
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_ADDR, slaveAddr); // Write mode
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_REG, reg);
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_DO, data);
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_CTRL, 0x80); // Enable SLV4
  delay(20);
  uint8_t status = readByte(MPU9250_ADDR, MPU9250_I2C_MST_STATUS);
  if (status & 0x10) { // SLV4_NACK
    Serial.printf("[I2C_MST] SLV4 NACK writing 0x%02X=0x%02X to 0x%02X\n", reg,
                  data, slaveAddr);
    return false;
  }
  Serial.printf("[I2C_MST] SLV4 wrote 0x%02X=0x%02X to 0x%02X OK\n", reg, data,
                slaveAddr);
  return true;
}

// Read a single byte from an external I2C slave via MPU9250 SLV4 (single-shot)
static uint8_t mpu_slv4_read(uint8_t slaveAddr, uint8_t reg) {
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_ADDR, slaveAddr | 0x80); // Read mode
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_REG, reg);
  writeByte(MPU9250_ADDR, MPU9250_I2C_SLV4_CTRL, 0x80); // Enable SLV4
  delay(20);
  uint8_t status = readByte(MPU9250_ADDR, MPU9250_I2C_MST_STATUS);
  if (status & 0x10) { // SLV4_NACK
    return 0x00;
  }
  return readByte(MPU9250_ADDR, MPU9250_I2C_SLV4_DI);
}

static bool initMPU9250() {
  // Check WHO_AM_I - accept many variants
  uint8_t whoami = readByte(MPU9250_ADDR, MPU9250_WHO_AM_I);
  Serial.printf("[IMU] WHO_AM_I at 0x68: 0x%02X\n", whoami);

  if (whoami == 0x00 || whoami == 0xFF) {
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
  writeByte(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x80);
  delay(100);

  // Wake up - auto select best clock
  writeByte(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x01);
  delay(100);

  // Configure gyro: ±250dps, DLPF 41Hz
  writeByte(MPU9250_ADDR, 0x1B, 0x00); // GYRO_CONFIG: ±250dps
  writeByte(MPU9250_ADDR, 0x1A, 0x03); // CONFIG: DLPF_CFG=3 (41Hz)

  // Configure accel: ±2g, DLPF 41Hz
  writeByte(MPU9250_ADDR, 0x1C, 0x00); // ACCEL_CONFIG: ±2g
  writeByte(MPU9250_ADDR, 0x1D, 0x03); // ACCEL_CONFIG2: DLPF 41Hz

  // Enable I2C bypass mode — exposes AK8963 at 0x0C on main I2C bus
  writeByte(MPU9250_ADDR, MPU9250_USER_CTRL, 0x00);   // Disable I2C master
  writeByte(MPU9250_ADDR, MPU9250_INT_PIN_CFG, 0x02); // BYPASS_EN
  delay(10);

  Serial.println("[IMU] Configured (bypass mode for AK8963)");

  // === Detect AK8963 magnetometer at 0x0C ===
  Wire.beginTransmission(AK8963_ADDR);
  uint8_t err = Wire.endTransmission();
  if (err == 0) {
    uint8_t wia = readByte(AK8963_ADDR, AK8963_WIA);
    Serial.printf("[MAG] AK8963 WIA: 0x%02X (expected 0x48)\n", wia);

    if (wia == 0x48) {
      ak8963_found = true;
      mag_found = true;

      // Power down before changing mode
      writeByte(AK8963_ADDR, AK8963_CNTL1, 0x00);
      delay(10);

      // Read sensitivity adjustment values from fuse ROM
      writeByte(AK8963_ADDR, AK8963_CNTL1, 0x0F); // Fuse ROM access mode
      delay(10);
      uint8_t asa[3];
      readBytes(AK8963_ADDR, AK8963_ASAX, 3, asa);
      magASA[0] = ((float)(asa[0] - 128) / 256.0f) + 1.0f;
      magASA[1] = ((float)(asa[1] - 128) / 256.0f) + 1.0f;
      magASA[2] = ((float)(asa[2] - 128) / 256.0f) + 1.0f;
      Serial.printf("[MAG] ASA: X=%.3f Y=%.3f Z=%.3f\n", magASA[0], magASA[1],
                    magASA[2]);

      // Power down, then set 16-bit output + continuous mode 2 (100Hz)
      writeByte(AK8963_ADDR, AK8963_CNTL1, 0x00);
      delay(10);
      writeByte(AK8963_ADDR, AK8963_CNTL1, 0x16); // 16-bit, 100Hz continuous
      delay(10);

      Serial.println("[MAG] AK8963 configured: 16-bit, 100Hz continuous");
    } else {
      Serial.println("[MAG] AK8963 WIA mismatch, magnetometer disabled");
    }
  } else {
    Serial.printf("[MAG] AK8963 not found at 0x0C (err=%d)\n", err);
    Serial.println("[MAG] Heading will use gyro integration (drift expected)");
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
  // Raw accel in chip frame
  float rawAX = ax / 16384.0f;
  float rawAY = ay / 16384.0f;
  float rawAZ = az / 16384.0f;

  // Axis remap for VERTICAL mounting (display facing driver, bottom down)
  // Chip Y-axis points up when flat → becomes Z (up) when vertical
  // Chip Z-axis points toward user when flat → becomes -Y when vertical
  accelX = rawAX;
  accelY = -rawAZ;
  accelZ = rawAY;

  // Gyroscope (±250 dps = 131 LSB/dps) — subtract bias
  int16_t gx = (rawData[8] << 8) | rawData[9];
  int16_t gy = (rawData[10] << 8) | rawData[11];
  int16_t gz = (rawData[12] << 8) | rawData[13];
  float rawGX = gx / 131.0f - gyroBiasX;
  float rawGY = gy / 131.0f - gyroBiasY;
  float rawGZ = gz / 131.0f - gyroBiasZ;

  // Same axis remap for gyro
  gyroX = rawGX;
  gyroY = -rawGZ;
  gyroZ = rawGY;

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

  // Convert quaternion to Euler angles (raw, before calibration)
  float rawPitch = asin(-2.0f * (q1 * q3 - q0 * q2)) * RAD_TO_DEG;
  float rawRoll =
      atan2(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) *
      RAD_TO_DEG;

  // Auto-calibrate: average first CAL_SAMPLES readings as zero reference
  if (!calDone) {
    pitchAccum += rawPitch;
    rollAccum += rawRoll;
    calCount++;
    if (calCount >= CAL_SAMPLES) {
      pitchOffset = pitchAccum / CAL_SAMPLES;
      rollOffset = rollAccum / CAL_SAMPLES;
      calDone = true;
      Serial.printf("[CAL] Pitch offset: %.2f, Roll offset: %.2f\n",
                    pitchOffset, rollOffset);
    }
  }

  pitch = rawPitch - pitchOffset;
  roll = rawRoll - rollOffset;
}

static void readMagnetometer() {
  // Always run gyro yaw integration for heading baseline
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

  if (!mag_found)
    return;

  // Check AK8963 data-ready (ST1 bit 0)
  uint8_t st1 = readByte(AK8963_ADDR, AK8963_ST1);
  if (!(st1 & 0x01))
    return; // DRDY not set

  // Read 7 bytes: HXL..HZH + ST2 (MUST read ST2 to unlock next measurement)
  uint8_t rawData[7];
  readBytes(AK8963_ADDR, AK8963_HXL, 7, rawData);

  uint8_t st2 = rawData[6];
  // Check for magnetic sensor overflow
  if (st2 & 0x08)
    return; // HOFL bit set, data invalid

  // AK8963: Little-endian (LSB first)
  int16_t rx = (int16_t)((rawData[1] << 8) | rawData[0]);
  int16_t ry = (int16_t)((rawData[3] << 8) | rawData[2]);
  int16_t rz = (int16_t)((rawData[5] << 8) | rawData[4]);

  // Reject saturated readings (AK8963 range: -32760 to 32760)
  if (abs(rx) >= 32760 || abs(ry) >= 32760 || abs(rz) >= 32760)
    return;

  if (rx != 0 || ry != 0 || rz != 0) {
    // Apply sensitivity adjustment: Hadj = H * ((ASA-128)/256 + 1)
    magRawX = rx;
    magRawY = ry;
    magRawZ = rz;
    float adjX = rx * magASA[0];
    float adjY = ry * magASA[1];
    float adjZ = rz * magASA[2];

    magX = (adjX - magOffsetX) * magScaleX;
    magY = (adjY - magOffsetY) * magScaleY;
    magZ = (adjZ - magOffsetZ) * magScaleZ;

    // Tilt-compensated heading
    float cosRoll = cos(roll * DEG_TO_RAD);
    float sinRoll = sin(roll * DEG_TO_RAD);
    float cosPitch = cos(pitch * DEG_TO_RAD);
    float sinPitch = sin(pitch * DEG_TO_RAD);
    float Xh = magX * cosPitch + magZ * sinPitch;
    float Yh =
        magX * sinRoll * sinPitch + magY * cosRoll - magZ * sinRoll * cosPitch;
    float rawHeading = atan2(-Yh, Xh) * RAD_TO_DEG;
    rawHeading += MAGNETIC_DECLINATION;
    if (rawHeading < 0)
      rawHeading += 360;
    if (rawHeading >= 360)
      rawHeading -= 360;

    // Smooth with wrap-around
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
  uint8_t rawData[8]; // Read all 8 bytes (press + temp + hum)
  readBytes(BME280_ADDR, BME280_PRESS_MSB, 6, rawData);

  int32_t adc_P = ((int32_t)rawData[0] << 12) | ((int32_t)rawData[1] << 4) |
                  (rawData[2] >> 4);
  int32_t adc_T = ((int32_t)rawData[3] << 12) | ((int32_t)rawData[4] << 4) |
                  (rawData[5] >> 4);

  // Reject obviously bad reads (I2C bus failure)
  if (adc_T == 0 || adc_T == 0xFFFFF || adc_P == 0 || adc_P == 0xFFFFF) {
    static int failCount = 0;
    failCount++;
    if (failCount > 5) {
      Serial.println("[BME280] Re-initializing after repeated read failures");
      initBME280();
      failCount = 0;
    }
    return; // Keep previous good values
  }

  // Temperature compensation (Bosch reference)
  int32_t var1 =
      ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
                    ((adc_T >> 4) - ((int32_t)dig_T1))) >>
                   12) *
                  ((int32_t)dig_T3)) >>
                 14;
  int32_t t_fine = var1 + var2;
  float rawTemp = (float)((t_fine * 5 + 128) >> 8) / 100.0f;

  // Sanity check: BME280 range is -40 to +85°C
  if (rawTemp < -40.0f || rawTemp > 85.0f) {
    return; // Reject out-of-range values
  }

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

  // Altitude (m ASL) — uses weather API SLP if available, else calibrated from
  // 920m
  if (pressure > 0) {
    extern float wifi_get_sea_level_pressure();
    float weatherSLP = wifi_get_sea_level_pressure();

    static float calibratedSLP = 0;
    if (calibratedSLP == 0 && pressure > 800 && pressure < 1200) {
      calibratedSLP =
          pressure / pow(1.0f - LOCAL_ELEVATION_METERS / 44330.0f, 5.255f);
      Serial.printf("[ALT] BME280-cal SLP: %.2f hPa\n", calibratedSLP);
    }

    float slp = (weatherSLP > 900 && weatherSLP < 1100) ? weatherSLP
                : (calibratedSLP > 0)                   ? calibratedSLP
                                                        : 1013.25f;
    float rawAlt = 44330.0f * (1.0f - pow(pressure / slp, 0.1903f));
    static float altFiltered = -9999;
    if (altFiltered < -9000)
      altFiltered = rawAlt;
    altFiltered = altFiltered * 0.85f + rawAlt * 0.15f;
    altitude = altFiltered;
  }
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

  // BME280 is on the same I2C bus — no bypass, no bus corruption
  // Try at default address 0x76, then 0x77
  bme280_found = initBME280();
  if (!bme280_found) {
    uint8_t chipId77 = readByte(0x77, BME280_CHIP_ID);
    if (chipId77 == 0x60 || chipId77 == 0x58) {
      Serial.printf("[BME280] Found at 0x77 (ID: 0x%02X)\n", chipId77);
    }
  }

  Serial.println("========================================");
  Serial.printf("[SENSORS] MPU9250:  %s\n", mpu9250_found ? "OK" : "NOT FOUND");
  Serial.printf("[SENSORS] MAG:      %s (%s)\n", mag_found ? "OK" : "NOT FOUND",
                ak8963_found ? "AK8963" : "none");
  Serial.printf("[SENSORS] BME280:   %s\n", bme280_found ? "OK" : "NOT FOUND");
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
    // Heading smoothing applied in readMagnetometer()
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

// ============== Auto Mag Calibration ==============
void sensors_auto_calibrate_mag(int durationSec) {
  if (!mag_found) {
    Serial.println("[MAG_CAL] No magnetometer found, skipping");
    return;
  }

  // Skip if we already have valid calibration
  if (magOffsetX != 0.0f || magOffsetY != 0.0f || magOffsetZ != 0.0f) {
    Serial.println("[MAG_CAL] Existing calibration found, skipping auto-cal");
    return;
  }

  Serial.printf("[MAG_CAL] Running %ds auto-calibration...\n", durationSec);

  int16_t minX = 32767, maxX = -32768;
  int16_t minY = 32767, maxY = -32768;
  int16_t minZ = 32767, maxZ = -32768;
  int samples = 0;
  unsigned long start = millis();

  while (millis() - start < (unsigned long)(durationSec * 1000)) {
    // Check AK8963 DRDY
    uint8_t st = readByte(AK8963_ADDR, AK8963_ST1);
    if (st & 0x01) {
      uint8_t rawData[7];
      readBytes(AK8963_ADDR, AK8963_HXL, 7, rawData);
      uint8_t st2 = rawData[6];
      if (st2 & 0x08) {
        delay(10);
        continue;
      } // HOFL overflow
      int16_t rx = (int16_t)((rawData[1] << 8) | rawData[0]);
      int16_t ry = (int16_t)((rawData[3] << 8) | rawData[2]);
      int16_t rz = (int16_t)((rawData[5] << 8) | rawData[4]);

      // Reject saturated values
      if (abs(rx) < 32760 && abs(ry) < 32760 && abs(rz) < 32760 &&
          (rx != 0 || ry != 0 || rz != 0)) {
        if (rx < minX)
          minX = rx;
        if (rx > maxX)
          maxX = rx;
        if (ry < minY)
          minY = ry;
        if (ry > maxY)
          maxY = ry;
        if (rz < minZ)
          minZ = rz;
        if (rz > maxZ)
          maxZ = rz;
        samples++;
      }
    }
    delay(20);
  }

  if (samples < 50) {
    Serial.printf("[MAG_CAL] Only %d samples, not enough data\n", samples);
    return;
  }

  // Hard-iron offsets
  magOffsetX = (maxX + minX) / 2.0f;
  magOffsetY = (maxY + minY) / 2.0f;
  magOffsetZ = (maxZ + minZ) / 2.0f;

  // Soft-iron scale (guard against zero range)
  float rangeX = max(1.0f, (maxX - minX) / 2.0f);
  float rangeY = max(1.0f, (maxY - minY) / 2.0f);
  float rangeZ = max(1.0f, (maxZ - minZ) / 2.0f);
  float avgRange = (rangeX + rangeY + rangeZ) / 3.0f;
  magScaleX = avgRange / rangeX;
  magScaleY = avgRange / rangeY;
  magScaleZ = avgRange / rangeZ;

  Serial.printf("[MAG_CAL] Done (%d samples): offset(%.1f,%.1f,%.1f) "
                "scale(%.3f,%.3f,%.3f)\n",
                samples, magOffsetX, magOffsetY, magOffsetZ, magScaleX,
                magScaleY, magScaleZ);

  // Save to SPIFFS
  sensors_save_calibration();
}
