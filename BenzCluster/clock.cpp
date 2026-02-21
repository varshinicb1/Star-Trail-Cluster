#include "clock.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <time.h>

static lv_obj_t *clockScreen = NULL;

// Face 0: Digital (HH:MM with orbiting dot)
static lv_obj_t *timeLabel = NULL;
static lv_obj_t *secLabel = NULL;
static lv_obj_t *dateLabel = NULL;
static lv_obj_t *secHand = NULL;
#define NUM_DOTS 12
static lv_obj_t *hourDots[NUM_DOTS];

// Face 1: Analog (hour/minute hands)
static lv_obj_t *hourHand = NULL;
static lv_obj_t *minuteHand = NULL;
static lv_obj_t *analogSecDot = NULL;
static lv_obj_t *analogCenter = NULL;

// Face 2: Minimal (just large time, nothing else)
static lv_obj_t *minimalTime = NULL;
static lv_obj_t *minimalSec = NULL;

// Shared
static lv_obj_t *faceHint = NULL;
static lv_obj_t *outerRing = NULL;

static int currentFace = 0;
#define NUM_FACES 3
static int lastSec = -1;

static void hideAllFaces();
static void showFace(int face);
static void updateFace0(struct tm *t);
static void updateFace1(struct tm *t);
static void updateFace2(struct tm *t);

