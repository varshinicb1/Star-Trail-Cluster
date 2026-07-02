#include "compass.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// Linear scrolling compass (car-dashboard style).
//   Pure-black background, a horizontal WHITE scale that scrolls, a fixed RED
//   needle always at centre, and the WHITE heading value below.
//   Ticks every 15 deg (minor), labelled cardinals/intercardinals every 45 deg.
//   The whole 0..360 wraps seamlessly through centre.
// ============================================================================

#define CX 120
#define BASE_Y 96           // baseline (top) of the tick band
#define PX_PER_DEG 2.6f     // ~92 deg visible across the round face
#define HALF_VIS 118        // clip ticks beyond this from centre (round bezel)

#define N_TICKS 24          // one every 15 deg
#define N_LABELS 8          // one every 45 deg

static lv_obj_t *compassScreen = NULL;
static lv_obj_t *needle = NULL;
static lv_obj_t *ticks[N_TICKS];
static lv_obj_t *labels[N_LABELS];
static lv_obj_t *headingLabel = NULL;
static lv_obj_t *degLabel = NULL;

static const char *cardinalNames[N_LABELS] = {"N", "NE", "E", "SE",
                                              "S", "SW", "W", "NW"};

// Shortest signed angular difference a-b in (-180,180].
static float angDiff(float a, float b) {
  float d = a - b;
  while (d > 180.0f) d -= 360.0f;
  while (d <= -180.0f) d += 360.0f;
  return d;
}

void compass_init() {
  compassScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(compassScreen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(compassScreen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(compassScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Minor/major tick pool (white bars, repositioned each frame).
  for (int i = 0; i < N_TICKS; i++) {
    ticks[i] = lv_obj_create(compassScreen);
    lv_obj_clear_flag(ticks[i], LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(ticks[i], 0, 0);
    lv_obj_set_style_radius(ticks[i], 0, 0);
    lv_obj_set_style_bg_color(ticks[i], lv_color_white(), 0);
    lv_obj_set_style_bg_opa(ticks[i], LV_OPA_COVER, 0);
    lv_obj_set_size(ticks[i], 2, 12);
  }

  // Cardinal/intercardinal labels.
  for (int i = 0; i < N_LABELS; i++) {
    labels[i] = lv_label_create(compassScreen);
    lv_obj_set_style_text_color(labels[i], lv_color_white(), 0);
    lv_obj_set_style_text_font(labels[i], &lv_font_montserrat_18, 0);
    lv_label_set_text(labels[i], cardinalNames[i]);
  }

  // Fixed red centre needle.
  needle = lv_obj_create(compassScreen);
  lv_obj_clear_flag(needle, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(needle, 0, 0);
  lv_obj_set_style_radius(needle, 1, 0);
  lv_obj_set_style_bg_color(needle, lv_color_hex(0xE01020), 0);  // red
  lv_obj_set_style_bg_opa(needle, LV_OPA_COVER, 0);
  lv_obj_set_size(needle, 3, 40);
  lv_obj_set_pos(needle, CX - 1, BASE_Y - 6);

  // Big white heading value below the scale.
  headingLabel = lv_label_create(compassScreen);
  lv_obj_set_style_text_color(headingLabel, lv_color_white(), 0);
  lv_obj_set_style_text_font(headingLabel, &lv_font_montserrat_44, 0);
  lv_label_set_text(headingLabel, "0");
  lv_obj_align(headingLabel, LV_ALIGN_TOP_MID, 6, 150);

  // Heading caption.
  degLabel = lv_label_create(compassScreen);
  lv_obj_set_style_text_color(degLabel, lv_color_hex(0x777777), 0);
  lv_obj_set_style_text_font(degLabel, &lv_font_montserrat_14, 0);
  lv_label_set_text(degLabel, "HEADING");
  lv_obj_align(degLabel, LV_ALIGN_TOP_MID, 0, 205);

  compass_update(0);
}

void compass_show() {
  if (compassScreen)
    lv_scr_load_anim(compassScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void compass_hide() {}

void compass_update(float heading_in) {
  if (heading_in < 0) heading_in += 360.0f;
  if (heading_in >= 360.0f) heading_in -= 360.0f;

  // Position each 15-deg tick relative to centre.
  for (int i = 0; i < N_TICKS; i++) {
    float tickHdg = i * 15.0f;
    float d = angDiff(tickHdg, heading_in);       // -180..180
    int x = CX + (int)lroundf(d * PX_PER_DEG);
    bool major = (i % 3 == 0);                     // every 45 deg
    if (abs(x - CX) > HALF_VIS) {
      lv_obj_add_flag(ticks[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(ticks[i], LV_OBJ_FLAG_HIDDEN);
      int h = major ? 22 : 12;
      lv_obj_set_size(ticks[i], major ? 3 : 2, h);
      lv_obj_set_pos(ticks[i], x - 1, BASE_Y);
    }
  }

  // Labels sit under their major ticks.
  for (int i = 0; i < N_LABELS; i++) {
    float lblHdg = i * 45.0f;
    float d = angDiff(lblHdg, heading_in);
    int x = CX + (int)lroundf(d * PX_PER_DEG);
    if (abs(x - CX) > HALF_VIS) {
      lv_obj_add_flag(labels[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(labels[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_update_layout(labels[i]);
      int w = lv_obj_get_width(labels[i]);
      lv_obj_set_pos(labels[i], x - w / 2, BASE_Y + 26);
    }
  }

  // Heading number.
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(heading_in) % 360);
  lv_label_set_text(headingLabel, buf);
}
