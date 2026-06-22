#include "sensorview.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

static lv_obj_t *sensorScreen = NULL;
static lv_obj_t *imuLabel = NULL;
static lv_obj_t *magLabel = NULL;
static lv_obj_t *envLabel = NULL;
static lv_obj_t *headingLabel = NULL;

void sensorview_init() {
  sensorScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(sensorScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(sensorScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Title - centered at top
  lv_obj_t *title = lv_label_create(sensorScreen);
  lv_label_set_text(title, "SENSORS");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x44FF44), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

  // Heading/Pitch/Roll - centered
  headingLabel = lv_label_create(sensorScreen);
  lv_label_set_text(headingLabel, "H:--- P:--- R:---");
  lv_obj_set_style_text_font(headingLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(headingLabel, lv_color_hex(0xFFFF00), 0);
  lv_obj_align(headingLabel, LV_ALIGN_CENTER, 0, -45);

  // Accel + gyro - centered
  imuLabel = lv_label_create(sensorScreen);
  lv_label_set_text(imuLabel, "A: --- --- ---\nG: --- --- ---");
  lv_obj_set_style_text_font(imuLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(imuLabel, lv_color_hex(0x00CCFF), 0);
  lv_obj_align(imuLabel, LV_ALIGN_CENTER, 0, -15);

  // Magnetometer - centered
  magLabel = lv_label_create(sensorScreen);
  lv_label_set_text(magLabel, "M: --- --- ---");
  lv_obj_set_style_text_font(magLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(magLabel, lv_color_hex(0xFF88FF), 0);
  lv_obj_align(magLabel, LV_ALIGN_CENTER, 0, 15);

  // Environment - centered
  envLabel = lv_label_create(sensorScreen);
  lv_label_set_text(envLabel, "T:---  A:---  P:---");
  lv_obj_set_style_text_font(envLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(envLabel, lv_color_hex(0xFF8844), 0);
  lv_obj_align(envLabel, LV_ALIGN_CENTER, 0, 40);

  // Legend
  lv_obj_t *legend = lv_label_create(sensorScreen);
  lv_label_set_text(legend, "Live sensor readout");
  lv_obj_set_style_text_font(legend, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(legend, lv_color_hex(0x444444), 0);
  lv_obj_align(legend, LV_ALIGN_BOTTOM_MID, 0, -30);
}

void sensorview_show() {
  if (sensorScreen)
    lv_scr_load_anim(sensorScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void sensorview_hide() {}

void sensorview_update(float heading, float pitch, float roll, float temp,
                       float alt, float pressure, int16_t mx, int16_t my,
                       int16_t mz, float accelX, float accelY, float accelZ,
                       float gyroX, float gyroY, float gyroZ) {
  char buf[48];

  snprintf(buf, sizeof(buf), "H:%.0f P:%.1f R:%.1f", heading, pitch, roll);
  lv_label_set_text(headingLabel, buf);
  lv_obj_align(headingLabel, LV_ALIGN_CENTER, 0, -45);

  snprintf(buf, sizeof(buf), "A:%.2f %.2f %.2f\nG:%.1f %.1f %.1f", accelX,
           accelY, accelZ, gyroX, gyroY, gyroZ);
  lv_label_set_text(imuLabel, buf);
  lv_obj_align(imuLabel, LV_ALIGN_CENTER, 0, -15);

  snprintf(buf, sizeof(buf), "M: %d  %d  %d", mx, my, mz);
  lv_label_set_text(magLabel, buf);
  lv_obj_align(magLabel, LV_ALIGN_CENTER, 0, 15);

  snprintf(buf, sizeof(buf), "T:%.1f A:%.0f P:%.0f", temp, alt, pressure);
  lv_label_set_text(envLabel, buf);
  lv_obj_align(envLabel, LV_ALIGN_CENTER, 0, 40);
}
