#include "commands.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <stdlib.h>
#include <string.h>

#include "attitude.h"
#include "sensors.h"

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

  return false;
}
