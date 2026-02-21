#include "gps.h"
#include <Arduino.h>

// GPS on Serial2 — connect GPS TX to GPIO16 (RX2)
#define GPS_RX_PIN 16
#define GPS_BAUD 9600

static float speedKmh = 0;
static bool hasFix = false;
static char nmeaBuf[128];
static int nmeaIdx = 0;

// Parse $GPRMC for speed in knots, convert to km/h
static void parseRMC(const char *sentence) {
  // $GPRMC,time,status,lat,N/S,lon,E/W,speed_knots,course,...
  int commaCount = 0;
  const char *p = sentence;
  const char *speedStart = NULL;
  const char *statusChar = NULL;

  while (*p) {
    if (*p == ',') {
      commaCount++;
      if (commaCount == 2)
        statusChar = p + 1; // A=valid, V=void
      if (commaCount == 7)
        speedStart = p + 1; // speed in knots
    }
    p++;
  }

  if (statusChar && *statusChar == 'A') {
    hasFix = true;
    if (speedStart) {
      float knots = atof(speedStart);
      speedKmh = knots * 1.852f;
    }
  } else {
    hasFix = false;
  }
}

void gps_init() {
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, -1); // RX only
  Serial.printf("[GPS] Listening on GPIO%d at %d baud\n", GPS_RX_PIN, GPS_BAUD);
}

void gps_update() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      if (nmeaIdx > 5) {
        nmeaBuf[nmeaIdx] = '\0';
        if (strncmp(nmeaBuf, "$GPRMC", 6) == 0 ||
            strncmp(nmeaBuf, "$GNRMC", 6) == 0) {
          parseRMC(nmeaBuf);
        }
      }
      nmeaIdx = 0;
    } else if (nmeaIdx < (int)sizeof(nmeaBuf) - 1) {
      nmeaBuf[nmeaIdx++] = c;
    }
  }
}

float gps_get_speed_kmh() { return speedKmh; }
bool gps_has_fix() { return hasFix; }
