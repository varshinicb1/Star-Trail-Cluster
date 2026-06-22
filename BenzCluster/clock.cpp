#include "clock.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <time.h>

static inline lv_point_t PT(lv_coord_t x, lv_coord_t y) {
  lv_point_t p;
  p.x = x;
  p.y = y;
  return p;
}

#define CX 120
#define CY 120

static lv_obj_t *clockScreen = NULL;

// ======== Face 0: Classic Digital ========
static lv_obj_t *timeLabel = NULL;
static lv_obj_t *secLabel = NULL;
static lv_obj_t *dateLabel = NULL;
static lv_obj_t *secDot = NULL;
#define NUM_DOTS 12
static lv_obj_t *hourDots[NUM_DOTS];

// ======== Face 1: Analog ========
static lv_obj_t *analogHourHand = NULL;
static lv_obj_t *analogMinHand = NULL;
static lv_obj_t *analogSecHand = NULL;
static lv_point_t hourPts[2], minPts[2], secHandPts[2];
static lv_obj_t *analogCenter = NULL;
static lv_obj_t *analogHourMarks[12];

// ======== Face 2: Minimal ========
static lv_obj_t *minimalTime = NULL;
static lv_obj_t *minimalAmPm = NULL;
static lv_obj_t *minimalDate = NULL;

// Shared
static lv_obj_t *outerRing = NULL;
static lv_obj_t *faceHint = NULL;
static int currentFace = 0;
#define NUM_FACES 3
static int lastSec = -1;

static void hideAllFaces();
static void showFace(int face);

