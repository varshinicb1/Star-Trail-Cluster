/*
 * Star Trail Instrument Cluster v6.0
 * CrowPanel 1.28" ESP32-S3 Round Display
 *
 * 7 Swipeable Widgets + Hidden System Menu (3s encoder hold)
 * WiFi OTA updates (no more BOOT+RESET!)
 * Rotary: LED brightness / volume / clock face
 * Encoder click: Play/Pause on music (live icon toggle)
 * Encoder 3s hold: toggle System overlay (Reboot)
 * Last widget remembered across reboots
 */

#define LGFX_USE_V1
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <LovyanGFX.hpp>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include <lvgl.h>
#include <time.h>

#include "alttemp.h"
#include "attitude.h"
#include "ble_media.h"
#include "calibration.h"
#include "clock.h"
#include "compass.h"
#include "config.h"
#include "display.h"
#include "gps.h"
#include "ledcolor.h"
#include "leds.h"
#include "music.h"
#include "ota_update.h"
#include "sensors.h"
#include "sensorview.h"
#include "splash.h"
#include "systemview.h"
#include "wifi_manager.h"

#define NUM_WIDGETS 7
#define W_COMPASS 0
#define W_ATTITUDE 1
#define W_ALTTEMP 2
#define W_CLOCK 3
#define W_MUSIC 4
#define W_LEDCOLOR 5
#define W_SENSORS 6

int currentWidget = 0;
int brightness = 50;
int musicVolume = 50;
int screenBrightness = 100;
bool calibrationMode = false;
bool systemVisible = false; // System overlay state

volatile float gH = 0, gP = 0, gR = 0;
volatile float gT = 0, gA = 0, gPr = 0;

TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;

void sensorTask(void *p);
void ledTask(void *p);
void switchWidget(int dir);

// ===== Gesture gating: block UP/DOWN on interactive widgets =====
static bool gestureBlocked() {
  return (currentWidget == W_LEDCOLOR || systemVisible);
}

static const char *wName[] = {"Compass", "Attitude", "AltTemp", "Clock",
                              "Music",   "LED",      "Sensors"};

void setup() {
  esp_task_wdt_deinit();
  pinMode(40, OUTPUT);
  digitalWrite(40, LOW);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  Serial.begin(115200);
  delay(500);
  Serial.println("\n========================================");
  Serial.println("[BOOT] Star Trail v6.0");
  Serial.println("========================================");

  if (!SPIFFS.begin(true))
    Serial.println("[SPIFFS] Failed!");
  else
    Serial.println("[SPIFFS] OK");

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire1.begin(TP_SDA, TP_SCL);

  display_init();
  leds_init();
  leds_set_mode(LED_MODE_BOOT);

  // Splash screen with animated progress bar
  splash_show();

  wifi_init();
  configTime(19800, 0, "pool.ntp.org"); // IST = UTC+5:30

  // BLE Media Controller
  ble_media_init();

  // OTA Updates (after WiFi)
  ota_init();

  // GPS
  gps_init();

  // Sensors
  if (sensors_init())
    Serial.println("[SENSORS] OK");
  else
    Serial.println("[SENSORS] Warning!");

  sensors_load_calibration();

  // The gyro calibration takes 3s, matching the splash progress bar animation
  sensors_auto_calibrate_gyro();

  delay(500);
  splash_welcome(WELCOME_NAME);
  leds_set_mode(LED_MODE_WELCOME);
  delay(2000); // Give the welcome animation time to finish
  leds_set_mode(LED_MODE_NORMAL);

  // Init all widgets
  compass_init();
  attitude_init();
  alttemp_init();
  clock_init();
  music_init();
  ledcolor_init();
  sensorview_init();
  systemview_init();

  // Restore last widget from SPIFFS
  int savedWidget = 0;
  File wf = SPIFFS.open("/last_widget.txt", "r");
  if (wf) {
    savedWidget = wf.parseInt();
    wf.close();
    if (savedWidget < 0 || savedWidget >= NUM_WIDGETS)
      savedWidget = 0;
    Serial.printf("[UI] Restored widget: %s\n", wName[savedWidget]);
  }
  currentWidget = savedWidget;
  switch (currentWidget) {
  case W_COMPASS:
    compass_show();
    break;
  case W_ATTITUDE:
    attitude_show();
    break;
  case W_ALTTEMP:
    alttemp_show();
    break;
  case W_CLOCK:
    clock_show();
    break;
  case W_MUSIC:
    music_show();
    break;
  case W_LEDCOLOR:
    ledcolor_show();
    break;
  case W_SENSORS:
    sensorview_show();
    break;
  }
  Serial.printf("[UI] %s (%d/%d)\n", wName[currentWidget], currentWidget + 1,
                NUM_WIDGETS);

  xTaskCreatePinnedToCore(sensorTask, "Sens", 8192, NULL, 2, &sensorTaskHandle,
                          0);
  xTaskCreatePinnedToCore(ledTask, "LED", 2048, NULL, 1, &ledTaskHandle, 0);
}

