#include "gforce.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *gforceScreen = NULL;
static lv_obj_t *gDot = NULL;
static lv_obj_t *peakDotF = NULL;
static lv_obj_t *peakDotR = NULL;
static lv_obj_t *latLabel = NULL;
static lv_obj_t *lonLabel = NULL;
static lv_obj_t *totalLabel = NULL;
static lv_obj_t *peakLabel = NULL;

// Ring objects for g-circles
#define NUM_RINGS 4
static lv_obj_t *rings[NUM_RINGS];

// Crosshair
static lv_obj_t *crossH = NULL;
static lv_obj_t *crossV = NULL;

static float peakG = 0;
static float peakLatG = 0;
static float peakLonG = 0;

// Scale: pixels per g
#define G_SCALE 80.0f
#define DOT_R 6

void gforce_init() {
  gforceScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(gforceScreen, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(gforceScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Title
  lv_obj_t *title = lv_label_create(gforceScreen);
  lv_label_set_text(title, "G-FORCE");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x777777), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

  // G-circle rings: 0.25g, 0.5g, 0.75g, 1.0g
  float gValues[] = {0.25f, 0.5f, 0.75f, 1.0f};
  uint32_t ringColors[] = {0x1A331A, 0x333300, 0x4D3300, 0x4D1A1A};
  for (int i = 0; i < NUM_RINGS; i++) {
    int r = (int)(gValues[i] * G_SCALE);
    rings[i] = lv_obj_create(gforceScreen);
    lv_obj_set_size(rings[i], r * 2, r * 2);
    lv_obj_align(rings[i], LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_style_radius(rings[i], r, 0);
    lv_obj_set_style_bg_opa(rings[i], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(rings[i], lv_color_hex(ringColors[i]), 0);
    lv_obj_set_style_border_width(rings[i], 1, 0);
    lv_obj_clear_flag(rings[i], LV_OBJ_FLAG_SCROLLABLE);
  }

  // G-value labels on rings
  const char *gLabels[] = {".25", ".5", ".75", "1g"};
  for (int i = 0; i < NUM_RINGS; i++) {
    lv_obj_t *lbl = lv_label_create(gforceScreen);
    lv_label_set_text(lbl, gLabels[i]);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x444444), 0);
    int r = (int)(gValues[i] * G_SCALE);
    lv_obj_align(lbl, LV_ALIGN_CENTER, r - 8, 5 - 6);
  }

  // Crosshair
  crossH = lv_obj_create(gforceScreen);
  lv_obj_set_size(crossH, 160, 1);
  lv_obj_align(crossH, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_color(crossH, lv_color_hex(0x222222), 0);
  lv_obj_set_style_border_width(crossH, 0, 0);

  crossV = lv_obj_create(gforceScreen);
  lv_obj_set_size(crossV, 1, 160);
  lv_obj_align(crossV, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_color(crossV, lv_color_hex(0x222222), 0);
  lv_obj_set_style_border_width(crossV, 0, 0);

  // Peak hold dots (small, dim)
  peakDotF = lv_obj_create(gforceScreen);
  lv_obj_set_size(peakDotF, 4, 4);
  lv_obj_align(peakDotF, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_color(peakDotF, lv_color_hex(0xE01020), 0);
  lv_obj_set_style_bg_opa(peakDotF, LV_OPA_50, 0);
  lv_obj_set_style_border_width(peakDotF, 0, 0);
  lv_obj_set_style_radius(peakDotF, 2, 0);
  lv_obj_add_flag(peakDotF, LV_OBJ_FLAG_HIDDEN);

  // Main G-dot (bright, shows current g)
  gDot = lv_obj_create(gforceScreen);
  lv_obj_set_size(gDot, DOT_R * 2, DOT_R * 2);
  lv_obj_align(gDot, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_color(gDot, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(gDot, 0, 0);
  lv_obj_set_style_radius(gDot, DOT_R, 0);
  lv_obj_set_style_shadow_color(gDot, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_shadow_width(gDot, 8, 0);
  lv_obj_set_style_shadow_spread(gDot, 2, 0);

  // Digital readouts
  latLabel = lv_label_create(gforceScreen);
  lv_label_set_text(latLabel, "LAT 0.00g");
  lv_obj_set_style_text_font(latLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(latLabel, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(latLabel, LV_ALIGN_CENTER, -40, 95);

  lonLabel = lv_label_create(gforceScreen);
  lv_label_set_text(lonLabel, "LON 0.00g");
  lv_obj_set_style_text_font(lonLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lonLabel, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(lonLabel, LV_ALIGN_CENTER, 40, 95);

  peakLabel = lv_label_create(gforceScreen);
  lv_label_set_text(peakLabel, "PEAK 0.00g");
  lv_obj_set_style_text_font(peakLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(peakLabel, lv_color_hex(0xE01020), 0);
  lv_obj_align(peakLabel, LV_ALIGN_CENTER, 0, 108);
}

void gforce_show() {
  if (gforceScreen)
    lv_scr_load_anim(gforceScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void gforce_hide() {}

void gforce_update(float ax, float ay, float az) {
  // ax = lateral (left/right), ay = longitudinal (forward/back)
  // az = vertical (up/down, ~1g when stationary)

  // Remove gravity from Z to get dynamic g
  float latG = ax;         // Lateral acceleration
  float lonG = ay;         // Longitudinal acceleration
  float vertG = az - 1.0f; // Vertical (subtract gravity)
  float totalG = sqrtf(latG * latG + lonG * lonG);

  // Clamp to display range
  float maxG = 1.2f;
  float clampLat = constrain(latG, -maxG, maxG);
  float clampLon = constrain(lonG, -maxG, maxG);

  // Map to pixel position
  int dotX = (int)(clampLat * G_SCALE);
  int dotY = (int)(-clampLon * G_SCALE); // Negative because screen Y is down
  lv_obj_align(gDot, LV_ALIGN_CENTER, dotX, 5 + dotY);

  // Color based on total g: green → yellow → red
  uint32_t dotColor;
  if (totalG < 0.3f)
    dotColor = 0x00FF66; // Green
  else if (totalG < 0.6f)
    dotColor = 0xFFFF00; // Yellow
  else if (totalG < 0.8f)
    dotColor = 0xFF8800; // Orange
  else
    dotColor = 0xFF2222; // Red
  lv_obj_set_style_bg_color(gDot, lv_color_hex(dotColor), 0);
  lv_obj_set_style_shadow_color(gDot, lv_color_hex(dotColor), 0);

  // Peak tracking
  if (totalG > peakG) {
    peakG = totalG;
    peakLatG = clampLat;
    peakLonG = clampLon;
    int peakPx = (int)(peakLatG * G_SCALE);
    int peakPy = (int)(-peakLonG * G_SCALE);
    lv_obj_align(peakDotF, LV_ALIGN_CENTER, peakPx, 5 + peakPy);
    lv_obj_clear_flag(peakDotF, LV_OBJ_FLAG_HIDDEN);
  }

  // Update labels
  char buf[20];
  snprintf(buf, sizeof(buf), "LAT %.2fg", fabs(latG));
  lv_label_set_text(latLabel, buf);
  lv_obj_align(latLabel, LV_ALIGN_CENTER, -40, 95);

  snprintf(buf, sizeof(buf), "LON %.2fg", fabs(lonG));
  lv_label_set_text(lonLabel, buf);
  lv_obj_align(lonLabel, LV_ALIGN_CENTER, 40, 95);

  snprintf(buf, sizeof(buf), "PEAK %.2fg", peakG);
  lv_label_set_text(peakLabel, buf);
  lv_obj_align(peakLabel, LV_ALIGN_CENTER, 0, 108);
}
