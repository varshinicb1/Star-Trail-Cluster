/*!
 * @file BMP180.cpp
 *
 * @mainpage Adafruit BMP085 Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the Adafruit BMP085/BMP180 Barometric Pressure + Temp
 * sensor
 *
 * Designed specifically to work with the Adafruit BMP085 or BMP180 Breakout
 * ----> http://www.adafruit.com/products/391
 * ----> http://www.adafruit.com/products/1603
 *
 * These displays use I2C to communicate, 2 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * Updated by Samy Kamkar for cross-platform support.
 *
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include "I2C_Insarianne.h"
#include <Adafruit_I2CDevice.h>

BMP180::BMP180() { i2c_dev = nullptr; }

bool BMP180::begin(uint8_t mode, TwoWire *wire) {
  if (mode > BMP085_ULTRAHIGHRES)
    mode = BMP085_ULTRAHIGHRES;
  oversampling = mode;

  if (i2c_dev) {
    delete i2c_dev; // remove old interface
  }

  i2c_dev = new Adafruit_I2CDevice(BMP085_I2CADDR, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  if (read8(0xD0) != 0x55)
    return false;

  /* read calibration data */
  ac1 = read16(BMP085_CAL_AC1);
  ac2 = read16(BMP085_CAL_AC2);
  ac3 = read16(BMP085_CAL_AC3);
  ac4 = read16(BMP085_CAL_AC4);
  ac5 = read16(BMP085_CAL_AC5);
  ac6 = read16(BMP085_CAL_AC6);

  b1 = read16(BMP085_CAL_B1);
  b2 = read16(BMP085_CAL_B2);

  mb = read16(BMP085_CAL_MB);
  mc = read16(BMP085_CAL_MC);
  md = read16(BMP085_CAL_MD);

  return true;
}

int32_t BMP180::computeB5(int32_t UT) {
  int32_t X1 = (UT - (int32_t)ac6) * ((int32_t)ac5) >> 15;
  int32_t X2 = ((int32_t)mc << 11) / (X1 + (int32_t)md);
  return X1 + X2;
}

uint16_t BMP180::readRawTemperature(void) {
  write8(BMP085_CONTROL, BMP085_READTEMPCMD);
  delay(5);
  return read16(BMP085_TEMPDATA);
}

uint32_t BMP180::readRawPressure(void) {
  uint32_t raw;

  write8(BMP085_CONTROL, BMP085_READPRESSURECMD + (oversampling << 6));

  if (oversampling == BMP085_ULTRALOWPOWER)
    delay(5);
  else if (oversampling == BMP085_STANDARD)
    delay(8);
  else if (oversampling == BMP085_HIGHRES)
    delay(14);
  else
    delay(26);

  raw = read16(BMP085_PRESSUREDATA);

  raw <<= 8;
  raw |= read8(BMP085_PRESSUREDATA + 2);
  raw >>= (8 - oversampling);

  return raw;
}

void  BMP180::read_sensor(void) {
  readPressure();
  readTemperature();
  readAltitude();
}

void BMP180::readPressure(void) {
  int32_t UT, UP, B3, B5, B6, X1, X2, X3, p;
  uint32_t B4, B7;

  UT = readRawTemperature();
  UP = readRawPressure();

  B5 = computeB5(UT);

  // do pressure calcs
  B6 = B5 - 4000;
  X1 = ((int32_t)b2 * ((B6 * B6) >> 12)) >> 11;
  X2 = ((int32_t)ac2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = ((((int32_t)ac1 * 4 + X3) << oversampling) + 2) / 4;

  X1 = ((int32_t)ac3 * B6) >> 13;
  X2 = ((int32_t)b1 * ((B6 * B6) >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = ((uint32_t)ac4 * (uint32_t)(X3 + 32768)) >> 15;
  B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL >> oversampling);


  if (B7 < 0x80000000) {
    p = (B7 * 2) / B4;
  } else {
    p = (B7 / B4) * 2;
  }
  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;

  p = p + ((X1 + X2 + (int32_t)3791) >> 4);
  pressure = p;
}

void BMP180::setSealevelPressure(float altitude_meters) {
  readPressure();
  sealevelpressure = (int32_t)(pressure / pow(1.0 - altitude_meters / 44330, 5.255));
}

void BMP180::readTemperature(void) {
  int32_t UT, B5; // following ds convention
  float temp;

  UT = readRawTemperature();

  B5 = computeB5(UT);
  temp = (B5 + 8) >> 4;
  temp /= 10;

  temperature = temp;
}

void BMP180::readAltitude(void) {
  altitude = 44330 * (1.0 - pow(pressure / sealevelpressure, 0.1903));
}

/*********************************************************************/

uint8_t BMP180::read8(uint8_t a) {
  uint8_t ret;

  // send 1 byte, reset i2c, read 1 byte
  i2c_dev->write_then_read(&a, 1, &ret, 1, true);

  return ret;
}

uint16_t BMP180::read16(uint8_t a) {
  uint8_t retbuf[2];
  uint16_t ret;

  // send 1 byte, reset i2c, read 2 bytes
  // we could typecast uint16_t as uint8_t array but would need to ensure proper
  // endianness
  i2c_dev->write_then_read(&a, 1, retbuf, 2, true);

  // write_then_read uses uint8_t array
  ret = retbuf[1] | (retbuf[0] << 8);

  return ret;
}

void BMP180::write8(uint8_t a, uint8_t d) {
  // send d prefixed with a (a d [stop])
  i2c_dev->write(&d, 1, true, &a, 1);
}
