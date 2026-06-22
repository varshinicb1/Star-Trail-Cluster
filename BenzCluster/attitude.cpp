#include "attitude.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static inline lv_point_t PT(lv_coord_t x, lv_coord_t y) {
  lv_point_t p;
  p.x = x;
  p.y = y;
  return p;
}

// ============================================================================
// Professional Attitude Indicator — ICAO/FAA Western Standard
// Sky/ground background, pitch ladder with degree labels, bank angle arc
// with ticks at 10/20/30/45/60°, fixed sky pointer, moving roll pointer.
// ============================================================================

static lv_obj_t *attitudeScreen = NULL;

// Sky and ground
static lv_obj_t *skyRect = NULL;
static lv_obj_t *gndRect = NULL;
static lv_obj_t *horizLine = NULL;

// Pitch ladder
#define NUM_PITCH_MARKS 8
static int pitchDegs[NUM_PITCH_MARKS] = {-30, -20, -10, -5, 5, 10, 20, 30};
static lv_obj_t *pLines[NUM_PITCH_MARKS];
static lv_point_t pPts[NUM_PITCH_MARKS][2];
static lv_obj_t *pLabelsL[NUM_PITCH_MARKS]; // degree labels left
static lv_obj_t *pLabelsR[NUM_PITCH_MARKS]; // degree labels right

// Aircraft symbol (fixed)
static lv_obj_t *acLeft = NULL;
static lv_obj_t *acRight = NULL;
static lv_obj_t *acCenter = NULL;

// Virtual runway (miercemk-inspired)
static lv_obj_t *runwayObj = NULL;
static lv_obj_t *rwyEdge1 = NULL;
static lv_obj_t *rwyEdge2 = NULL;
static lv_obj_t *rwyEdge3 = NULL;

// Bank angle arc + ticks (ICAO: 10, 20, 30, 45, 60)
static lv_obj_t *bankArc = NULL;
#define NUM_BANK_TICKS 11
static lv_obj_t *bankTicks[NUM_BANK_TICKS];
static lv_point_t bankPts[NUM_BANK_TICKS][2];
// ±10, ±20, ±30, ±45, ±60, 0
static int bankAngles[NUM_BANK_TICKS] = {0,  -10, 10, -20, 20, -30,
                                         30, -45, 45, -60, 60};

// Fixed sky pointer (white triangle at top, always at 0°)
static lv_obj_t *skyPtr = NULL;
static lv_point_t skyPtrPts[3];

// Moving roll pointer (yellow triangle)
static lv_obj_t *rollPtr = NULL;
static lv_point_t rollPtrPts[3];

// HUD readouts — large, visible
static lv_obj_t *pitchValLabel = NULL;
static lv_obj_t *rollValLabel = NULL;

#define CX 120
#define CY 120
#define PPD 3

static float prevPitch = -999, prevRoll = -999;