void clock_init() {
  clockScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(clockScreen, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(clockScreen, LV_OBJ_FLAG_SCROLLABLE);

  outerRing = lv_obj_create(clockScreen);
  lv_obj_set_size(outerRing, 238, 238);
  lv_obj_align(outerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(outerRing, 119, 0);
  lv_obj_set_style_bg_opa(outerRing, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(outerRing, lv_color_hex(0x222222), 0);
  lv_obj_set_style_border_width(outerRing, 2, 0);

  // ===== Face 0: Classic Digital =====
  for (int i = 0; i < NUM_DOTS; i++) {
    float angle = (i * 30.0f - 90.0f) * M_PI / 180.0f;
    int x = (int)(cosf(angle) * 108);
    int y = (int)(sinf(angle) * 108);
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
  lv_obj_align(secLabel, LV_ALIGN_CENTER, 0, 22);

  dateLabel = lv_label_create(clockScreen);
  lv_label_set_text(dateLabel, "---");
  lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(dateLabel, lv_color_hex(0x333333), 0);
  lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 45);

  secDot = lv_obj_create(clockScreen);
  lv_obj_set_size(secDot, 5, 5);
  lv_obj_set_style_radius(secDot, 3, 0);
  lv_obj_set_style_bg_color(secDot, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(secDot, 0, 0);

  // ===== Face 1: Analog Clock =====
  for (int i = 0; i < 12; i++) {
    float angle = (i * 30.0f - 90.0f) * M_PI / 180.0f;
    analogHourMarks[i] = lv_obj_create(clockScreen);
    bool major = (i % 3 == 0);
    int w = major ? 4 : 2;
    int h = major ? 14 : 8;
    lv_obj_set_size(analogHourMarks[i], w, h);
    // Position at edge of dial
    int r = 100;
    int cx = (int)(cosf(angle) * r);
    int cy = (int)(sinf(angle) * r);
    lv_obj_align(analogHourMarks[i], LV_ALIGN_CENTER, cx, cy);
    lv_obj_set_style_bg_color(
        analogHourMarks[i],
        major ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x666666), 0);
    lv_obj_set_style_border_width(analogHourMarks[i], 0, 0);
    lv_obj_set_style_radius(analogHourMarks[i], 1, 0);
  }
  // Hour hand
  analogHourHand = lv_line_create(clockScreen);
  hourPts[0] = PT(CX, CY);
  hourPts[1] = PT(CX, CY - 45);
  lv_line_set_points(analogHourHand, hourPts, 2);
  lv_obj_set_style_line_width(analogHourHand, 5, 0);
  lv_obj_set_style_line_color(analogHourHand, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_line_rounded(analogHourHand, true, 0);
  // Minute hand
  analogMinHand = lv_line_create(clockScreen);
  minPts[0] = PT(CX, CY);
  minPts[1] = PT(CX, CY - 70);
  lv_line_set_points(analogMinHand, minPts, 2);
  lv_obj_set_style_line_width(analogMinHand, 3, 0);
  lv_obj_set_style_line_color(analogMinHand, lv_color_hex(0xCCCCCC), 0);
  lv_obj_set_style_line_rounded(analogMinHand, true, 0);
  // Second hand
  analogSecHand = lv_line_create(clockScreen);
  secHandPts[0] = PT(CX, CY + 15);
  secHandPts[1] = PT(CX, CY - 85);
  lv_line_set_points(analogSecHand, secHandPts, 2);
  lv_obj_set_style_line_width(analogSecHand, 1, 0);
  lv_obj_set_style_line_color(analogSecHand, lv_color_hex(0xFF2222), 0);
  // Center dot
  analogCenter = lv_obj_create(clockScreen);
  lv_obj_set_size(analogCenter, 8, 8);
  lv_obj_align(analogCenter, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(analogCenter, lv_color_hex(0xFF2222), 0);
  lv_obj_set_style_border_width(analogCenter, 0, 0);
  lv_obj_set_style_radius(analogCenter, 4, 0);

  // ===== Face 2: Minimal =====
  minimalTime = lv_label_create(clockScreen);
  lv_label_set_text(minimalTime, "00:00");
  lv_obj_set_style_text_font(minimalTime, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(minimalTime, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_letter_space(minimalTime, 4, 0);
  lv_obj_align(minimalTime, LV_ALIGN_CENTER, 0, -12);

  minimalAmPm = lv_label_create(clockScreen);
  lv_label_set_text(minimalAmPm, "AM");
  lv_obj_set_style_text_font(minimalAmPm, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(minimalAmPm, lv_color_hex(0x444444), 0);
  lv_obj_align(minimalAmPm, LV_ALIGN_CENTER, 0, 30);

  minimalDate = lv_label_create(clockScreen);
  lv_label_set_text(minimalDate, "---");
  lv_obj_set_style_text_font(minimalDate, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(minimalDate, lv_color_hex(0x333333), 0);
  lv_obj_align(minimalDate, LV_ALIGN_CENTER, 0, 55);

  // Face hint
  faceHint = lv_label_create(clockScreen);
  lv_label_set_text(faceHint, "");
  lv_obj_set_style_text_font(faceHint, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(faceHint, lv_color_hex(0x333333), 0);
  lv_obj_align(faceHint, LV_ALIGN_BOTTOM_MID, 0, -20);

  showFace(0);
}

static void hideAllFaces() {
  // Face 0
  for (int i = 0; i < NUM_DOTS; i++)
    lv_obj_add_flag(hourDots[i], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(timeLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(secLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(dateLabel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(secDot, LV_OBJ_FLAG_HIDDEN);
  // Face 1
  for (int i = 0; i < 12; i++)
    lv_obj_add_flag(analogHourMarks[i], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(analogHourHand, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(analogMinHand, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(analogSecHand, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(analogCenter, LV_OBJ_FLAG_HIDDEN);
  // Face 2
  lv_obj_add_flag(minimalTime, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(minimalAmPm, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(minimalDate, LV_OBJ_FLAG_HIDDEN);
}

static void showFace(int face) {
  hideAllFaces();
  currentFace = face;
  lastSec = -1;
  const char *names[] = {"Digital", "Analog", "Clean"};
  lv_label_set_text(faceHint, names[face]);

  switch (face) {
  case 0:
    for (int i = 0; i < NUM_DOTS; i++)
      lv_obj_clear_flag(hourDots[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(timeLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(secLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(dateLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(secDot, LV_OBJ_FLAG_HIDDEN);
    break;
  case 1:
    for (int i = 0; i < 12; i++)
      lv_obj_clear_flag(analogHourMarks[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(analogHourHand, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(analogMinHand, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(analogSecHand, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(analogCenter, LV_OBJ_FLAG_HIDDEN);
    break;
  case 2:
    lv_obj_clear_flag(minimalTime, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(minimalAmPm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(minimalDate, LV_OBJ_FLAG_HIDDEN);
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
  snprintf(buf, sizeof(buf), "%d:%02d", hr12, t->tm_min);
  lv_label_set_text(timeLabel, buf);
  snprintf(buf, sizeof(buf), ":%02d %s", t->tm_sec,
           (t->tm_hour < 12) ? "AM" : "PM");
  lv_label_set_text(secLabel, buf);
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(buf, sizeof(buf), "%s %d, %d", months[t->tm_mon], t->tm_mday,
           t->tm_year + 1900);
  lv_label_set_text(dateLabel, buf);
  float angle = (t->tm_sec * 6.0f - 90.0f) * M_PI / 180.0f;
  lv_obj_align(secDot, LV_ALIGN_CENTER, (int)(cosf(angle) * 100),
               (int)(sinf(angle) * 100));
}

static void updateFace1(struct tm *t) {
  // Hour hand (short, thick)
  float hAngle =
      ((t->tm_hour % 12) * 30.0f + t->tm_min * 0.5f - 90.0f) * M_PI / 180.0f;
  hourPts[0] = PT(CX, CY);
  hourPts[1] = PT((lv_coord_t)(CX + cosf(hAngle) * 50),
                  (lv_coord_t)(CY + sinf(hAngle) * 50));
  lv_line_set_points(analogHourHand, hourPts, 2);

  // Minute hand (long, medium)
  float mAngle = (t->tm_min * 6.0f + t->tm_sec * 0.1f - 90.0f) * M_PI / 180.0f;
  minPts[0] = PT(CX, CY);
  minPts[1] = PT((lv_coord_t)(CX + cosf(mAngle) * 75),
                 (lv_coord_t)(CY + sinf(mAngle) * 75));
  lv_line_set_points(analogMinHand, minPts, 2);

  // Second hand (longest, thinnest, red, with tail)
  float sAngle = (t->tm_sec * 6.0f - 90.0f) * M_PI / 180.0f;
  secHandPts[0] = PT((lv_coord_t)(CX - cosf(sAngle) * 18),
                     (lv_coord_t)(CY - sinf(sAngle) * 18));
  secHandPts[1] = PT((lv_coord_t)(CX + cosf(sAngle) * 90),
                     (lv_coord_t)(CY + sinf(sAngle) * 90));
  lv_line_set_points(analogSecHand, secHandPts, 2);
}

static void updateFace2(struct tm *t) {
  char buf[24];
  int hr12 = t->tm_hour % 12;
  if (hr12 == 0)
    hr12 = 12;
  snprintf(buf, sizeof(buf), "%d:%02d", hr12, t->tm_min);
  lv_label_set_text(minimalTime, buf);
  lv_label_set_text(minimalAmPm, (t->tm_hour < 12) ? "AM" : "PM");
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(buf, sizeof(buf), "%s %d", months[t->tm_mon], t->tm_mday);
  lv_label_set_text(minimalDate, buf);
}

void clock_update() {
  struct tm t;
  if (!getLocalTime(&t)) {
    unsigned long ms = millis() / 1000;
    t.tm_hour = (ms / 3600) % 24;
    t.tm_min = (ms / 60) % 60;
    t.tm_sec = ms % 60;
    t.tm_mday = 27;
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
