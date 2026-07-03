#include "commands.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <stdlib.h>
#include <string.h>

#include "attitude.h"
#include "ble_media.h"
#include "calibration.h"
#include "display.h"
#include "leds.h"
#include "sensors.h"

// Globals owned by BenzCluster.ino — shared so BLE and HTTP commands stay in
// perfect sync (both paths call into this file, never duplicate logic).
extern int screenBrightness;
extern bool calibrationMode;

#define ATT_STYLE_FILE "/attitude_style.txt"

static void saveAttitudeStyle(uint8_t s) {
  File f = SPIFFS.open(ATT_STYLE_FILE, "w");
  if (f) {
    f.print((int)s);
    f.close();
  }
}

void cluster_load_attitude_style() {
  File f = SPIFFS.open(ATT_STYLE_FILE, "r");
  if (!f) return;
  int s = f.parseInt();
  f.close();
  attitude_set_style((uint8_t)s);
  Serial.printf("[CMD] Attitude style restored: %d\n", s);
}

bool cluster_handle_command(const char *cmd, char *reply, int replyLen) {
  if (!cmd || !reply || replyLen <= 0) return false;
  reply[0] = '\0';

  const char *eq = strchr(cmd, '=');
  char key[32];
  if (eq) {
    size_t klen = (size_t)(eq - cmd);
    if (klen >= sizeof(key)) klen = sizeof(key) - 1;
    memcpy(key, cmd, klen);
    key[klen] = '\0';
  } else {
    strncpy(key, cmd, sizeof(key) - 1);
    key[sizeof(key) - 1] = '\0';
  }
  const char *val = eq ? eq + 1 : "";

  if (!strcmp(key, "attitude_style")) {
    uint8_t s = (uint8_t)atoi(val);
    attitude_set_style(s);
    saveAttitudeStyle(attitude_get_style());
    snprintf(reply, replyLen, "attitude_style=%d", attitude_get_style());
    Serial.printf("[CMD] %s\n", reply);
    return true;
  }

  if (!strcmp(key, "factory_zero")) {
    sensors_factory_zero_orientation();
    snprintf(reply, replyLen, "orientation zeroed");
    return true;
  }

  if (!strcmp(key, "factory_magcal")) {
    int secs = atoi(val);
    if (secs < 5) secs = 20;
    if (secs > 120) secs = 120;
    sensors_factory_mag_calibrate(secs);
    snprintf(reply, replyLen, "mag cal done (%ds)", secs);
    return true;
  }

  if (!strcmp(key, "declination")) {
    sensors_set_declination((float)atof(val));
    snprintf(reply, replyLen, "declination=%s", val);
    return true;
  }

  if (!strcmp(key, "debug_sensors")) {
    sensors_debug_string(reply, replyLen);
    return true;
  }

  // --- Legacy control surface (LED / brightness / music / power) ---
  // These mirror the HTTP routes in ota_update.cpp exactly, so BLE-only
  // customers (no WiFi configured) get full control from the app too.
  if (!strcmp(key, "led_on")) {
    leds_set_mode(LED_MODE_CUSTOM);
    snprintf(reply, replyLen, "led on");
    return true;
  }
  if (!strcmp(key, "led_off")) {
    leds_set_mode(LED_MODE_OFF);
    snprintf(reply, replyLen, "led off");
    return true;
  }
  if (!strcmp(key, "led_color")) {
    char clean[16];
    const char *src = val;
    if (src[0] == '#') src++;
    strncpy(clean, src, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    leds_set_color((uint32_t)strtoul(clean, nullptr, 16));
    snprintf(reply, replyLen, "led_color=%s", clean);
    return true;
  }
  if (!strcmp(key, "led_brightness")) {
    leds_set_brightness(atoi(val));
    snprintf(reply, replyLen, "led_brightness=%s", val);
    return true;
  }
  if (!strcmp(key, "brightness")) {
    screenBrightness = constrain(atoi(val), 10, 100);
    display_set_brightness(screenBrightness);
    snprintf(reply, replyLen, "brightness=%d", screenBrightness);
    return true;
  }
  if (!strcmp(key, "play")) {
    ble_media_play_pause();
    snprintf(reply, replyLen, "play");
    return true;
  }
  if (!strcmp(key, "next")) {
    ble_media_next();
    snprintf(reply, replyLen, "next");
    return true;
  }
  if (!strcmp(key, "prev")) {
    ble_media_prev();
    snprintf(reply, replyLen, "prev");
    return true;
  }
  if (!strcmp(key, "gyro_reset")) {
    sensors_auto_calibrate_gyro();
    snprintf(reply, replyLen, "gyro recalibrated");
    return true;
  }
  if (!strcmp(key, "calibrate")) {
    calibrationMode = true;
    calibration_start();
    snprintf(reply, replyLen, "calibration started (30s)");
    return true;
  }
  if (!strcmp(key, "reboot")) {
    snprintf(reply, replyLen, "rebooting");
    Serial.println("[CMD] Reboot requested");
    // Deferred so the BLE/HTTP reply actually reaches the phone first.
    static bool rebootPending = false;
    if (!rebootPending) {
      rebootPending = true;
      // main loop's delay(5) gives this ~5ms of margin; that's enough for the
      // BLE stack to flush the notification before restart.
      delay(150);
      ESP.restart();
    }
    return true;
  }

  return false;
}
