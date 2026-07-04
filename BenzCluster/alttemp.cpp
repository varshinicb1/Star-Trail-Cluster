#include "alttemp.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// Minimal Alt/Temp widget: pure-black screen, two rows.
//   Row 1: mountain glyph (left) + altitude value/unit (beside it)
//   Row 2: thermometer glyph (left) + temperature value/unit (beside it)
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

// Layout: icon column on the left, value+caption stacked beside it. Rows are
// horizontally centered as a unit but each row is internally icon-left.
#define ROW1_ICON_X 32
#define ROW1_ICON_Y 79
#define ROW1_TEXT_X 92
#define ROW1_VALUE_Y 62
#define ROW1_UNIT_Y 106

#define DIVIDER_Y 128

#define ROW2_ICON_X 38
#define ROW2_ICON_Y 161
#define ROW2_TEXT_X 92
#define ROW2_VALUE_Y 154
#define ROW2_UNIT_Y 198

void alttemp_init() {
  alttempScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(alttempScreen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(alttempScreen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(alttempScreen, LV_OBJ_FLAG_SCROLLABLE);

  // --- Row 1: Altitude (mountain icon left, value+unit beside it) ---
  mountainPts[0].x = 0;  mountainPts[0].y = 30;
  mountainPts[1].x = 14; mountainPts[1].y = 4;
  mountainPts[2].x = 24; mountainPts[2].y = 16;
  mountainPts[3].x = 34; mountainPts[3].y = 0;
  mountainPts[4].x = 44; mountainPts[4].y = 30;
  altIcon = lv_line_create(alttempScreen);
  lv_line_set_points(altIcon, mountainPts, 5);
  lv_obj_set_style_line_color(altIcon, lv_color_white(), 0);
  lv_obj_set_style_line_width(altIcon, 3, 0);
  lv_obj_set_style_line_rounded(altIcon, true, 0);
  lv_obj_set_pos(altIcon, ROW1_ICON_X, ROW1_ICON_Y);

  altValue = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(altValue, lv_color_white(), 0);
  lv_obj_set_style_text_font(altValue, &lv_font_montserrat_34, 0);
  lv_label_set_text(altValue, "0");
  lv_obj_set_pos(altValue, ROW1_TEXT_X, ROW1_VALUE_Y);

  altUnit = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(altUnit, lv_color_hex(0x777777), 0);
  lv_obj_set_style_text_font(altUnit, &lv_font_montserrat_14, 0);
  lv_label_set_text(altUnit, "ALTITUDE ft");
  lv_obj_set_pos(altUnit, ROW1_TEXT_X, ROW1_UNIT_Y);

  // Thin divider
  lv_obj_t *div = lv_obj_create(alttempScreen);
  lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(div, 0, 0);
  lv_obj_set_style_radius(div, 0, 0);
  lv_obj_set_style_bg_color(div, lv_color_hex(0x2A2A2A), 0);
  lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
  lv_obj_set_size(div, 170, 1);
  lv_obj_align(div, LV_ALIGN_TOP_MID, 0, DIVIDER_Y);

  // --- Row 2: Temperature (thermometer icon left, value+unit beside it) ---
  tmpIconStem = lv_obj_create(alttempScreen);
  lv_obj_clear_flag(tmpIconStem, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_color(tmpIconStem, lv_color_white(), 0);
  lv_obj_set_style_border_width(tmpIconStem, 3, 0);
  lv_obj_set_style_radius(tmpIconStem, 5, 0);
  lv_obj_set_style_bg_opa(tmpIconStem, LV_OPA_TRANSP, 0);
  lv_obj_set_size(tmpIconStem, 10, 34);
  lv_obj_set_pos(tmpIconStem, ROW2_ICON_X + 5, ROW2_ICON_Y);

  tmpIconBulb = lv_obj_create(alttempScreen);
  lv_obj_clear_flag(tmpIconBulb, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(tmpIconBulb, 0, 0);
  lv_obj_set_style_radius(tmpIconBulb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(tmpIconBulb, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(tmpIconBulb, LV_OPA_COVER, 0);
  lv_obj_set_size(tmpIconBulb, 18, 18);
  lv_obj_set_pos(tmpIconBulb, ROW2_ICON_X, ROW2_ICON_Y + 30);

  tempValue = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(tempValue, lv_color_white(), 0);
  lv_obj_set_style_text_font(tempValue, &lv_font_montserrat_34, 0);
  lv_label_set_text(tempValue, "0.0");
  lv_obj_set_pos(tempValue, ROW2_TEXT_X, ROW2_VALUE_Y);

  tempUnit = lv_label_create(alttempScreen);
  lv_obj_set_style_text_color(tempUnit, lv_color_hex(0x777777), 0);
  lv_obj_set_style_text_font(tempUnit, &lv_font_montserrat_14, 0);
  lv_label_set_text(tempUnit, "TEMP C");
  lv_obj_set_pos(tempUnit, ROW2_TEXT_X, ROW2_UNIT_Y);

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
    lv_obj_set_pos(altValue, ROW1_TEXT_X, ROW1_VALUE_Y);
  }
  if (fabsf(temperature - lastTemp) > 0.05f) {
    lastTemp = temperature;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", temperature);
    lv_label_set_text(tempValue, buf);
    lv_obj_set_pos(tempValue, ROW2_TEXT_X, ROW2_VALUE_Y);
  }
}
