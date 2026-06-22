#include "compass.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *compassScreen = NULL;
static lv_obj_t *rulerContainer = NULL;
static lv_obj_t *degreeLabel = NULL;
static lv_obj_t *cardinalLabel = NULL;
static lv_obj_t *centerNeedle = NULL;

#define VISIBLE_W SCREEN_WIDTH
#define TICK_SPACING 2

#define MAX_TICKS 60
static lv_obj_t *tickLines[MAX_TICKS];
static lv_obj_t *tickLabels[MAX_TICKS];
static int numTicks = 0;

static float lastHeading = -999;
static float smoothHeading = 0;

static const char *getCardinal(float h) {
  if (h < 0)
    h += 360;
  if (h >= 360)
    h -= 360;
  if (h < 22.5 || h >= 337.5)
    return "N";
  if (h < 67.5)
    return "NE";
  if (h < 112.5)
    return "E";
  if (h < 157.5)
    return "SE";
  if (h < 202.5)
    return "S";
  if (h < 247.5)
    return "SW";
  if (h < 292.5)
    return "W";
  return "NW";
}

static const char *getTickLabel(int deg) {
  deg = deg % 360;
  if (deg < 0)
    deg += 360;
  switch (deg) {
  case 0:
    return "N";
  case 45:
    return "NE";
  case 90:
    return "E";
  case 135:
    return "SE";
  case 180:
    return "S";
  case 225:
    return "SW";
  case 270:
    return "W";
  case 315:
    return "NW";
  default:
    return NULL;
  }
}

