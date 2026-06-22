#include "calibration_widget.h"
#include "calibration.h"
#include "config.h"
#include "sensors.h"
#include <Arduino.h>
#include <lvgl.h>

static lv_obj_t *calScreen = NULL;
static lv_obj_t *arcProgress = NULL;
static lv_obj_t *lblTitle = NULL;
static lv_obj_t *lblStatus = NULL;
static lv_obj_t *lblMagRaw = NULL;
static lv_obj_t *lblPercent = NULL;

static bool calActive = false;

extern bool calibrationMode; // from BenzCluster.ino

void calwidget_init() {}

void calwidget_show() {
  calScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(calScreen, lv_color_hex(0x0A0A1A), 0);
  lv_scr_load(calScreen);

  // Title
  lblTitle = lv_label_create(calScreen);
  lv_label_set_text(lblTitle, "MAG CAL");
  lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lblTitle, lv_color_hex(0x00CCFF), 0);
  lv_obj_set_style_text_letter_space(lblTitle, 3, 0);
  lv_obj_align(lblTitle, LV_ALIGN_TOP_MID, 0, 15);

  // Progress arc
  arcProgress = lv_arc_create(calScreen);
  lv_obj_set_size(arcProgress, 180, 180);
  lv_obj_align(arcProgress, LV_ALIGN_CENTER, 0, 0);
  lv_arc_set_rotation(arcProgress, 270);
  lv_arc_set_bg_angles(arcProgress, 0, 360);
  lv_arc_set_range(arcProgress, 0, 100);
  lv_arc_set_value(arcProgress, 0);
  lv_obj_set_style_arc_width(arcProgress, 6, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arcProgress, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arcProgress, lv_color_hex(0x222233), LV_PART_MAIN);
  lv_obj_set_style_arc_color(arcProgress, lv_color_hex(0x00FF88),
                             LV_PART_INDICATOR);
  lv_obj_remove_style(arcProgress, NULL, LV_PART_KNOB);

  // Percent label inside arc
  lblPercent = lv_label_create(calScreen);
  lv_label_set_text(lblPercent, "0%");
  lv_obj_set_style_text_font(lblPercent, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lblPercent, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(lblPercent, LV_ALIGN_CENTER, 0, -10);

  // Status text
  lblStatus = lv_label_create(calScreen);
  lv_label_set_text(lblStatus, "Rotate slowly\nin figure-8");
  lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x888888), 0);
  lv_obj_set_style_text_align(lblStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lblStatus, LV_ALIGN_CENTER, 0, 25);

  // Mag raw values at bottom
  lblMagRaw = lv_label_create(calScreen);
  lv_label_set_text(lblMagRaw, "M: ---");
  lv_obj_set_style_text_font(lblMagRaw, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lblMagRaw, lv_color_hex(0x555555), 0);
  lv_obj_set_style_text_align(lblMagRaw, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lblMagRaw, LV_ALIGN_BOTTOM_MID, 0, -12);

  // Don't auto-start - user taps to begin
  calActive = false;
  calibrationMode = false;
  lv_label_set_text(lblStatus, "Tap to start\ncalibration");

  // Tap handler to start calibration
  lv_obj_add_flag(calScreen, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(
      calScreen,
      [](lv_event_t *e) {
        if (!calActive) {
          calibration_start();
          calibrationMode = true;
          calActive = true;
          lv_label_set_text(lblStatus, "Rotate slowly\nin figure-8");
          lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x888888), 0);
          lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_12, 0);
          lv_arc_set_value(arcProgress, 0);
          lv_label_set_text(lblPercent, "0%");
        }
      },
      LV_EVENT_CLICKED, NULL);
}

void calwidget_hide() {
  calibrationMode = false;
  calActive = false;
  if (calScreen) {
    lv_obj_del(calScreen);
    calScreen = NULL;
  }
}

void calwidget_update() {
  if (!calScreen || !calActive)
    return;

  int progress = calibration_get_progress();
  lv_arc_set_value(arcProgress, progress);

  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", progress);
  lv_label_set_text(lblPercent, buf);

  // Show live mag raw
  int16_t mx, my, mz;
  sensors_get_mag_raw(&mx, &my, &mz);
  char magBuf[48];
  snprintf(magBuf, sizeof(magBuf), "X:%d Y:%d Z:%d", mx, my, mz);
  lv_label_set_text(lblMagRaw, magBuf);

  if (calibration_is_complete()) {
    calActive = false;
    calibrationMode = false;
    lv_label_set_text(lblStatus, "Calibrated!");
    lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_14, 0);
    lv_obj_set_style_arc_color(arcProgress, lv_color_hex(0x00FF88),
                               LV_PART_INDICATOR);
    Serial.println("[CAL_UI] Calibration complete!");
  }
}
