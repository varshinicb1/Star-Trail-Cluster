#include "compass.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *compassScreen = NULL;
static lv_obj_t *degreeLabel = NULL;
static lv_obj_t *cardinalLabel = NULL;

// Rotating ring objects
#define NUM_COMPASS_TICKS 36
#define NUM_COMPASS_LETTERS 4
#define NUM_COMPASS_NUMS 8
#define NUM_DOTS 36

static lv_obj_t *tickLines[NUM_COMPASS_TICKS];
static lv_obj_t *dotObjs[NUM_DOTS];
static lv_obj_t *letterLabels[NUM_COMPASS_LETTERS];
static lv_obj_t *numLabels[NUM_COMPASS_NUMS];

// Airplane symbol (static at center)
static lv_obj_t *planeBody = NULL;
static lv_obj_t *planeWingL = NULL;
static lv_obj_t *planeWingR = NULL;
static lv_obj_t *planeTail = NULL;
static lv_obj_t *planeCenter = NULL;

// Bezel rings
static lv_obj_t *outerRing = NULL;
static lv_obj_t *innerRing = NULL;

static float lastHeading = -999;
static float smoothHeading = 0;

#define CX 120
#define CY 120

static const char *cardinalNames[] = {"N", "E", "S", "W"};
static const int cardinalDegs[] = {0, 90, 180, 270};

static const int numDegs[] = {30, 60, 120, 150, 210, 240, 300, 330};
static const char *numTexts[] = {"3", "6", "12", "15", "21", "24", "30", "33"};

static void posOnCircle(int degOffset, float radius, int *x, int *y) {
  float rad = (degOffset - 90) * M_PI / 180.0f;
  *x = CX + (int)(cosf(rad) * radius);
  *y = CY + (int)(sinf(rad) * radius);
}

