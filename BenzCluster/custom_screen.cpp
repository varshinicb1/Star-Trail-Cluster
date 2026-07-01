#include "custom_screen.h"

#include <Arduino.h>
#include <lvgl.h>

#include "custom_widget.h"

static lv_obj_t *customScreen = nullptr;
static lv_obj_t *canvas = nullptr;      // element host container
static lv_obj_t *hintLabel = nullptr;   // shown when no layout exists
static CwLayout layout;
static bool hasLayout = false;

// Rebuild the on-screen element tree from `layout`.
static void rebuild() {
  if (canvas) {
    lv_obj_del(canvas);
    canvas = nullptr;
  }
  canvas = lv_obj_create(customScreen);
  lv_obj_set_size(canvas, CW_SCREEN, CW_SCREEN);
  lv_obj_align(canvas, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(canvas, 0, 0);
  lv_obj_set_style_pad_all(canvas, 0, 0);
  lv_obj_set_style_radius(canvas, 0, 0);

  if (hasLayout && layout.count > 0) {
    lv_obj_clear_flag(hintLabel ? hintLabel : canvas, LV_OBJ_FLAG_HIDDEN);
    if (hintLabel) lv_obj_add_flag(hintLabel, LV_OBJ_FLAG_HIDDEN);
    cw_render(canvas, &layout);
  } else {
    lv_obj_set_style_bg_color(canvas, lv_color_hex(0x0A0A0C), 0);
    if (!hintLabel) {
      hintLabel = lv_label_create(customScreen);
      lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x6A6C72), 0);
      lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_14, 0);
      lv_obj_align(hintLabel, LV_ALIGN_CENTER, 0, 0);
    }
    lv_label_set_text(hintLabel,
                      "No custom face\nDesign one in the app");
    lv_obj_set_style_text_align(hintLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(hintLabel, LV_OBJ_FLAG_HIDDEN);
  }
}

void custom_screen_init() {
  customScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(customScreen, lv_color_hex(0x0A0A0C), 0);
  lv_obj_clear_flag(customScreen, LV_OBJ_FLAG_SCROLLABLE);

  hasLayout = cw_load(&layout);
  rebuild();
}

void custom_screen_show() {
  if (customScreen)
    lv_scr_load_anim(customScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void custom_screen_hide() {}

void custom_screen_update() {
  if (hasLayout && layout.count > 0) cw_update(&layout);
}

void custom_screen_reload() {
  hasLayout = cw_load(&layout);
  rebuild();
  Serial.printf("[CUSTOM] reloaded layout: %s (%d elements)\n",
                hasLayout ? layout.name : "none",
                hasLayout ? layout.count : 0);
}