void compass_init() {
  compassScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(compassScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(compassScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Heading degree label (centered at top)
  degreeLabel = lv_label_create(compassScreen);
  lv_label_set_text(degreeLabel, "0\xC2\xB0");
  lv_obj_set_style_text_font(degreeLabel, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(degreeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(degreeLabel, LV_ALIGN_CENTER, 0, -55);

  // Ruler container — centered vertically
  rulerContainer = lv_obj_create(compassScreen);
  lv_obj_set_size(rulerContainer, VISIBLE_W, 70);
  lv_obj_align(rulerContainer, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_opa(rulerContainer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(rulerContainer, 0, 0);
  lv_obj_set_style_pad_all(rulerContainer, 0, 0);
  lv_obj_clear_flag(rulerContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_clip_corner(rulerContainer, true, 0);
  lv_obj_set_style_radius(rulerContainer, 0, 0);

  int maxVisible = VISIBLE_W / (TICK_SPACING * 5) + 4;
  numTicks = min(maxVisible, MAX_TICKS);

  for (int i = 0; i < numTicks; i++) {
    tickLines[i] = lv_obj_create(rulerContainer);
    lv_obj_set_style_bg_color(tickLines[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(tickLines[i], 0, 0);
    lv_obj_set_style_radius(tickLines[i], 0, 0);
    lv_obj_set_size(tickLines[i], 1, 12);

    tickLabels[i] = lv_label_create(rulerContainer);
    lv_label_set_text(tickLabels[i], "");
    lv_obj_set_style_text_font(tickLabels[i], &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(tickLabels[i], lv_color_hex(0xAAAAAA), 0);
  }

  // Red center needle
  centerNeedle = lv_obj_create(compassScreen);
  lv_obj_set_size(centerNeedle, 3, 80);
  lv_obj_align(centerNeedle, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_color(centerNeedle, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(centerNeedle, 0, 0);
  lv_obj_set_style_radius(centerNeedle, 1, 0);

  // Red dot pointer
  lv_obj_t *pointer = lv_obj_create(compassScreen);
  lv_obj_set_size(pointer, 7, 7);
  lv_obj_align(pointer, LV_ALIGN_CENTER, 0, -26);
  lv_obj_set_style_bg_color(pointer, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(pointer, 0, 0);
  lv_obj_set_style_radius(pointer, 4, 0);

  // Cardinal direction
  cardinalLabel = lv_label_create(compassScreen);
  lv_label_set_text(cardinalLabel, "N");
  lv_obj_set_style_text_font(cardinalLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(cardinalLabel, lv_color_hex(0xFF4444), 0);
  lv_obj_align(cardinalLabel, LV_ALIGN_CENTER, 0, 90);
}

void compass_show() {
  if (compassScreen)
    lv_scr_load_anim(compassScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0,
                     false);
}
void compass_hide() {}

void compass_update(float heading_in) {
  float diff = heading_in - smoothHeading;
  if (diff > 180)
    diff -= 360;
  if (diff < -180)
    diff += 360;
  smoothHeading += diff * 0.25f;
  if (smoothHeading < 0)
    smoothHeading += 360;
  if (smoothHeading >= 360)
    smoothHeading -= 360;

  if (fabs(smoothHeading - lastHeading) < 0.2f)
    return;
  lastHeading = smoothHeading;

  char buf[16];
  snprintf(buf, sizeof(buf), "%d°", (int)(smoothHeading + 0.5f));
  lv_label_set_text(degreeLabel, buf);
  lv_obj_align(degreeLabel, LV_ALIGN_CENTER, 0, -55);
  lv_label_set_text(cardinalLabel, getCardinal(smoothHeading));
  lv_obj_align(cardinalLabel, LV_ALIGN_CENTER, 0, 90);

  int halfW = VISIBLE_W / 2;
  int spanDeg = halfW / TICK_SPACING + 10;
  int startDeg = ((int)smoothHeading - spanDeg) / 5 * 5;
  int idx = 0;

  for (int deg = startDeg; idx < numTicks; deg += 5, idx++) {
    int normDeg = deg % 360;
    if (normDeg < 0)
      normDeg += 360;
    float dDeg = deg - smoothHeading;
    int xPos = halfW + (int)(dDeg * TICK_SPACING);

    if (xPos < -20 || xPos > VISIBLE_W + 20) {
      lv_obj_add_flag(tickLines[idx], LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(tickLabels[idx], LV_OBJ_FLAG_HIDDEN);
      continue;
    }

    lv_obj_clear_flag(tickLines[idx], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(tickLabels[idx], LV_OBJ_FLAG_HIDDEN);

    bool isCardinal = (normDeg % 90 == 0);
    bool isInterCard = (normDeg % 45 == 0) && !isCardinal;
    bool isMajor = (normDeg % 30 == 0);

    int tickH = isCardinal ? 30 : isInterCard ? 24 : isMajor ? 18 : 12;
    int tickW = isCardinal ? 3 : (normDeg % 15 == 0) ? 2 : 1;

    lv_obj_set_size(tickLines[idx], tickW, tickH);
    lv_obj_set_pos(tickLines[idx], xPos - tickW / 2, 35 - tickH / 2);

    if (normDeg == 0)
      lv_obj_set_style_bg_color(tickLines[idx], lv_color_hex(0xFF2222), 0);
    else if (isCardinal || isInterCard)
      lv_obj_set_style_bg_color(tickLines[idx], lv_color_hex(0xFFFFFF), 0);
    else
      lv_obj_set_style_bg_color(tickLines[idx], lv_color_hex(0x555555), 0);

    const char *label = getTickLabel(normDeg);
    if (label) {
      lv_label_set_text(tickLabels[idx], label);
      lv_obj_set_pos(tickLabels[idx], xPos - 5, 2);
      lv_obj_set_style_text_color(
          tickLabels[idx],
          normDeg == 0 ? lv_color_hex(0xFF4444) : lv_color_hex(0xBBBBBB), 0);
    } else if (normDeg % 30 == 0) {
      char degBuf[8];
      snprintf(degBuf, sizeof(degBuf), "%d", normDeg);
      lv_label_set_text(tickLabels[idx], degBuf);
      lv_obj_set_pos(tickLabels[idx], xPos - 7, 5);
      lv_obj_set_style_text_color(tickLabels[idx], lv_color_hex(0x777777), 0);
    } else {
      lv_label_set_text(tickLabels[idx], "");
    }
  }
  for (; idx < numTicks; idx++) {
    lv_obj_add_flag(tickLines[idx], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tickLabels[idx], LV_OBJ_FLAG_HIDDEN);
  }
}