void attitude_init() {
  attitudeScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(attitudeScreen, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(attitudeScreen, LV_OBJ_FLAG_SCROLLABLE);

  // === Sky ===
  skyRect = lv_obj_create(attitudeScreen);
  lv_obj_set_size(skyRect, 280, 280);
  lv_obj_set_style_bg_color(skyRect, lv_color_hex(0x1177CC), 0);
  lv_obj_set_style_bg_grad_color(skyRect, lv_color_hex(0x002255), 0);
  lv_obj_set_style_bg_grad_dir(skyRect, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_width(skyRect, 0, 0);
  lv_obj_set_style_radius(skyRect, 0, 0);
  lv_obj_clear_flag(skyRect, LV_OBJ_FLAG_SCROLLABLE);

  // === Ground ===
  gndRect = lv_obj_create(attitudeScreen);
  lv_obj_set_size(gndRect, 280, 280);
  lv_obj_set_style_bg_color(gndRect, lv_color_hex(0x885522), 0);
  lv_obj_set_style_bg_grad_color(gndRect, lv_color_hex(0x331100), 0);
  lv_obj_set_style_bg_grad_dir(gndRect, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_width(gndRect, 0, 0);
  lv_obj_set_style_radius(gndRect, 0, 0);
  lv_obj_clear_flag(gndRect, LV_OBJ_FLAG_SCROLLABLE);

  // === Horizon line ===
  horizLine = lv_obj_create(attitudeScreen);
  lv_obj_set_size(horizLine, 280, 3);
  lv_obj_set_style_bg_color(horizLine, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(horizLine, 0, 0);
  lv_obj_set_style_radius(horizLine, 0, 0);

  // === Virtual Runway (airport-style perspective) ===
  static lv_point_t rwyPts[3];
  static lv_point_t rwyLine1[2];
  static lv_point_t rwyLine2[2];
  static lv_point_t rwyLine3[2];
  runwayObj = lv_line_create(attitudeScreen);
  lv_line_set_points(runwayObj, rwyPts, 3);
  lv_obj_set_style_line_color(runwayObj, lv_color_hex(0x502616), 0);
  lv_obj_set_style_line_width(runwayObj, 0, 0);
  lv_obj_set_style_line_opa(runwayObj, LV_OPA_80, 0);

  rwyEdge1 = lv_line_create(attitudeScreen);
  lv_line_set_points(rwyEdge1, rwyLine1, 2);
  lv_obj_set_style_line_color(rwyEdge1, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_line_width(rwyEdge1, 2, 0);

  rwyEdge2 = lv_line_create(attitudeScreen);
  lv_line_set_points(rwyEdge2, rwyLine2, 2);
  lv_obj_set_style_line_color(rwyEdge2, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_line_width(rwyEdge2, 2, 0);

  rwyEdge3 = lv_line_create(attitudeScreen);
  lv_line_set_points(rwyEdge3, rwyLine3, 2);
  lv_obj_set_style_line_color(rwyEdge3, lv_color_hex(0xFF8800), 0);
  lv_obj_set_style_line_width(rwyEdge3, 2, 0);

  // === Pitch ladder ===
  for (int i = 0; i < NUM_PITCH_MARKS; i++) {
    pLines[i] = lv_line_create(attitudeScreen);
    pPts[i][0] = PT(0, 0);
    pPts[i][1] = PT(0, 0);
    lv_line_set_points(pLines[i], pPts[i], 2);
    bool major = (abs(pitchDegs[i]) % 10 == 0);
    lv_obj_set_style_line_width(pLines[i], major ? 2 : 1, 0);
    lv_obj_set_style_line_color(pLines[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_line_opa(pLines[i], major ? LV_OPA_80 : LV_OPA_40, 0);

    // Degree labels for major marks
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", abs(pitchDegs[i]));
    pLabelsL[i] = lv_label_create(attitudeScreen);
    lv_label_set_text(pLabelsL[i], buf);
    lv_obj_set_style_text_font(pLabelsL[i], &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(pLabelsL[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(pLabelsL[i], major ? LV_OPA_70 : LV_OPA_30, 0);
    pLabelsR[i] = lv_label_create(attitudeScreen);
    lv_label_set_text(pLabelsR[i], buf);
    lv_obj_set_style_text_font(pLabelsR[i], &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(pLabelsR[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(pLabelsR[i], major ? LV_OPA_70 : LV_OPA_30, 0);
  }

  // === Bank arc ===
  bankArc = lv_arc_create(attitudeScreen);
  lv_obj_set_size(bankArc, 228, 228);
  lv_obj_align(bankArc, LV_ALIGN_CENTER, 0, 0);
  lv_arc_set_bg_angles(bankArc, 200, 340);
  lv_arc_set_value(bankArc, 0);
  lv_obj_set_style_arc_width(bankArc, 2, LV_PART_MAIN);
  lv_obj_set_style_arc_color(bankArc, lv_color_hex(0x999999), LV_PART_MAIN);
  lv_obj_remove_style(bankArc, NULL, LV_PART_INDICATOR);
  lv_obj_remove_style(bankArc, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(bankArc, LV_OBJ_FLAG_CLICKABLE);

  // Bank tick marks (ICAO standard positions)
  for (int i = 0; i < NUM_BANK_TICKS; i++) {
    bankTicks[i] = lv_line_create(attitudeScreen);
    float angRad = (bankAngles[i] - 90.0f) * M_PI / 180.0f;
    bool isZero = (bankAngles[i] == 0);
    bool isMajor = (abs(bankAngles[i]) == 30 || abs(bankAngles[i]) == 60);
    int r1 = 110;
    int r2 = isZero ? 98 : (isMajor ? 100 : 104);
    bankPts[i][0] = PT((lv_coord_t)(CX + cosf(angRad) * r1),
                       (lv_coord_t)(CY + sinf(angRad) * r1));
    bankPts[i][1] = PT((lv_coord_t)(CX + cosf(angRad) * r2),
                       (lv_coord_t)(CY + sinf(angRad) * r2));
    lv_line_set_points(bankTicks[i], bankPts[i], 2);
    lv_obj_set_style_line_width(bankTicks[i], isZero ? 3 : 2, 0);
    lv_obj_set_style_line_color(bankTicks[i], lv_color_hex(0xFFFFFF), 0);
  }

  // === Fixed sky pointer (white triangle at 0° top) ===
  skyPtr = lv_line_create(attitudeScreen);
  float topRad = -90.0f * M_PI / 180.0f;
  skyPtrPts[0] = PT((lv_coord_t)(CX + cosf(topRad) * 96),
                    (lv_coord_t)(CY + sinf(topRad) * 96));
  skyPtrPts[1] = PT((lv_coord_t)(CX + cosf(topRad - 0.08f) * 88),
                    (lv_coord_t)(CY + sinf(topRad - 0.08f) * 88));
  skyPtrPts[2] = PT((lv_coord_t)(CX + cosf(topRad + 0.08f) * 88),
                    (lv_coord_t)(CY + sinf(topRad + 0.08f) * 88));
  lv_line_set_points(skyPtr, skyPtrPts, 3);
  lv_obj_set_style_line_width(skyPtr, 2, 0);
  lv_obj_set_style_line_color(skyPtr, lv_color_hex(0xFFFFFF), 0);

  // === Moving roll pointer (yellow triangle) ===
  rollPtr = lv_line_create(attitudeScreen);
  rollPtrPts[0] = PT(0, 0);
  rollPtrPts[1] = PT(0, 0);
  rollPtrPts[2] = PT(0, 0);
  lv_line_set_points(rollPtr, rollPtrPts, 3);
  lv_obj_set_style_line_width(rollPtr, 2, 0);
  lv_obj_set_style_line_color(rollPtr, lv_color_hex(0xFFDD00), 0);

  // === Aircraft symbol (miercemk-style: long yellow wings + inverted V) ===
  acLeft = lv_obj_create(attitudeScreen);
  lv_obj_set_size(acLeft, 70, 5);
  lv_obj_align(acLeft, LV_ALIGN_CENTER, -55, 0);
  lv_obj_set_style_bg_color(acLeft, lv_color_hex(0xFFDD00), 0);
  lv_obj_set_style_border_width(acLeft, 0, 0);
  lv_obj_set_style_radius(acLeft, 2, 0);
  lv_obj_set_style_shadow_width(acLeft, 6, 0);
  lv_obj_set_style_shadow_color(acLeft, lv_color_hex(0xFFDD00), 0);
  lv_obj_set_style_shadow_opa(acLeft, LV_OPA_40, 0);

  acRight = lv_obj_create(attitudeScreen);
  lv_obj_set_size(acRight, 70, 5);
  lv_obj_align(acRight, LV_ALIGN_CENTER, 55, 0);
  lv_obj_set_style_bg_color(acRight, lv_color_hex(0xFFDD00), 0);
  lv_obj_set_style_border_width(acRight, 0, 0);
  lv_obj_set_style_radius(acRight, 2, 0);
  lv_obj_set_style_shadow_width(acRight, 6, 0);
  lv_obj_set_style_shadow_color(acRight, lv_color_hex(0xFFDD00), 0);
  lv_obj_set_style_shadow_opa(acRight, LV_OPA_40, 0);

  // Inverted V center
  acCenter = lv_line_create(attitudeScreen);
  static lv_point_t ctrPts[4];
  ctrPts[0] = PT(CX - 18, CY - 2);
  ctrPts[1] = PT(CX, CY + 14);
  ctrPts[2] = PT(CX + 18, CY - 2);
  ctrPts[3] = PT(CX, CY + 14);
  lv_line_set_points(acCenter, ctrPts, 4);
  lv_obj_set_style_line_color(acCenter, lv_color_hex(0xFFDD00), 0);
  lv_obj_set_style_line_width(acCenter, 5, 0);
  lv_obj_set_style_line_rounded(acCenter, true, 0);

  // === HUD values (large, always visible) ===
  pitchValLabel = lv_label_create(attitudeScreen);
  lv_label_set_text(pitchValLabel, "P +0°");
  lv_obj_set_style_text_font(pitchValLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(pitchValLabel, lv_color_hex(0x00FFAA), 0);
  lv_obj_align(pitchValLabel, LV_ALIGN_BOTTOM_LEFT, 15, -10);

  rollValLabel = lv_label_create(attitudeScreen);
  lv_label_set_text(rollValLabel, "R +0°");
  lv_obj_set_style_text_font(rollValLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(rollValLabel, lv_color_hex(0x00FFAA), 0);
  lv_obj_align(rollValLabel, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
}

void attitude_update(float pitch, float roll) {
  if (!attitudeScreen)
    return;

  // Smooth filter
  if (prevPitch < -900) {
    prevPitch = pitch;
    prevRoll = roll;
  } else {
    prevPitch += (pitch - prevPitch) * 0.25f;
    float rd = roll - prevRoll;
    if (rd > 180)
      rd -= 360;
    if (rd < -180)
      rd += 360;
    prevRoll += rd * 0.25f;
  }

  float p = constrain(prevPitch, -60, 60);
  float r = prevRoll;
  int pitchPx = (int)(p * PPD);

  // Move sky/ground/horizon (full 240px width for circular clip)
  lv_obj_set_pos(skyRect, -20, CY + pitchPx - 240);
  lv_obj_set_pos(gndRect, -20, CY + pitchPx);
  lv_obj_set_pos(horizLine, -20, CY + pitchPx - 1);

  // Pitch ladder (rotates with roll)
  float rRad = r * M_PI / 180.0f;
  float cosR = cosf(rRad);
  float sinR = sinf(rRad);

  for (int i = 0; i < NUM_PITCH_MARKS; i++) {
    int deg = pitchDegs[i];
    float dy = (float)(p - deg) * PPD;
    float lx = CX + dy * sinR;
    float ly = CY + dy * cosR;
    int halfLen = (abs(deg) % 10 == 0) ? 40 : 20;
    float dx = halfLen * cosR;
    float ddy = -halfLen * sinR;

    pPts[i][0] = PT((lv_coord_t)(lx - dx), (lv_coord_t)(ly - ddy));
    pPts[i][1] = PT((lv_coord_t)(lx + dx), (lv_coord_t)(ly + ddy));
    lv_line_set_points(pLines[i], pPts[i], 2);

    // Position degree labels at ends of lines
    if (pLabelsL[i]) {
      lv_obj_set_pos(pLabelsL[i], pPts[i][0].x - 18, pPts[i][0].y - 6);
      lv_obj_set_pos(pLabelsR[i], pPts[i][1].x + 4, pPts[i][1].y - 6);
    }
  }

  // === Virtual Runway (miercemk-style filled triangle + perspective) ===
  {
    float rwyTop = 8.0f - pitchPx;
    float rwyBot = 120.0f - pitchPx;
    float rwyHalf = 100.0f;
    // Runway triangle (thick brown line simulates filled polygon)
    static lv_point_t rPts[3];
    rPts[0] = PT(CX, (lv_coord_t)(CY + rwyTop));
    rPts[1] = PT(CX - rwyHalf, (lv_coord_t)(CY + rwyBot));
    rPts[2] = PT(CX + rwyHalf, (lv_coord_t)(CY + rwyBot));
    lv_line_set_points(runwayObj, rPts, 3);
    lv_obj_set_style_line_width(runwayObj, 50, 0);
    lv_obj_set_style_line_rounded(runwayObj, false, 0);

    // Orange perspective lines
    float rwyMid = 36.0f;
    static lv_point_t e1[2], e2[2], e3[2];
    e1[0] = rPts[0];
    e1[1] = PT(CX - rwyMid, (lv_coord_t)(CY + rwyBot));
    lv_line_set_points(rwyEdge1, e1, 2);

    e2[0] = rPts[0];
    e2[1] = PT(CX + rwyMid, (lv_coord_t)(CY + rwyBot));
    lv_line_set_points(rwyEdge2, e2, 2);

    e3[0] = PT(CX - rwyMid, (lv_coord_t)(CY + rwyBot));
    e3[1] = PT(CX + rwyMid, (lv_coord_t)(CY + rwyBot));
    lv_line_set_points(rwyEdge3, e3, 2);
  }

  // Roll pointer (yellow, moves along bank arc)
  float pRad = (-r - 90.0f) * M_PI / 180.0f;
  rollPtrPts[0] = PT((lv_coord_t)(CX + cosf(pRad) * 112),
                     (lv_coord_t)(CY + sinf(pRad) * 112));
  rollPtrPts[1] = PT((lv_coord_t)(CX + cosf(pRad - 0.07f) * 104),
                     (lv_coord_t)(CY + sinf(pRad - 0.07f) * 104));
  rollPtrPts[2] = PT((lv_coord_t)(CX + cosf(pRad + 0.07f) * 104),
                     (lv_coord_t)(CY + sinf(pRad + 0.07f) * 104));
  lv_line_set_points(rollPtr, rollPtrPts, 3);

  // Update HUD values
  char buf[16];
  snprintf(buf, sizeof(buf), "P %+.0f\xC2\xB0", prevPitch);
  lv_label_set_text(pitchValLabel, buf);
  snprintf(buf, sizeof(buf), "R %+.0f\xC2\xB0", prevRoll);
  lv_label_set_text(rollValLabel, buf);
}

void attitude_show() {
  if (attitudeScreen)
    lv_scr_load_anim(attitudeScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0,
                     false);
}

void attitude_hide() {}