void loop() {
  lv_timer_handler();
  ota_handle(); // Check for OTA updates

  // GPS update
  gps_update();

  // ===== Touch gestures — gated on interactive widgets =====
  uint8_t g = touch_get_gesture();
  if (g != 0) {
    switch (g) {
    case GESTURE_SWIPE_UP:
      if (!gestureBlocked())
        switchWidget(1);
      break;
    case GESTURE_SWIPE_DOWN:
      if (!gestureBlocked())
        switchWidget(-1);
      break;
    case GESTURE_SWIPE_LEFT:
      if (currentWidget == W_MUSIC) {
        music_prev_track();
      } else if (gestureBlocked()) {
        switchWidget(-1); // L/R to navigate on widgets that block U/D
      } else {
        screenBrightness = constrain(screenBrightness - 10, 10, 100);
        display_set_brightness(screenBrightness);
      }
      break;
    case GESTURE_SWIPE_RIGHT:
      if (currentWidget == W_MUSIC) {
        music_next_track();
      } else if (gestureBlocked()) {
        switchWidget(1);
      } else {
        screenBrightness = constrain(screenBrightness + 10, 10, 100);
        display_set_brightness(screenBrightness);
      }
      break;
    }
  }

  // ===== Long-press touch on Clock: change watch face =====
  if (currentWidget == W_CLOCK) {
    static uint32_t touchStart = 0;
    static bool touchActive = false;
    uint16_t tx, ty;
    if (touch_get_point(&tx, &ty)) {
      if (!touchActive) {
        touchStart = millis();
        touchActive = true;
      } else if (millis() - touchStart > 2000) {
        clock_next_face();
        touchActive = false;
        delay(500); // debounce
      }
    } else {
      touchActive = false;
    }
  }

  // ===== Rotary encoder =====
  int8_t enc = encoder_read();
  if (enc != 0) {
    if (currentWidget == W_MUSIC) {
      musicVolume = constrain(musicVolume + enc * 5, 0, 100);
      music_set_volume(musicVolume);
    } else if (currentWidget == W_CLOCK) {
      if (enc > 0)
        clock_next_face();
    } else {
      brightness = constrain(brightness + enc * 10, 0, 100);
      leds_set_brightness(brightness);
    }
  }

  // ===== Encoder button: short click = Play/Pause, 3s hold = System Menu =====
  static uint32_t btnDownTime = 0;
  static bool btnHeld = false;       // True while button is down
  static bool holdTriggered = false; // Prevents re-trigger
  bool btnDown = encoder_button_pressed();

  if (btnDown && !btnHeld) {
    // Rising edge: button just pressed
    btnDownTime = millis();
    btnHeld = true;
    holdTriggered = false;
  }

  if (btnDown && btnHeld && !holdTriggered && (millis() - btnDownTime > 3000)) {
    // 3-second hold achieved
    holdTriggered = true;
    if (!systemVisible) {
      systemVisible = true;
      systemview_show();
      Serial.println("[UI] System overlay ON (3s hold)");
    } else {
      systemVisible = false;
      // Re-show current widget
      switch (currentWidget) {
      case W_COMPASS:
        compass_show();
        break;
      case W_ATTITUDE:
        attitude_show();
        break;
      case W_ALTTEMP:
        alttemp_show();
        break;
      case W_CLOCK:
        clock_show();
        break;
      case W_MUSIC:
        music_show();
        break;
      case W_LEDCOLOR:
        ledcolor_show();
        break;
      case W_SENSORS:
        sensorview_show();
        break;
      }
      Serial.println("[UI] System overlay OFF (3s hold)");
    }
  }

  if (!btnDown && btnHeld) {
    // Falling edge: button released
    uint32_t held = millis() - btnDownTime;
    if (!holdTriggered && held < 1000) {
      // Short click
      if (currentWidget == W_MUSIC && !systemVisible) {
        ble_media_play_pause();
        music_toggle_play();
      }
    }
    btnHeld = false;
  }

  // ===== Update current widget =====
  switch (currentWidget) {
  case W_COMPASS:
    compass_update(gH);
    compass_update_speed(gps_get_speed_kmh());
    break;
  case W_ATTITUDE:
    attitude_update(gP, gR);
    break;
  case W_ALTTEMP:
    alttemp_update(gT, gA);
    break;
  case W_CLOCK:
    clock_update();
    break;
  case W_MUSIC:
    music_update();
    break;
  case W_LEDCOLOR:
    ledcolor_update();
    break;
  case W_SENSORS: {
    int16_t mx, my, mz;
    float ax, ay, az, gx, gy, gz;
    sensors_get_mag_raw(&mx, &my, &mz);
    sensors_get_accel(&ax, &ay, &az);
    sensors_get_gyro(&gx, &gy, &gz);
    sensorview_update(gH, gP, gR, gT, gA, gPr, mx, my, mz, ax, ay, az, gx, gy,
                      gz);
    break;
  }
  }
  delay(5);
}

