#include "airplane.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static inline lv_point_t PT(lv_coord_t x, lv_coord_t y) { lv_point_t p; p.x = x; p.y = y; return p; }

static lv_obj_t *airplaneScreen = NULL;
static lv_obj_t *planeContainer = NULL;

// Airplane parts (inside container)
static lv_obj_t *partBody = NULL;
static lv_obj_t *partNoseL = NULL;
static lv_obj_t *partNoseR = NULL;
static lv_obj_t *partWingL = NULL;
static lv_obj_t *partWingR = NULL;
static lv_obj_t *partTipL = NULL;
static lv_obj_t *partTipR = NULL;
static lv_obj_t *partTailL = NULL;
static lv_obj_t *partTailR = NULL;
static lv_obj_t *partCenter = NULL;

// Bezel rings
static lv_obj_t *outerRing = NULL;
static lv_obj_t *innerRing = NULL;

// HUD labels
static lv_obj_t *pitchLabel = NULL;
static lv_obj_t *rollLabel = NULL;
static lv_obj_t *hdgLabel = NULL;

static float prevPitch = 0, prevRoll = 0, prevHdg = 0;

#define CX 120
#define CY 120

void airplane_init() {
  airplaneScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(airplaneScreen, lv_color_hex(0x0A121C), 0);
  lv_obj_clear_flag(airplaneScreen, LV_OBJ_FLAG_SCROLLABLE);

  // === Bezel rings ===
  outerRing = lv_obj_create(airplaneScreen);
  lv_obj_set_size(outerRing, 230, 230);
  lv_obj_align(outerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(outerRing, 115, 0);
  lv_obj_set_style_bg_color(outerRing, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_style_border_color(outerRing, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(outerRing, 1, 0);
  lv_obj_clear_flag(outerRing, LV_OBJ_FLAG_SCROLLABLE);

  innerRing = lv_obj_create(airplaneScreen);
  lv_obj_set_size(innerRing, 218, 218);
  lv_obj_align(innerRing, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(innerRing, 109, 0);
  lv_obj_set_style_bg_color(innerRing, lv_color_hex(0x0A121C), 0);
  lv_obj_set_style_border_color(innerRing, lv_color_hex(0x0F0F0F), 0);
  lv_obj_set_style_border_width(innerRing, 1, 0);
  lv_obj_clear_flag(innerRing, LV_OBJ_FLAG_SCROLLABLE);

  // === Reference grid (concentric circles as arc objects) ===
  // Outer reference circle
  lv_obj_t *refCircle1 = lv_obj_create(airplaneScreen);
  lv_obj_set_size(refCircle1, 210, 210);
  lv_obj_align(refCircle1, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(refCircle1, 105, 0);
  lv_obj_set_style_bg_opa(refCircle1, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(refCircle1, lv_color_hex(0x505050), 0);
  lv_obj_set_style_border_width(refCircle1, 1, 0);
  lv_obj_clear_flag(refCircle1, LV_OBJ_FLAG_SCROLLABLE);

  // Inner reference circle
  lv_obj_t *refCircle2 = lv_obj_create(airplaneScreen);
  lv_obj_set_size(refCircle2, 140, 140);
  lv_obj_align(refCircle2, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_radius(refCircle2, 70, 0);
  lv_obj_set_style_bg_opa(refCircle2, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(refCircle2, lv_color_hex(0x323232), 0);
  lv_obj_set_style_border_width(refCircle2, 1, 0);
  lv_obj_clear_flag(refCircle2, LV_OBJ_FLAG_SCROLLABLE);

  // Crosshair horizontal
  lv_obj_t *crossH = lv_obj_create(airplaneScreen);
  lv_obj_set_size(crossH, 210, 1);
  lv_obj_align(crossH, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(crossH, lv_color_hex(0x3C3C3C), 0);
  lv_obj_set_style_border_width(crossH, 0, 0);

  // Crosshair vertical
  lv_obj_t *crossV = lv_obj_create(airplaneScreen);
  lv_obj_set_size(crossV, 1, 210);
  lv_obj_align(crossV, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(crossV, lv_color_hex(0x3C3C3C), 0);
  lv_obj_set_style_border_width(crossV, 0, 0);

  // === Airplane container (rotates with roll, moves with pitch) ===
  planeContainer = lv_obj_create(airplaneScreen);
  lv_obj_set_size(planeContainer, 240, 240);
  lv_obj_align(planeContainer, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_opa(planeContainer, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(planeContainer, 0, 0);
  lv_obj_clear_flag(planeContainer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_transform_pivot_x(planeContainer, CX, 0);
  lv_obj_set_style_transform_pivot_y(planeContainer, CY, 0);

  uint32_t bodyCol = 0xFFB428;  // Amber/orange
  uint32_t wingCol = 0x28DCFF;  // Cyan
  uint32_t tailCol = 0xFF5050;  // Red

  // Airplane body (vertical)
  partBody = lv_obj_create(planeContainer);
  lv_obj_set_size(partBody, 5, 60);
  lv_obj_align(partBody, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_color(partBody, lv_color_hex(bodyCol), 0);
  lv_obj_set_style_border_width(partBody, 0, 0);
  lv_obj_set_style_radius(partBody, 2, 0);

  // Nose left
  partNoseL = lv_line_create(planeContainer);
  static lv_point_t nl[2];
  nl[0] = PT(CX, CY - 25);
  nl[1] = PT(CX - 12, CY + 5);
  lv_line_set_points(partNoseL, nl, 2);
  lv_obj_set_style_line_color(partNoseL, lv_color_hex(bodyCol), 0);
  lv_obj_set_style_line_width(partNoseL, 3, 0);

  // Nose right
  partNoseR = lv_line_create(planeContainer);
  static lv_point_t nr[2];
  nr[0] = PT(CX, CY - 25);
  nr[1] = PT(CX + 12, CY + 5);
  lv_line_set_points(partNoseR, nr, 2);
  lv_obj_set_style_line_color(partNoseR, lv_color_hex(bodyCol), 0);
  lv_obj_set_style_line_width(partNoseR, 3, 0);

  // Main wings (left)
  partWingL = lv_obj_create(planeContainer);
  lv_obj_set_size(partWingL, 70, 4);
  lv_obj_align(partWingL, LV_ALIGN_CENTER, -40, 15);
  lv_obj_set_style_bg_color(partWingL, lv_color_hex(wingCol), 0);
  lv_obj_set_style_border_width(partWingL, 0, 0);
  lv_obj_set_style_radius(partWingL, 2, 0);

  // Main wings (right)
  partWingR = lv_obj_create(planeContainer);
  lv_obj_set_size(partWingR, 70, 4);
  lv_obj_align(partWingR, LV_ALIGN_CENTER, 40, 15);
  lv_obj_set_style_bg_color(partWingR, lv_color_hex(wingCol), 0);
  lv_obj_set_style_border_width(partWingR, 0, 0);
  lv_obj_set_style_radius(partWingR, 2, 0);

  // Wing tips (left)
  partTipL = lv_obj_create(planeContainer);
  lv_obj_set_size(partTipL, 24, 3);
  lv_obj_align(partTipL, LV_ALIGN_CENTER, -68, 30);
  lv_obj_set_style_bg_color(partTipL, lv_color_hex(wingCol), 0);
  lv_obj_set_style_border_width(partTipL, 0, 0);
  lv_obj_set_style_radius(partTipL, 1, 0);
  lv_obj_set_style_bg_opa(partTipL, LV_OPA_80, 0);

  // Wing tips (right)
  partTipR = lv_obj_create(planeContainer);
  lv_obj_set_size(partTipR, 24, 3);
  lv_obj_align(partTipR, LV_ALIGN_CENTER, 68, 30);
  lv_obj_set_style_bg_color(partTipR, lv_color_hex(wingCol), 0);
  lv_obj_set_style_border_width(partTipR, 0, 0);
  lv_obj_set_style_radius(partTipR, 1, 0);
  lv_obj_set_style_bg_opa(partTipR, LV_OPA_80, 0);

  // Tail wings (left)
  partTailL = lv_obj_create(planeContainer);
  lv_obj_set_size(partTailL, 28, 3);
  lv_obj_align(partTailL, LV_ALIGN_CENTER, -18, 43);
  lv_obj_set_style_bg_color(partTailL, lv_color_hex(tailCol), 0);
  lv_obj_set_style_border_width(partTailL, 0, 0);
  lv_obj_set_style_radius(partTailL, 1, 0);

  // Tail wings (right)
  partTailR = lv_obj_create(planeContainer);
  lv_obj_set_size(partTailR, 28, 3);
  lv_obj_align(partTailR, LV_ALIGN_CENTER, 18, 43);
  lv_obj_set_style_bg_color(partTailR, lv_color_hex(tailCol), 0);
  lv_obj_set_style_border_width(partTailR, 0, 0);
  lv_obj_set_style_radius(partTailR, 1, 0);

  // Center dot
  partCenter = lv_obj_create(planeContainer);
  lv_obj_set_size(partCenter, 8, 8);
  lv_obj_align(partCenter, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_radius(partCenter, 4, 0);
  lv_obj_set_style_bg_color(partCenter, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(partCenter, 0, 0);

  // === HUD Labels ===
  pitchLabel = lv_label_create(airplaneScreen);
  lv_label_set_text(pitchLabel, "P +0\xC2\xB0");
  lv_obj_set_style_text_font(pitchLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(pitchLabel, lv_color_hex(0x00FFAA), 0);
  lv_obj_align(pitchLabel, LV_ALIGN_BOTTOM_LEFT, 10, -10);

  rollLabel = lv_label_create(airplaneScreen);
  lv_label_set_text(rollLabel, "R +0\xC2\xB0");
  lv_obj_set_style_text_font(rollLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(rollLabel, lv_color_hex(0x00FFAA), 0);
  lv_obj_align(rollLabel, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

  hdgLabel = lv_label_create(airplaneScreen);
  lv_label_set_text(hdgLabel, "HDG 000\xC2\xB0");
  lv_obj_set_style_text_font(hdgLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(hdgLabel, lv_color_hex(0x88CCFF), 0);
  lv_obj_align(hdgLabel, LV_ALIGN_TOP_MID, 0, 10);
}

void airplane_show() {
  if (airplaneScreen)
    lv_scr_load_anim(airplaneScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void airplane_hide() {}

void airplane_update(float pitch, float roll, float heading) {
  float p = constrain(pitch, -30, 30);
  float r = roll;
  float h = heading;

  // Smooth
  prevPitch += (p - prevPitch) * 0.2f;
  float rd = r - prevRoll;
  if (rd > 180) rd -= 360;
  if (rd < -180) rd += 360;
  prevRoll += rd * 0.2f;
  float hd = h - prevHdg;
  if (hd > 180) hd -= 360;
  if (hd < -180) hd += 360;
  prevHdg += hd * 0.2f;
  if (prevHdg < 0) prevHdg += 360;
  if (prevHdg >= 360) prevHdg -= 360;

  // Move container vertically for pitch
  int pitchMove = (int)(prevPitch * 2.5f);
  lv_obj_set_pos(planeContainer, 0, pitchMove);

  // Rotate container for roll
  lv_obj_set_style_transform_angle(planeContainer, (int16_t)(prevRoll * 10), 0);

  // Update HUD
  char buf[20];
  snprintf(buf, sizeof(buf), "P %+.0f\xC2\xB0", prevPitch);
  lv_label_set_text(pitchLabel, buf);
  lv_obj_align(pitchLabel, LV_ALIGN_BOTTOM_LEFT, 10, -10);

  snprintf(buf, sizeof(buf), "R %+.0f\xC2\xB0", prevRoll);
  lv_label_set_text(rollLabel, buf);
  lv_obj_align(rollLabel, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

  snprintf(buf, sizeof(buf), "HDG %03.0f\xC2\xB0", prevHdg);
  lv_label_set_text(hdgLabel, buf);
  lv_obj_align(hdgLabel, LV_ALIGN_TOP_MID, 0, 10);
}
