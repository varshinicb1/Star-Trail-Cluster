#include "gps.h"
#include <Arduino.h>

// GPS: Quectel L86 on U0TXD(GPIO43) / U0RXD(GPIO44)
// Use Serial1 remapped to avoid USB-CDC conflict on Serial/HardwareSerial(0)
#define GPS_RX_PIN 44 // ESP32 RX ← GPS TX
#define GPS_TX_PIN 43 // ESP32 TX → GPS RX
#define GPS_BAUD 9600

static float speedKmh = 0;
static float courseHeading = 0; // True heading from GPS (degrees)
static float latitude = 0;
static float longitude = 0;
static bool hasFix = false;
static bool hasCourse = false;
static char nmeaBuf[128];
static int nmeaIdx = 0;

// Convert NMEA ddmm.mmmmm to decimal degrees
static float nmeaToDecimal(const char *str, char dir) {
  if (!str || *str == '\0')
    return 0;
  float raw = atof(str);
  int degrees = (int)(raw / 100);
  float minutes = raw - (degrees * 100);
  float decimal = degrees + (minutes / 60.0f);
  if (dir == 'S' || dir == 'W')
    decimal = -decimal;
  return decimal;
}

// Parse $GPRMC / $GNRMC for speed, course, lat, lon
static void parseRMC(const char *sentence) {
  char fields[12][20];
  memset(fields, 0, sizeof(fields));

  int fieldIdx = 0;
  int charIdx = 0;
  const char *p = sentence;

  while (*p && fieldIdx < 12) {
    if (*p == ',' || *p == '*') {
      fields[fieldIdx][charIdx] = '\0';
      fieldIdx++;
      charIdx = 0;
    } else if (charIdx < 19) {
      fields[fieldIdx][charIdx++] = *p;
    }
    p++;
  }

  if (fields[2][0] == 'A') {
    hasFix = true;

    if (fields[3][0] != '\0') {
      latitude = nmeaToDecimal(fields[3], fields[4][0]);
    }
    if (fields[5][0] != '\0') {
      longitude = nmeaToDecimal(fields[5], fields[6][0]);
    }
    if (fields[7][0] != '\0') {
      float knots = atof(fields[7]);
      speedKmh = knots * 1.852f;
    }
    if (fields[8][0] != '\0') {
      courseHeading = atof(fields[8]);
      hasCourse = (speedKmh > 2.0f);
    } else {
      hasCourse = false;
    }
  } else {
    hasFix = false;
    hasCourse = false;
  }
}

void gps_init() {
  Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.printf("[GPS] Serial1 on RX=%d TX=%d at %d baud\n", GPS_RX_PIN,
                GPS_TX_PIN, GPS_BAUD);
}

void gps_update() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (nmeaIdx > 5) {
        nmeaBuf[nmeaIdx] = '\0';
        if (strncmp(nmeaBuf, "$GPRMC", 6) == 0 ||
            strncmp(nmeaBuf, "$GNRMC", 6) == 0) {
          parseRMC(nmeaBuf);
          Serial.printf("[GPS] Fix:%d Spd:%.1f Crs:%.1f Lat:%.5f Lon:%.5f\n",
                        hasFix, speedKmh, courseHeading, latitude, longitude);
        }
      }
      nmeaIdx = 0;
    } else if (nmeaIdx < (int)sizeof(nmeaBuf) - 1) {
      nmeaBuf[nmeaIdx++] = c;
    }
  }
}

float gps_get_speed_kmh() { return speedKmh; }
float gps_get_course() { return courseHeading; }
float gps_get_latitude() { return latitude; }
float gps_get_longitude() { return longitude; }
bool gps_has_fix() { return hasFix; }
bool gps_has_course() { return hasCourse; }
