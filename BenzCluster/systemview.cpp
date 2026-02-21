#include "systemview.h"
#include <Arduino.h>
#include <lvgl.h>

static lv_obj_t *systemScreen = NULL;
static lv_obj_t *rebootBtn = NULL;

static void btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    if (obj == rebootBtn) {
      Serial.println("[SYS] Reboot -> Restarting...");
      delay(500);
      ESP.restart();
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
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -50);

  // Reboot button
  rebootBtn = lv_btn_create(systemScreen);
  lv_obj_set_size(rebootBtn, 140, 42);
  lv_obj_align(rebootBtn, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_bg_color(rebootBtn, lv_color_hex(0xCC2222), 0);
  lv_obj_set_style_radius(rebootBtn, 21, 0);
  lv_obj_add_event_cb(rebootBtn, btn_event_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *rebootLbl = lv_label_create(rebootBtn);
  lv_label_set_text(rebootLbl, LV_SYMBOL_POWER "  Reboot");
  lv_obj_set_style_text_font(rebootLbl, &lv_font_montserrat_14, 0);
  lv_obj_center(rebootLbl);

  // Flash hint
  lv_obj_t *hint = lv_label_create(systemScreen);
  lv_label_set_text(hint, "Flash: Hold BOOT + RESET");
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x555555), 0);
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 60);

  // Version info
  lv_obj_t *ver = lv_label_create(systemScreen);
  lv_label_set_text(ver, "Star Trail v6.0");
  lv_obj_set_style_text_font(ver, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(ver, lv_color_hex(0x444444), 0);
  lv_obj_align(ver, LV_ALIGN_CENTER, 0, 85);
}

void systemview_show() {
  if (systemScreen)
    lv_scr_load_anim(systemScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void systemview_hide() {}