void compass_init() {
  compassScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(compassScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(compassScreen, LV_OBJ_FLAG_SCROLLABLE);

  // === Bezel rings (miercemk-style concentric) ===
  outerRing = lv_obj_create(compassScreen);
  lv_obj_set_size(outerRing, 234, 234);
  lv_obj_align(outerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(outerRing, 117, 0);
  lv_obj_set_style_bg_color(outerRing, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_border_color(outerRing, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(outerRing, 1, 0);
  lv_obj_clear_flag(outerRing, LV_OBJ_FLAG_SCROLLABLE);

  innerRing = lv_obj_create(compassScreen);
  lv_obj_set_size(innerRing, 222, 222);
  lv_obj_align(innerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(innerRing, 111, 0);
  lv_obj_set_style_bg_color(innerRing, lv_color_hex(0x2D3234), 0);
  lv_obj_set_style_border_color(innerRing, lv_color_hex(0x0F0F0F), 0);
  lv_obj_set_style_border_width(innerRing, 1, 0);
  lv_obj_clear_flag(innerRing, LV_OBJ_FLAG_SCROLLABLE);

  // === Tick marks (36 at 10 intervals) ===
  for (int i = 0; i < NUM_COMPASS_TICKS; i++) {
    tickLines[i] = lv_obj_create(compassScreen);
    lv_obj_set_style_bg_color(tickLines[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(tickLines[i], 0, 0);
    lv_obj_set_style_radius(tickLines[i], 0, 0);
    lv_obj_set_size(tickLines[i], 2, 10);
  }

  // === Dots at 5 intervals ===
  for (int i = 0; i < NUM_DOTS; i++) {
    dotObjs[i] = lv_obj_create(compassScreen);
    lv_obj_set_size(dotObjs[i], 3, 3);
    lv_obj_set_style_radius(dotObjs[i], 2, 0);
    lv_obj_set_style_bg_color(dotObjs[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(dotObjs[i], 0, 0);
    lv_obj_set_style_bg_opa(dotObjs[i], LV_OPA_50, 0);
  }

  // === Cardinal letters (N/E/S/W) ===
  for (int i = 0; i < NUM_COMPASS_LETTERS; i++) {
    letterLabels[i] = lv_label_create(compassScreen);
    lv_label_set_text(letterLabels[i], cardinalNames[i]);
    lv_obj_set_style_text_font(letterLabels[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(letterLabels[i], lv_color_hex(0xFFDD00), 0);
  }

  // === Degree numbers (30/60/120/150/210/240/300/330) ===
  for (int i = 0; i < NUM_COMPASS_NUMS; i++) {
    numLabels[i] = lv_label_create(compassScreen);
    lv_label_set_text(numLabels[i], numTexts[i]);
    lv_obj_set_style_text_font(numLabels[i], &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(numLabels[i], lv_color_hex(0xBBBBBB), 0);
  }

  // === Airplane symbol (miercemk-style, static at center) ===
  uint32_t acColor = 0xFFAA00;
  // Fuselage (vertical line)
  planeBody = lv_obj_create(compassScreen);
  lv_obj_set_size(planeBody, 4, 90);
  lv_obj_align(planeBody, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(planeBody, lv_color_hex(acColor), 0);
  lv_obj_set_style_border_width(planeBody, 0, 0);
  lv_obj_set_style_radius(planeBody, 2, 0);

  // Left wing
  planeWingL = lv_obj_create(compassScreen);
  lv_obj_set_size(planeWingL, 45, 4);
  lv_obj_align(planeWingL, LV_ALIGN_CENTER, -30, 12);
  lv_obj_set_style_bg_color(planeWingL, lv_color_hex(acColor), 0);
  lv_obj_set_style_border_width(planeWingL, 0, 0);
  lv_obj_set_style_radius(planeWingL, 2, 0);

  // Right wing
  planeWingR = lv_obj_create(compassScreen);
  lv_obj_set_size(planeWingR, 45, 4);
  lv_obj_align(planeWingR, LV_ALIGN_CENTER, 30, 12);
  lv_obj_set_style_bg_color(planeWingR, lv_color_hex(acColor), 0);
  lv_obj_set_style_border_width(planeWingR, 0, 0);
  lv_obj_set_style_radius(planeWingR, 2, 0);

  // Tail
  planeTail = lv_obj_create(compassScreen);
  lv_obj_set_size(planeTail, 3, 24);
  lv_obj_align(planeTail, LV_ALIGN_CENTER, 0, -42);
  lv_obj_set_style_bg_color(planeTail, lv_color_hex(acColor), 0);
  lv_obj_set_style_border_width(planeTail, 0, 0);
  lv_obj_set_style_radius(planeTail, 1, 0);
  lv_obj_set_style_bg_opa(planeTail, LV_OPA_60, 0);

  // Center dot
  planeCenter = lv_obj_create(compassScreen);
  lv_obj_set_size(planeCenter, 8, 8);
  lv_obj_align(planeCenter, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(planeCenter, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(planeCenter, 0, 0);
  lv_obj_set_style_radius(planeCenter, 4, 0);

  // === Heading degree label (top) ===
  degreeLabel = lv_label_create(compassScreen);
  lv_label_set_text(degreeLabel, "0\xC2\xB0");
  lv_obj_set_style_text_font(degreeLabel, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(degreeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(degreeLabel, LV_ALIGN_CENTER, 0, -80);

  // Cardinal label (below center)
  cardinalLabel = lv_label_create(compassScreen);
  lv_label_set_text(cardinalLabel, "N");
  lv_obj_set_style_text_font(cardinalLabel, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(cardinalLabel, lv_color_hex(0xFF4444), 0);
  lv_obj_align(cardinalLabel, LV_ALIGN_CENTER, 0, 75);
}

void compass_show() {
  if (compassScreen)
    lv_scr_load_anim(compassScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void compass_hide() {}

void compass_update(float heading_in) {
  float diff = heading_in - smoothHeading;
  if (diff > 180) diff -= 360;
  if (diff < -180) diff += 360;
  smoothHeading += diff * 0.25f;
  if (smoothHeading < 0) smoothHeading += 360;
  if (smoothHeading >= 360) smoothHeading -= 360;

  if (fabs(smoothHeading - lastHeading) < 0.2f) return;
  lastHeading = smoothHeading;

  // Update heading label
  char buf[16];
  snprintf(buf, sizeof(buf), "%d\xC2\xB0", (int)(smoothHeading + 0.5f));
  lv_label_set_text(degreeLabel, buf);
  lv_obj_align(degreeLabel, LV_ALIGN_CENTER, 0, -80);

  // Update cardinal label
  int h = (int)(smoothHeading + 0.5f) % 360;
  const char *card = "N";
  if (h < 22 || h >= 337) card = "N";
  else if (h < 67) card = "NE";
  else if (h < 112) card = "E";
  else if (h < 157) card = "SE";
  else if (h < 202) card = "S";
  else if (h < 247) card = "SW";
  else if (h < 292) card = "W";
  else card = "NW";
  lv_label_set_text(cardinalLabel, card);
  lv_obj_align(cardinalLabel, LV_ALIGN_CENTER, 0, 75);

  // Position ticks (rotate around center based on heading)
  for (int i = 0; i < NUM_COMPASS_TICKS; i++) {
    int deg = (i * 10) - (int)smoothHeading;
    int x, y;
    posOnCircle(deg, 102, &x, &y);
    lv_obj_set_pos(tickLines[i], x - 1, y - 5);
  }

  // Position dots at 5, 15, 25...
  for (int i = 0; i < NUM_DOTS; i++) {
    int deg = (i * 10 + 5) - (int)smoothHeading;
    int x, y;
    posOnCircle(deg, 98, &x, &y);
    lv_obj_set_pos(dotObjs[i], x - 1, y - 1);
  }

  // Position cardinal letters
  for (int i = 0; i < NUM_COMPASS_LETTERS; i++) {
    int deg = cardinalDegs[i] - (int)smoothHeading;
    int x, y;
    posOnCircle(deg, 82, &x, &y);
    lv_obj_set_pos(letterLabels[i], x - 8, y - 8);
  }

  // Position degree numbers
  for (int i = 0; i < NUM_COMPASS_NUMS; i++) {
    int deg = numDegs[i] - (int)smoothHeading;
    int x, y;
    posOnCircle(deg, 78, &x, &y);
    lv_obj_set_pos(numLabels[i], x - 6, y - 6);
  }
}