void switchWidget(int dir) {
  switch (currentWidget) {
  case W_COMPASS:
    compass_hide();
    break;
  case W_ATTITUDE:
    attitude_hide();
    break;
  case W_ALTTEMP:
    alttemp_hide();
    break;
  case W_CLOCK:
    clock_hide();
    break;
  case W_MUSIC:
    music_hide();
    break;
  case W_LEDCOLOR:
    ledcolor_hide();
    break;
  case W_SENSORS:
    sensorview_hide();
    break;
  }
  currentWidget = (currentWidget + dir + NUM_WIDGETS) % NUM_WIDGETS;
  switch (currentWidget) {
  case W_COMPASS:
    compass_show();
    break;
  case W_ATTITUDE:
    attitude_show();
    break;
  case W_ALTTEMP:
    alttemp_show();
    break;
  case W_CLOCK:
    clock_show();
    break;
  case W_MUSIC:
    music_show();
    break;
  case W_LEDCOLOR:
    ledcolor_show();
    break;
  case W_SENSORS:
    sensorview_show();
    break;
  }
  Serial.printf("[UI] %s (%d/%d)\n", wName[currentWidget], currentWidget + 1,
                NUM_WIDGETS);
  // Save last widget to SPIFFS
  File wf = SPIFFS.open("/last_widget.txt", "w");
  if (wf) {
    wf.print(currentWidget);
    wf.close();
  }
}

void sensorTask(void *p) {
  TickType_t lw = xTaskGetTickCount();
  uint32_t lp = 0;
  while (true) {
    sensors_update();
    gH = sensors_get_heading();
    gP = sensors_get_pitch();
    gR = sensors_get_roll();
    gT = sensors_get_temperature();
    gA = sensors_get_altitude();
    gPr = sensors_get_pressure();
    if (millis() - lp > 500) {
      int16_t mx, my, mz;
      sensors_get_mag_raw(&mx, &my, &mz);
      Serial.printf("[S] H:%.0f P:%.1f R:%.1f T:%.1f M:%d,%d,%d\n", gH, gP, gR,
                    gT, mx, my, mz);
      lp = millis();
    }
    if (calibrationMode)
      calibration_update();
    vTaskDelayUntil(&lw, pdMS_TO_TICKS(20));
  }
}

void ledTask(void *p) {
  while (true) {
    leds_update();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
