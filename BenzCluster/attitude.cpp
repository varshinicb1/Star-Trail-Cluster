#include "attitude.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *attitudeScreen = NULL;
static lv_obj_t *clipContainer = NULL;
static lv_obj_t *skyObj = NULL;
static lv_obj_t *groundObj = NULL;
static lv_obj_t *horizonLine = NULL;
static lv_obj_t *pitchLabel = NULL;
static lv_obj_t *rollLabel = NULL;

// Pitch ladder
#define NUM_PITCH_LINES 6
static lv_obj_t *pitchLines[NUM_PITCH_LINES];
static lv_obj_t *pitchLabelsL[NUM_PITCH_LINES];
static lv_obj_t *pitchLabelsR[NUM_PITCH_LINES];

// ===== Floating Roll Arc Layer =====
// Roll indicator arc aroundthe outside — this layer floats/rotates with roll
#define NUM_ROLL_TICKS 11
static lv_obj_t *rollTicks[NUM_ROLL_TICKS]; // tick marks around arc
static lv_obj_t *rollPointer = NULL;        // roll pointer triangle at top

#define CX 100
#define CY 100
#define CR 95
#define SAFE_R 90 // keep elements inside this radius

static float lastPitch = -999, lastRoll = -999;

void attitude_init() {
  attitudeScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(attitudeScreen, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(attitudeScreen, LV_OBJ_FLAG_SCROLLABLE);

  // ===== Floating roll tick marks around the outer ring =====
  // Marks at 0, ±10, ±20, ±30, ±45, ±60 degrees
  int rollAngles[] = {0, -10, 10, -20, 20, -30, 30, -45, 45, -60, 60};
  for (int i = 0; i < NUM_ROLL_TICKS; i++) {
    rollTicks[i] = lv_obj_create(attitudeScreen);
    int len = (rollAngles[i] == 0) ? 12 : (abs(rollAngles[i]) <= 30) ? 8 : 5;
    int w = (rollAngles[i] == 0) ? 3 : 2;
    lv_obj_set_size(rollTicks[i], w, len);
    lv_obj_set_style_bg_color(rollTicks[i],
                              rollAngles[i] == 0 ? lv_color_hex(0xFFFFFF)
                              : abs(rollAngles[i]) <= 30
                                  ? lv_color_hex(0xAAAAAA)
                                  : lv_color_hex(0x555555),
                              0);
    lv_obj_set_style_border_width(rollTicks[i], 0, 0);
    lv_obj_set_style_radius(rollTicks[i], 0, 0);
    // Initial position — will be updated in update() based on roll
    float ang = (rollAngles[i] - 90.0f) * 3.14159f / 180.0f;
    lv_obj_align(rollTicks[i], LV_ALIGN_CENTER, (int)(cos(ang) * SAFE_R),
                 (int)(sin(ang) * SAFE_R) - 5);
  }

  // Roll pointer (white triangle at top, fixed)
  rollPointer = lv_obj_create(attitudeScreen);
  lv_obj_set_size(rollPointer, 8, 8);
  lv_obj_align(rollPointer, LV_ALIGN_CENTER, 0, -SAFE_R - 5);
  lv_obj_set_style_bg_color(rollPointer, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(rollPointer, 0, 0);
  lv_obj_set_style_radius(rollPointer, 1, 0);

  // ===== Circular clip container for artificial horizon =====
  clipContainer = lv_obj_create(attitudeScreen);
  lv_obj_set_size(clipContainer, 170, 170);
  lv_obj_align(clipContainer, LV_ALIGN_CENTER, 0, -5);
  lv_obj_set_style_radius(clipContainer, 85, 0);
  lv_obj_set_style_clip_corner(clipContainer, true, 0);
  lv_obj_set_style_border_color(clipContainer, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_width(clipContainer, 1, 0);
  lv_obj_set_style_pad_all(clipContainer, 0, 0);
  lv_obj_clear_flag(clipContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(clipContainer, LV_OPA_TRANSP, 0);

  // Sky with gradient
  skyObj = lv_obj_create(clipContainer);
  lv_obj_set_size(skyObj, 220, 180);
  lv_obj_set_pos(skyObj, -25, -90);
  lv_obj_set_style_bg_color(skyObj, lv_color_hex(0x1166CC), 0);
  lv_obj_set_style_bg_grad_color(skyObj, lv_color_hex(0x002266), 0);
  lv_obj_set_style_bg_grad_dir(skyObj, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_width(skyObj, 0, 0);
  lv_obj_set_style_radius(skyObj, 0, 0);

  // Ground with gradient
  groundObj = lv_obj_create(clipContainer);
  lv_obj_set_size(groundObj, 220, 180);
  lv_obj_set_pos(groundObj, -25, 85);
  lv_obj_set_style_bg_color(groundObj, lv_color_hex(0x885522), 0);
  lv_obj_set_style_bg_grad_color(groundObj, lv_color_hex(0x442211), 0);
  lv_obj_set_style_bg_grad_dir(groundObj, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_border_width(groundObj, 0, 0);
  lv_obj_set_style_radius(groundObj, 0, 0);

  // Horizon line
  horizonLine = lv_obj_create(clipContainer);
  lv_obj_set_size(horizonLine, 200, 2);
  lv_obj_set_pos(horizonLine, -15, 84);
  lv_obj_set_style_bg_color(horizonLine, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(horizonLine, 0, 0);
  lv_obj_set_style_radius(horizonLine, 0, 0);

  // Pitch ladder (±10, ±20, ±30)
  int pitchDegs[] = {-30, -20, -10, 10, 20, 30};
  for (int i = 0; i < NUM_PITCH_LINES; i++) {
    int lineW = (abs(pitchDegs[i]) >= 30)   ? 40
                : (abs(pitchDegs[i]) >= 20) ? 30
                                            : 22;
    pitchLines[i] = lv_obj_create(clipContainer);
    lv_obj_set_size(pitchLines[i], lineW, 1);
    lv_obj_set_style_bg_color(pitchLines[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(pitchLines[i], LV_OPA_50, 0);
    lv_obj_set_style_border_width(pitchLines[i], 0, 0);
    lv_obj_set_style_radius(pitchLines[i], 0, 0);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", pitchDegs[i]);
    pitchLabelsL[i] = lv_label_create(clipContainer);
    lv_label_set_text(pitchLabelsL[i], buf);
    lv_obj_set_style_text_font(pitchLabelsL[i], &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(pitchLabelsL[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(pitchLabelsL[i], LV_OPA_50, 0);

    pitchLabelsR[i] = lv_label_create(clipContainer);
    lv_label_set_text(pitchLabelsR[i], buf);
    lv_obj_set_style_text_font(pitchLabelsR[i], &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(pitchLabelsR[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_opa(pitchLabelsR[i], LV_OPA_50, 0);
  }

  // ===== Aircraft symbol =====
  lv_obj_t *lwing = lv_obj_create(attitudeScreen);
  lv_obj_set_size(lwing, 30, 3);
  lv_obj_align(lwing, LV_ALIGN_CENTER, -32, -5);
  lv_obj_set_style_bg_color(lwing, lv_color_hex(0xFFD700), 0);
  lv_obj_set_style_border_width(lwing, 0, 0);
  lv_obj_set_style_radius(lwing, 1, 0);

  lv_obj_t *ltip = lv_obj_create(attitudeScreen);
  lv_obj_set_size(ltip, 3, 8);
  lv_obj_align(ltip, LV_ALIGN_CENTER, -47, -1);
  lv_obj_set_style_bg_color(ltip, lv_color_hex(0xFFD700), 0);
  lv_obj_set_style_border_width(ltip, 0, 0);

  lv_obj_t *rwing = lv_obj_create(attitudeScreen);
  lv_obj_set_size(rwing, 30, 3);
  lv_obj_align(rwing, LV_ALIGN_CENTER, 32, -5);
  lv_obj_set_style_bg_color(rwing, lv_color_hex(0xFFD700), 0);
  lv_obj_set_style_border_width(rwing, 0, 0);
  lv_obj_set_style_radius(rwing, 1, 0);

  lv_obj_t *rtip = lv_obj_create(attitudeScreen);
  lv_obj_set_size(rtip, 3, 8);
  lv_obj_align(rtip, LV_ALIGN_CENTER, 47, -1);
  lv_obj_set_style_bg_color(rtip, lv_color_hex(0xFFD700), 0);
  lv_obj_set_style_border_width(rtip, 0, 0);

  lv_obj_t *cdot = lv_obj_create(attitudeScreen);
  lv_obj_set_size(cdot, 6, 6);
  lv_obj_align(cdot, LV_ALIGN_CENTER, 0, -5);
  lv_obj_set_style_bg_color(cdot, lv_color_hex(0xFFD700), 0);
  lv_obj_set_style_border_width(cdot, 0, 0);
  lv_obj_set_style_radius(cdot, 3, 0);

  // Pitch/Roll readout — positioned inside safe area
  pitchLabel = lv_label_create(attitudeScreen);
  lv_label_set_text(pitchLabel, "P: 0°");
  lv_obj_set_style_text_font(pitchLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(pitchLabel, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(pitchLabel, LV_ALIGN_CENTER, -35, 100);

  rollLabel = lv_label_create(attitudeScreen);
  lv_label_set_text(rollLabel, "R: 0°");
  lv_obj_set_style_text_font(rollLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(rollLabel, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(rollLabel, LV_ALIGN_CENTER, 35, 100);
}

void attitude_show() {
  if (attitudeScreen)
    lv_scr_load_anim(attitudeScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0,
                     false);
}
void attitude_hide() {}

void attitude_update(float pitchIn, float rollIn) {
  if (fabs(pitchIn - lastPitch) < 0.3f && fabs(rollIn - lastRoll) < 0.3f)
    return;
  lastPitch = pitchIn;
  lastRoll = rollIn;

  // ===== Update floating roll tick positions =====
  int rollAngles[] = {0, -10, 10, -20, 20, -30, 30, -45, 45, -60, 60};
  for (int i = 0; i < NUM_ROLL_TICKS; i++) {
    // Each tick rotates by the current roll angle
    float ang = ((rollAngles[i] + rollIn) - 90.0f) * 3.14159f / 180.0f;
    int ex = (int)(cos(ang) * SAFE_R);
    int ey = (int)(sin(ang) * SAFE_R);
    lv_obj_align(rollTicks[i], LV_ALIGN_CENTER, ex, ey - 5);
  }

  // ===== Update horizon position from pitch =====
  float pitchPx = pitchIn * 2.0f;
  int hCenter = 85; // center of the 170px clip container
  int horizonY = hCenter + (int)pitchPx;
  horizonY = constrain(horizonY, -30, 200);

  lv_obj_set_pos(skyObj, -25, horizonY - 180);
  lv_obj_set_pos(groundObj, -25, horizonY);
  lv_obj_set_pos(horizonLine, -15, horizonY - 1);

  // ===== Pitch ladder =====
  int pitchDegs[] = {-30, -20, -10, 10, 20, 30};
  for (int i = 0; i < NUM_PITCH_LINES; i++) {
    int lineW = (abs(pitchDegs[i]) >= 30)   ? 40
                : (abs(pitchDegs[i]) >= 20) ? 30
                                            : 22;
    int lineY = horizonY - (int)(pitchDegs[i] * 2.0f);
    if (lineY < 5 || lineY > 165) {
      lv_obj_add_flag(pitchLines[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(pitchLabelsL[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(pitchLabelsR[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(pitchLines[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(pitchLabelsL[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(pitchLabelsR[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(pitchLines[i], 85 - lineW / 2, lineY);
      lv_obj_set_pos(pitchLabelsL[i], 85 - lineW / 2 - 20, lineY - 5);
      lv_obj_set_pos(pitchLabelsR[i], 85 + lineW / 2 + 3, lineY - 5);
    }
  }

  // ===== Text readout =====
  char buf[16];
  snprintf(buf, sizeof(buf), "P:%d°", (int)pitchIn);
  lv_label_set_text(pitchLabel, buf);
  lv_obj_align(pitchLabel, LV_ALIGN_CENTER, -35, 100);

  snprintf(buf, sizeof(buf), "R:%d°", (int)rollIn);
  lv_label_set_text(rollLabel, buf);
  lv_obj_align(rollLabel, LV_ALIGN_CENTER, 35, 100);
}
