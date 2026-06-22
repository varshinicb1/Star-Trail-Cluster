#include "systemview.h"
#include "calibration.h"
#include "calibration_widget.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <lvgl.h>

static lv_obj_t *systemScreen = NULL;
static lv_obj_t *rebootBtn = NULL;
static lv_obj_t *calBtn = NULL;
static lv_obj_t *themeBtn = NULL;
static lv_obj_t *themeLbl = NULL;

extern bool calibrationMode;
extern bool systemVisible;

static const char *themes[] = {"star_trail", "illuminati", "vw"};
static const char *themeNames[] = {"Star Trail", "Illuminati", "VW"};
static int currentTheme = 0;
#define NUM_THEMES 3

static void loadTheme() {
  File f = SPIFFS.open("/splash_theme.txt", "r");
  if (f) {
    String t = f.readString();
    t.trim();
    f.close();
    for (int i = 0; i < NUM_THEMES; i++) {
      if (t == themes[i]) {
        currentTheme = i;
        return;
      }
    }
  }
}

static void saveTheme() {
  File f = SPIFFS.open("/splash_theme.txt", "w");
  if (f) {
    f.print(themes[currentTheme]);
    f.close();
  }
}

static void btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    if (obj == rebootBtn) {
      Serial.println("[SYS] Reboot -> Restarting...");
      delay(500);
      ESP.restart();
    } else if (obj == calBtn) {
      Serial.println("[SYS] Calibrate triggered from system menu");
      calwidget_show();
    } else if (obj == themeBtn) {
      currentTheme = (currentTheme + 1) % NUM_THEMES;
      saveTheme();
      char buf[32];
      snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE "  %s",
               themeNames[currentTheme]);
      lv_label_set_text(themeLbl, buf);
      lv_obj_center(themeLbl);
      Serial.printf("[SYS] Splash theme: %s\n", themeNames[currentTheme]);
    }
  }
}

void systemview_init() {
  systemScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(systemScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(systemScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Title
  lv_obj_t *title = lv_label_create(systemScreen);
  lv_label_set_text(title, LV_SYMBOL_SETTINGS "  SYSTEM");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -70);

  // Splash Theme button
  loadTheme();
  themeBtn = lv_btn_create(systemScreen);
  lv_obj_set_size(themeBtn, 140, 36);
  lv_obj_align(themeBtn, LV_ALIGN_CENTER, 0, -30);
  lv_obj_set_style_bg_color(themeBtn, lv_color_hex(0x333366), 0);
  lv_obj_set_style_radius(themeBtn, 18, 0);
  lv_obj_add_event_cb(themeBtn, btn_event_cb, LV_EVENT_CLICKED, NULL);

  themeLbl = lv_label_create(themeBtn);
  char buf[32];
  snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE "  %s", themeNames[currentTheme]);
  lv_label_set_text(themeLbl, buf);
  lv_obj_set_style_text_font(themeLbl, &lv_font_montserrat_12, 0);
  lv_obj_center(themeLbl);

  // Calibrate button
  calBtn = lv_btn_create(systemScreen);
  lv_obj_set_size(calBtn, 140, 36);
  lv_obj_align(calBtn, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_color(calBtn, lv_color_hex(0x0088CC), 0);
  lv_obj_set_style_radius(calBtn, 18, 0);
  lv_obj_add_event_cb(calBtn, btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *calLbl = lv_label_create(calBtn);
  lv_label_set_text(calLbl, LV_SYMBOL_REFRESH "  Calibrate");
  lv_obj_set_style_text_font(calLbl, &lv_font_montserrat_12, 0);
  lv_obj_center(calLbl);

  // Reboot button
  rebootBtn = lv_btn_create(systemScreen);
  lv_obj_set_size(rebootBtn, 140, 36);
  lv_obj_align(rebootBtn, LV_ALIGN_CENTER, 0, 50);
  lv_obj_set_style_bg_color(rebootBtn, lv_color_hex(0xCC2222), 0);
  lv_obj_set_style_radius(rebootBtn, 18, 0);
  lv_obj_add_event_cb(rebootBtn, btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *rebootLbl = lv_label_create(rebootBtn);
  lv_label_set_text(rebootLbl, LV_SYMBOL_POWER "  Reboot");
  lv_obj_set_style_text_font(rebootLbl, &lv_font_montserrat_12, 0);
  lv_obj_center(rebootLbl);

  // Version info
  lv_obj_t *ver = lv_label_create(systemScreen);
  lv_label_set_text(ver, "Star Trail v8.0");
  lv_obj_set_style_text_font(ver, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(ver, lv_color_hex(0x444444), 0);
  lv_obj_align(ver, LV_ALIGN_CENTER, 0, 85);
}

void systemview_show() {
  if (systemScreen)
    lv_scr_load_anim(systemScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void systemview_hide() {}
