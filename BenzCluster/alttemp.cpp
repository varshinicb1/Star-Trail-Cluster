#include "alttemp.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// Minimal Alt/Temp widget: pure-black screen.
//   Top half:    mountain/altitude glyph + altitude value (ft)
//   Bottom half: thermometer glyph + temperature (degC)
// White values, muted captions, thin divider. No dials, no colour fills.
// ============================================================================

static lv_obj_t *alttempScreen = NULL;
static lv_obj_t *altValue = NULL;
static lv_obj_t *altUnit = NULL;
static lv_obj_t *tempValue = NULL;
static lv_obj_t *tempUnit = NULL;
static lv_obj_t *altIcon = NULL;   // drawn mountain (lv_line)
static lv_obj_t *tmpIconBulb = NULL;
static lv_obj_t *tmpIconStem = NULL;

static float lastAlt = -99999, lastTemp = -999;

// Mountain glyph points (small, white outline)
static lv_point_t mountainPts[5];

void alttemp_init() {
  alttempScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(alttempScreen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(alttempScreen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(alttempScreen, LV_OBJ_FLAG_SCROLLABLE);

  // --- Altitude (top half) ---
  // Mountain glyph: two peaks
  mountainPts[0].x = 0;  mountainPts[0].y = 22;
  mountainPts[1].x = 12; mountainPts[1].y = 4;
  mountainPts[2].x = 20; mountainPts[2].y = 14;
  mountainPts[3].x = 28; mountainPts[3].y = 0;
  mountainPts[4].x = 44; mountainPts[4].y = 22;
  altIcon = lv_line_create(alttempScreen);
  lv_line_set_points(altIcon, mountainPts, 5);
  lv_obj_set_style_line_color(altIcon, lv_color_white(), 0);
  lv_obj_set_style_line_width(altIcon, 3, 0);
  lv_obj_set_style_line_rounded(altIcon, true, 0);
  lv_obj_set_pos(altIcon, 98, 32);

  altValue = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(altValue, lv_color_white(), 0);
  lv_obj_set_style_text_font(altValue, &lv_font_montserrat_40, 0);
  lv_label_set_text(altValue, "0");
  lv_obj_align(altValue, LV_ALIGN_TOP_MID, 0, 62);

  altUnit = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(altUnit, lv_color_hex(0x777777), 0);
  lv_obj_set_style_text_font(altUnit, &lv_font_montserrat_14, 0);
  lv_label_set_text(altUnit, "ALTITUDE ft");
  lv_obj_align(altUnit, LV_ALIGN_TOP_MID, 0, 102);

  // Thin divider
  lv_obj_t *div = lv_obj_create(alttempScreen);
  lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(div, 0, 0);
  lv_obj_set_style_radius(div, 0, 0);
  lv_obj_set_style_bg_color(div, lv_color_hex(0x2A2A2A), 0);
  lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
  lv_obj_set_size(div, 120, 1);
  lv_obj_align(div, LV_ALIGN_CENTER, 0, 0);

  // --- Temperature (bottom half) ---
  // Thermometer glyph: rounded stem + filled bulb
  tmpIconStem = lv_obj_create(alttempScreen);
  lv_obj_clear_flag(tmpIconStem, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_color(tmpIconStem, lv_color_white(), 0);
  lv_obj_set_style_border_width(tmpIconStem, 3, 0);
  lv_obj_set_style_radius(tmpIconStem, 5, 0);
  lv_obj_set_style_bg_opa(tmpIconStem, LV_OPA_TRANSP, 0);
  lv_obj_set_size(tmpIconStem, 12, 26);
  lv_obj_set_pos(tmpIconStem, 114, 128);

  tmpIconBulb = lv_obj_create(alttempScreen);
  lv_obj_clear_flag(tmpIconBulb, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(tmpIconBulb, 0, 0);
  lv_obj_set_style_radius(tmpIconBulb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(tmpIconBulb, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(tmpIconBulb, LV_OPA_COVER, 0);
  lv_obj_set_size(tmpIconBulb, 16, 16);
  lv_obj_set_pos(tmpIconBulb, 112, 148);

  tempValue = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(tempValue, lv_color_white(), 0);
  lv_obj_set_style_text_font(tempValue, &lv_font_montserrat_40, 0);
  lv_label_set_text(tempValue, "0.0");
  lv_obj_align(tempValue, LV_ALIGN_TOP_MID, 0, 166);

  tempUnit = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(tempUnit, lv_color_hex(0x777777), 0);
  lv_obj_set_style_text_font(tempUnit, &lv_font_montserrat_14, 0);
  lv_label_set_text(tempUnit, "TEMP C");
  lv_obj_align(tempUnit, LV_ALIGN_TOP_MID, 0, 206);

  alttemp_update(25.0f, LOCAL_ELEVATION_FEET);
}

void alttemp_show() {
  if (alttempScreen)
    lv_scr_load_anim(alttempScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void alttemp_hide() {}

void alttemp_update(float temperature, float altitude) {
  if (fabsf(altitude - lastAlt) > 0.5f) {
    lastAlt = altitude;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(altitude));
    lv_label_set_text(altValue, buf);
    lv_obj_align(altValue, LV_ALIGN_TOP_MID, 0, 62);
  }
  if (fabsf(temperature - lastTemp) > 0.05f) {
    lastTemp = temperature;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", temperature);
    lv_label_set_text(tempValue, buf);
    lv_obj_align(tempValue, LV_ALIGN_TOP_MID, 0, 166);
  }
}
