#include "alttemp.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *alttempScreen = NULL;
static lv_obj_t *tempLabel = NULL;
static lv_obj_t *altLabel = NULL;
static lv_obj_t *tempMercury = NULL;
static lv_obj_t *tempUnitLabel = NULL;

static float lastTemp = -999;
static float lastAlt = -999;

LV_IMG_DECLARE(img_thermometer);
LV_IMG_DECLARE(img_mountain);

void alttemp_init() {
  alttempScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(alttempScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(alttempScreen, LV_OBJ_FLAG_SCROLLABLE);

  // ===== Temperature Section (top half) =====
  // Thermometer tube outline — centered
  lv_obj_t *tubeOutline = lv_obj_create(alttempScreen);
  lv_obj_set_size(tubeOutline, 12, 36);
  lv_obj_align(tubeOutline, LV_ALIGN_CENTER, -60, -35);
  lv_obj_set_style_radius(tubeOutline, 6, 0);
  lv_obj_set_style_bg_opa(tubeOutline, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(tubeOutline, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_style_border_width(tubeOutline, 2, 0);

  // Thermometer bulb outline
  lv_obj_t *bulbOutline = lv_obj_create(alttempScreen);
  lv_obj_set_size(bulbOutline, 20, 20);
  lv_obj_align(bulbOutline, LV_ALIGN_CENTER, -60, -6);
  lv_obj_set_style_radius(bulbOutline, 10, 0);
  lv_obj_set_style_bg_opa(bulbOutline, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(bulbOutline, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_style_border_width(bulbOutline, 2, 0);

  // Mercury fill inside bulb
  lv_obj_t *bulbFill = lv_obj_create(alttempScreen);
  lv_obj_set_size(bulbFill, 12, 12);
  lv_obj_align(bulbFill, LV_ALIGN_CENTER, -60, -6);
  lv_obj_set_style_radius(bulbFill, 6, 0);
  lv_obj_set_style_bg_color(bulbFill, lv_color_hex(0xFF3333), 0);
  lv_obj_set_style_border_width(bulbFill, 0, 0);

  // Mercury column
  tempMercury = lv_obj_create(alttempScreen);
  lv_obj_set_size(tempMercury, 5, 20);
  lv_obj_align(tempMercury, LV_ALIGN_CENTER, -60, -30);
  lv_obj_set_style_radius(tempMercury, 2, 0);
  lv_obj_set_style_bg_color(tempMercury, lv_color_hex(0xFF3333), 0);
  lv_obj_set_style_border_width(tempMercury, 0, 0);

  // Temperature value - centered
  tempLabel = lv_label_create(alttempScreen);
  lv_label_set_text(tempLabel, "--.-");
  lv_obj_set_style_text_font(tempLabel, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(tempLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(tempLabel, LV_ALIGN_CENTER, 15, -30);

  tempUnitLabel = lv_label_create(alttempScreen);
  lv_label_set_text(tempUnitLabel, "°C");
  lv_obj_set_style_text_font(tempUnitLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(tempUnitLabel, lv_color_hex(0x888888), 0);
  lv_obj_align(tempUnitLabel, LV_ALIGN_CENTER, 65, -35);

  // Divider line - centered
  lv_obj_t *divider = lv_obj_create(alttempScreen);
  lv_obj_set_size(divider, 140, 1);
  lv_obj_align(divider, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_color(divider, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_width(divider, 0, 0);

  // ===== Altitude Section (bottom half) =====
  // Mountain icon - centered
  lv_obj_t *mt1 = lv_line_create(alttempScreen);
  static lv_point_t mt1_pts[3] = {{0, 25}, {18, 0}, {36, 25}};
  lv_line_set_points(mt1, mt1_pts, 3);
  lv_obj_set_style_line_color(mt1, lv_color_hex(0x55AA55), 0);
  lv_obj_set_style_line_width(mt1, 2, 0);
  lv_obj_align(mt1, LV_ALIGN_CENTER, -55, 35);

  // Snow cap
  lv_obj_t *snow = lv_line_create(alttempScreen);
  static lv_point_t snow_pts[3] = {{12, 6}, {18, 0}, {24, 6}};
  lv_line_set_points(snow, snow_pts, 3);
  lv_obj_set_style_line_color(snow, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_line_width(snow, 2, 0);
  lv_obj_align(snow, LV_ALIGN_CENTER, -55, 35);

  // Altitude value - centered
  altLabel = lv_label_create(alttempScreen);
  lv_label_set_text(altLabel, "---- m");
  lv_obj_set_style_text_font(altLabel, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(altLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(altLabel, LV_ALIGN_CENTER, 15, 35);
}

void alttemp_show() {
  if (alttempScreen)
    lv_scr_load_anim(alttempScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0,
                     false);
}

void alttemp_hide() {}

void alttemp_update(float temperature, float altitude) {
  bool tempChanged = fabs(temperature - lastTemp) >= 0.1;
  bool altChanged = fabs(altitude - lastAlt) >= 1.0;

  if (!tempChanged && !altChanged)
    return;

  if (tempChanged) {
    lastTemp = temperature;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", temperature);
    lv_label_set_text(tempLabel, buf);
    lv_obj_align(tempLabel, LV_ALIGN_CENTER, 15, -30);

    // Animate mercury height
    int mercH = constrain((int)(temperature * 0.6f), 2, 28);
    lv_obj_set_size(tempMercury, 5, mercH);
    lv_obj_align(tempMercury, LV_ALIGN_CENTER, -60, -16 - mercH / 2);
  }

  if (altChanged) {
    lastAlt = altitude;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f ft", altitude * 3.28084f);
    lv_label_set_text(altLabel, buf);
    lv_obj_align(altLabel, LV_ALIGN_CENTER, 15, 35);
  }
}
