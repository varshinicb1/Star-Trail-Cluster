#include "alttemp.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *alttempScreen = NULL;

// Altimeter dial
#define NUM_ALT_TICKS 100
static lv_obj_t *altTicks[NUM_ALT_TICKS];
static lv_obj_t *altNums[10];
static lv_obj_t *handLong = NULL;
static lv_obj_t *handShort = NULL;
static lv_obj_t *handHub = NULL;
static lv_obj_t *altLabel = NULL;
static lv_obj_t *tempLabel = NULL;
static lv_obj_t *x1000Label = NULL;
static lv_obj_t *x1000Frame = NULL;

// Bezel
static lv_obj_t *outerRing = NULL;
static lv_obj_t *innerRing = NULL;

static float lastAltFt = -999;
static float lastTempC = -999;

#define CX 120
#define CY 110

static void posOnCircle(int degOffset, float radius, int *x, int *y) {
  float rad = (degOffset - 90) * M_PI / 180.0f;
  *x = CX + (int)(cosf(rad) * radius);
  *y = CY + (int)(sinf(rad) * radius);
}

void alttemp_init() {
  alttempScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(alttempScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(alttempScreen, LV_OBJ_FLAG_SCROLLABLE);

  // === Bezel rings ===
  outerRing = lv_obj_create(alttempScreen);
  lv_obj_set_size(outerRing, 234, 234);
  lv_obj_align(outerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(outerRing, 117, 0);
  lv_obj_set_style_bg_color(outerRing, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_border_color(outerRing, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(outerRing, 1, 0);
  lv_obj_clear_flag(outerRing, LV_OBJ_FLAG_SCROLLABLE);

  innerRing = lv_obj_create(alttempScreen);
  lv_obj_set_size(innerRing, 222, 222);
  lv_obj_align(innerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(innerRing, 111, 0);
  lv_obj_set_style_bg_color(innerRing, lv_color_hex(0x191C1E), 0);
  lv_obj_set_style_border_color(innerRing, lv_color_hex(0x0F0F0F), 0);
  lv_obj_set_style_border_width(innerRing, 1, 0);
  lv_obj_clear_flag(innerRing, LV_OBJ_FLAG_SCROLLABLE);

  // Inner dark circle background
  lv_obj_t *dialBg = lv_obj_create(alttempScreen);
  lv_obj_set_size(dialBg, 210, 210);
  lv_obj_align(dialBg, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(dialBg, 105, 0);
  lv_obj_set_style_bg_color(dialBg, lv_color_hex(0x191C1E), 0);
  lv_obj_set_style_border_width(dialBg, 0, 0);
  lv_obj_set_style_bg_opa(dialBg, LV_OPA_COVER, 0);
  lv_obj_clear_flag(dialBg, LV_OBJ_FLAG_SCROLLABLE);

  // Inner lighter circle (miercemk-style)
  lv_obj_t *innerBg = lv_obj_create(alttempScreen);
  lv_obj_set_size(innerBg, 110, 110);
  lv_obj_align(innerBg, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(innerBg, 55, 0);
  lv_obj_set_style_bg_color(innerBg, lv_color_hex(0x24282A), 0);
  lv_obj_set_style_border_width(innerBg, 0, 0);
  lv_obj_clear_flag(innerBg, LV_OBJ_FLAG_SCROLLABLE);

  // === Ticks (100 ticks at 3.6°, major every 10th) ===
  int tickAngles[NUM_ALT_TICKS];
  for (int i = 0; i < NUM_ALT_TICKS; i++) {
    tickAngles[i] = i * 3.6f;
    altTicks[i] = lv_obj_create(alttempScreen);
    lv_obj_set_style_bg_color(altTicks[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(altTicks[i], 0, 0);
    lv_obj_set_style_radius(altTicks[i], 0, 0);
    bool major = (i % 10 == 0);
    lv_obj_set_size(altTicks[i], major ? 2 : 1, major ? 10 : 6);
  }

  // === Numbers 0-9 ===
  for (int n = 0; n < 10; n++) {
    altNums[n] = lv_label_create(alttempScreen);
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", n);
    lv_label_set_text(altNums[n], buf);
    lv_obj_set_style_text_font(altNums[n], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(altNums[n], lv_color_hex(0xFFFFFF), 0);
  }

  // === "ALT" text (miercemk-style) ===
  lv_obj_t *altText = lv_label_create(alttempScreen);
  lv_label_set_text(altText, "ALT");
  lv_obj_set_style_text_font(altText, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(altText, lv_color_hex(0x888888), 0);
  lv_obj_align(altText, LV_ALIGN_CENTER, 0, -55);

  // === "x1000 ft" reference window (bottom of dial like code.c) ===
  x1000Frame = lv_obj_create(alttempScreen);
  lv_obj_set_size(x1000Frame, 40, 16);
  lv_obj_align(x1000Frame, LV_ALIGN_CENTER, 0, 70);
  lv_obj_set_style_bg_color(x1000Frame, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_color(x1000Frame, lv_color_hex(0x555555), 0);
  lv_obj_set_style_border_width(x1000Frame, 1, 0);
  lv_obj_set_style_radius(x1000Frame, 2, 0);
  lv_obj_clear_flag(x1000Frame, LV_OBJ_FLAG_SCROLLABLE);

  x1000Label = lv_label_create(x1000Frame);
  lv_label_set_text(x1000Label, "x1000");
  lv_obj_set_style_text_font(x1000Label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(x1000Label, lv_color_hex(0x888888), 0);
  lv_obj_center(x1000Label);

  // === Altimeter hands ===
  // Long thin hand (1000ft per rotation)
  handLong = lv_obj_create(alttempScreen);
  lv_obj_set_size(handLong, 3, 82);
  lv_obj_set_style_bg_color(handLong, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(handLong, 0, 0);
  lv_obj_set_style_radius(handLong, 1, 0);
  lv_obj_set_style_transform_pivot_x(handLong, 1, 0);
  lv_obj_set_style_transform_pivot_y(handLong, 0, 0);

  // Short thick hand (10000ft per rotation)
  handShort = lv_obj_create(alttempScreen);
  lv_obj_set_size(handShort, 6, 55);
  lv_obj_set_style_bg_color(handShort, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(handShort, 0, 0);
  lv_obj_set_style_radius(handShort, 2, 0);
  lv_obj_set_style_transform_pivot_x(handShort, 3, 0);
  lv_obj_set_style_transform_pivot_y(handShort, 0, 0);

  // Hub cap
  handHub = lv_obj_create(alttempScreen);
  lv_obj_set_size(handHub, 12, 12);
  lv_obj_align(handHub, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(handHub, 6, 0);
  lv_obj_set_style_bg_color(handHub, lv_color_hex(0x888888), 0);
  lv_obj_set_style_border_color(handHub, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(handHub, 1, 0);

  // Inner white dot
  lv_obj_t *hubDot = lv_obj_create(alttempScreen);
  lv_obj_set_size(hubDot, 6, 6);
  lv_obj_align(hubDot, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(hubDot, 3, 0);
  lv_obj_set_style_bg_color(hubDot, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(hubDot, 0, 0);

  // === Altitude digital readout (bottom center) ===
  altLabel = lv_label_create(alttempScreen);
  lv_label_set_text(altLabel, "---- ft");
  lv_obj_set_style_text_font(altLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(altLabel, lv_color_hex(0x00FFAA), 0);
  lv_obj_align(altLabel, LV_ALIGN_CENTER, 0, 90);

  // === Temperature readout (bottom) ===
  tempLabel = lv_label_create(alttempScreen);
  lv_label_set_text(tempLabel, "--.- \xC2\xB0C");
  lv_obj_set_style_text_font(tempLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(tempLabel, lv_color_hex(0xFF6644), 0);
  lv_obj_align(tempLabel, LV_ALIGN_CENTER, 0, 105);
}

void alttemp_show() {
  if (alttempScreen)
    lv_scr_load_anim(alttempScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void alttemp_hide() {}

void alttemp_update(float temperature, float altitude) {
  float altFt = altitude * 3.28084f;
  float tempC = temperature;

  bool altChanged = fabs(altFt - lastAltFt) >= 1.0f;
  bool tempChanged = fabs(tempC - lastTempC) >= 0.1f;

  if (!altChanged && !tempChanged) return;

  if (altChanged) {
    lastAltFt = altFt;
    int altInt = (int)(altFt + 0.5f);

    // Digital readout
    char buf[16];
    snprintf(buf, sizeof(buf), "%d ft", altInt);
    lv_label_set_text(altLabel, buf);
    lv_obj_align(altLabel, LV_ALIGN_CENTER, 0, 90);

    // Long hand: 1000ft per rotation
    float longAngle = ((altInt % 1000) / 1000.0f) * 360.0f;
    lv_obj_align(handLong, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_transform_angle(handLong, (int16_t)(longAngle * 10), 0);

    // Short hand: 10000ft per rotation
    float shortAngle = ((altInt % 10000) / 10000.0f) * 360.0f;
    lv_obj_align(handShort, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_transform_angle(handShort, (int16_t)(shortAngle * 10), 0);
  }

  if (tempChanged) {
    lastTempC = tempC;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f \xC2\xB0C", tempC);
    lv_label_set_text(tempLabel, buf);
    lv_obj_align(tempLabel, LV_ALIGN_CENTER, 0, 105);
  }

  // Position ticks (100 ticks at 3.6, major every 10th)
  for (int i = 0; i < NUM_ALT_TICKS; i++) {
    int deg = (int)(i * 3.6f);
    int x, y;
    posOnCircle(deg, 98, &x, &y);
    bool major = (i % 10 == 0);
    int h = major ? 10 : 6;
    lv_obj_set_pos(altTicks[i], x - (major ? 1 : 0), y - h / 2);
  }

  // Position numbers 0-9 at every 36 (every 10th tick)
  for (int n = 0; n < 10; n++) {
    int deg = n * 36;
    int x, y;
    posOnCircle(deg, 72, &x, &y);
    lv_obj_set_pos(altNums[n], x - 7, y - 8);
  }
}