void clock_init() {
  clockScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(clockScreen, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(clockScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Outer ring
  outerRing = lv_obj_create(clockScreen);
  lv_obj_set_size(outerRing, 210, 210);
  lv_obj_align(outerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(outerRing, 105, 0);
  lv_obj_set_style_bg_opa(outerRing, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(outerRing, lv_color_hex(0x222222), 0);
  lv_obj_set_style_border_width(outerRing, 1, 0);

  // ===== Face 0: Digital =====
  for (int i = 0; i < NUM_DOTS; i++) {
    float angle = (i * 30.0f - 90.0f) * 3.14159f / 180.0f;
    int x = (int)(cos(angle) * 95);
    int y = (int)(sin(angle) * 95);
    hourDots[i] = lv_obj_create(clockScreen);
    int sz = (i % 3 == 0) ? 6 : 3;
    lv_obj_set_size(hourDots[i], sz, sz);
    lv_obj_align(hourDots[i], LV_ALIGN_CENTER, x, y);
    lv_obj_set_style_radius(hourDots[i], sz / 2, 0);
    lv_obj_set_style_bg_color(
        hourDots[i],
        (i % 3 == 0) ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(hourDots[i], 0, 0);
  }

  timeLabel = lv_label_create(clockScreen);
  lv_label_set_text(timeLabel, "00:00");
  lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(timeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_letter_space(timeLabel, 4, 0);
  lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -10);

  secLabel = lv_label_create(clockScreen);
  lv_label_set_text(secLabel, ":00");
  lv_obj_set_style_text_font(secLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(secLabel, lv_color_hex(0x555555), 0);
  lv_obj_align(secLabel, LV_ALIGN_CENTER, 0, 20);

  dateLabel = lv_label_create(clockScreen);
  lv_label_set_text(dateLabel, "--- -- ----");
  lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(dateLabel, lv_color_hex(0x333333), 0);
  lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 45);

  secHand = lv_obj_create(clockScreen);
  lv_obj_set_size(secHand, 5, 5);
  lv_obj_align(secHand, LV_ALIGN_CENTER, 0, -95);
  lv_obj_set_style_radius(secHand, 3, 0);
  lv_obj_set_style_bg_color(secHand, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(secHand, 0, 0);

  // ===== Face 1: Analog =====
  hourHand = lv_obj_create(clockScreen);
  lv_obj_set_size(hourHand, 4, 50);
  lv_obj_align(hourHand, LV_ALIGN_CENTER, 0, -25);
  lv_obj_set_style_bg_color(hourHand, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(hourHand, 0, 0);
  lv_obj_set_style_radius(hourHand, 2, 0);

  minuteHand = lv_obj_create(clockScreen);
  lv_obj_set_size(minuteHand, 3, 70);
  lv_obj_align(minuteHand, LV_ALIGN_CENTER, 0, -35);
  lv_obj_set_style_bg_color(minuteHand, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_style_border_width(minuteHand, 0, 0);
  lv_obj_set_style_radius(minuteHand, 1, 0);

  analogSecDot = lv_obj_create(clockScreen);
  lv_obj_set_size(analogSecDot, 5, 5);
  lv_obj_align(analogSecDot, LV_ALIGN_CENTER, 0, -80);
  lv_obj_set_style_bg_color(analogSecDot, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(analogSecDot, 0, 0);
  lv_obj_set_style_radius(analogSecDot, 3, 0);

  analogCenter = lv_obj_create(clockScreen);
  lv_obj_set_size(analogCenter, 10, 10);
  lv_obj_align(analogCenter, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(analogCenter, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(analogCenter, 0, 0);
  lv_obj_set_style_radius(analogCenter, 5, 0);

  // ===== Face 2: Minimal =====
  minimalTime = lv_label_create(clockScreen);
  lv_label_set_text(minimalTime, "00:00");
  lv_obj_set_style_text_font(minimalTime, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(minimalTime, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_letter_space(minimalTime, 6, 0);
  lv_obj_align(minimalTime, LV_ALIGN_CENTER, 0, -5);

  minimalSec = lv_label_create(clockScreen);
  lv_label_set_text(minimalSec, "00");
  lv_obj_set_style_text_font(minimalSec, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(minimalSec, lv_color_hex(0x333333), 0);
  lv_obj_align(minimalSec, LV_ALIGN_CENTER, 0, 30);

  // Face hint
  faceHint = lv_label_create(clockScreen);
  lv_label_set_text(faceHint, "");
  lv_obj_set_style_text_font(faceHint, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(faceHint, lv_color_hex(0x333333), 0);
  lv_obj_align(faceHint, LV_ALIGN_BOTTOM_MID, 0, -30);

  showFace(0);
}

static void hideAllFaces() {
  // Face 0
  for (int i = 0; i < NUM_DOTS; i++)
    lv_obj_add_flag(hourDots[i], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(timeLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(secLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(dateLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(secHand, LV_OBJ_FLAG_HIDDEN);
  // Face 1
  lv_obj_add_flag(hourHand, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(minuteHand, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(analogSecDot, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(analogCenter, LV_OBJ_FLAG_HIDDEN);
  // Face 2
  lv_obj_add_flag(minimalTime, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(minimalSec, LV_OBJ_FLAG_HIDDEN);
}

static void showFace(int face) {
  hideAllFaces();
  currentFace = face;
  lastSec = -1; // force redraw

  const char *names[] = {"Classic", "Analog", "Clean"};
  lv_label_set_text(faceHint, names[face]);
  lv_obj_align(faceHint, LV_ALIGN_BOTTOM_MID, 0, -30);

  switch (face) {
  case 0:
    for (int i = 0; i < NUM_DOTS; i++)
      lv_obj_clear_flag(hourDots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(timeLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(secLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(dateLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(secHand, LV_OBJ_FLAG_HIDDEN);
    break;
  case 1:
    for (int i = 0; i < NUM_DOTS; i++)
      lv_obj_clear_flag(hourDots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hourHand, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(minuteHand, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(analogSecDot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(analogCenter, LV_OBJ_FLAG_HIDDEN);
    break;
  case 2:
    lv_obj_clear_flag(minimalTime, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(minimalSec, LV_OBJ_FLAG_HIDDEN);
    break;
  }
}

void clock_next_face() { showFace((currentFace + 1) % NUM_FACES); }

void clock_show() {
  if (clockScreen)
    lv_scr_load_anim(clockScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}
void clock_hide() {}

static void updateFace0(struct tm *t) {
  char buf[24];
  int hr12 = t->tm_hour % 12;
  if (hr12 == 0)
    hr12 = 12;
  const char *ampm = (t->tm_hour < 12) ? "AM" : "PM";
  snprintf(buf, sizeof(buf), "%d:%02d", hr12, t->tm_min);
  lv_label_set_text(timeLabel, buf);
  lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -10);

  snprintf(buf, sizeof(buf), ":%02d %s", t->tm_sec, ampm);
  lv_label_set_text(secLabel, buf);
  lv_obj_align(secLabel, LV_ALIGN_CENTER, 0, 20);

  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(buf, sizeof(buf), "%s %d, %d", months[t->tm_mon], t->tm_mday,
           t->tm_year + 1900);
  lv_label_set_text(dateLabel, buf);
  lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 45);

  float angle = (t->tm_sec * 6.0f - 90.0f) * 3.14159f / 180.0f;
  lv_obj_align(secHand, LV_ALIGN_CENTER, (int)(cos(angle) * 90),
               (int)(sin(angle) * 90));
}

static void updateFace1(struct tm *t) {
  // Hour hand position (simulated with position offset since LVGL doesn't
  // rotate rects)
  float hAngle = ((t->tm_hour % 12) * 30.0f + t->tm_min * 0.5f - 90.0f) *
                 3.14159f / 180.0f;
  int hx = (int)(cos(hAngle) * 35);
  int hy = (int)(sin(hAngle) * 35);
  lv_obj_align(hourHand, LV_ALIGN_CENTER, hx / 2, hy / 2);

  float mAngle = (t->tm_min * 6.0f - 90.0f) * 3.14159f / 180.0f;
  int mx = (int)(cos(mAngle) * 55);
  int my = (int)(sin(mAngle) * 55);
  lv_obj_align(minuteHand, LV_ALIGN_CENTER, mx / 2, my / 2);

  float sAngle = (t->tm_sec * 6.0f - 90.0f) * 3.14159f / 180.0f;
  lv_obj_align(analogSecDot, LV_ALIGN_CENTER, (int)(cos(sAngle) * 80),
               (int)(sin(sAngle) * 80));
}

static void updateFace2(struct tm *t) {
  char buf[24];
  int hr12 = t->tm_hour % 12;
  if (hr12 == 0)
    hr12 = 12;
  const char *ampm = (t->tm_hour < 12) ? "AM" : "PM";
  snprintf(buf, sizeof(buf), "%d:%02d", hr12, t->tm_min);
  lv_label_set_text(minimalTime, buf);
  lv_obj_align(minimalTime, LV_ALIGN_CENTER, 0, -5);

  snprintf(buf, sizeof(buf), "%s", ampm);
  lv_label_set_text(minimalSec, buf);
  lv_obj_align(minimalSec, LV_ALIGN_CENTER, 0, 30);
}

void clock_update() {
  struct tm t;
  if (!getLocalTime(&t)) {
    unsigned long ms = millis() / 1000;
    t.tm_hour = (ms / 3600) % 24;
    t.tm_min = (ms / 60) % 60;
    t.tm_sec = ms % 60;
    t.tm_mday = 21;
    t.tm_mon = 1;
    t.tm_year = 126;
  }
  if (t.tm_sec == lastSec)
    return;
  lastSec = t.tm_sec;

  switch (currentFace) {
  case 0:
    updateFace0(&t);
    break;
  case 1:
    updateFace1(&t);
    break;
  case 2:
    updateFace2(&t);
    break;
  }
}
