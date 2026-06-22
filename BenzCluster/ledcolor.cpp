#include "ledcolor.h"
#include "leds.h"
#include <Arduino.h>
#include <lvgl.h>

static lv_obj_t *ledScreen = NULL;
static lv_obj_t *colorWheel = NULL;
static lv_obj_t *patternLabel = NULL;
static lv_obj_t *previewDot = NULL;

static int currentPattern = 0;
static const char *patterns[] = {"Solid", "Breathe", "Rainbow", "Pulse"};
static const int numPatterns = 4;
static uint32_t selectedColor = 0xFFAA00;

static void colorwheel_cb(lv_event_t *e) {
  lv_obj_t *cw = lv_event_get_target(e);
  lv_color_t c = lv_colorwheel_get_rgb(cw);
  uint32_t g6 = ((uint32_t)c.ch.green_h << 3) | c.ch.green_l;
  selectedColor = ((uint32_t)c.ch.red << 19) | (g6 << 10) |
                  ((uint32_t)c.ch.blue << 3);
  lv_obj_set_style_bg_color(previewDot, c, 0);
  leds_set_color(selectedColor);
}

void ledcolor_init() {
  ledScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(ledScreen, lv_color_hex(0x0A0A0A), 0);
  lv_obj_clear_flag(ledScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Title
  lv_obj_t *title = lv_label_create(ledScreen);
  lv_label_set_text(title, "LED COLOR");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(0x888888), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

  // Color wheel
  colorWheel = lv_colorwheel_create(ledScreen, true);
  lv_obj_set_size(colorWheel, 150, 150);
  lv_obj_align(colorWheel, LV_ALIGN_CENTER, 0, -5);
  lv_colorwheel_set_rgb(colorWheel, lv_color_hex(0xFFAA00));
  lv_obj_add_event_cb(colorWheel, colorwheel_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_set_style_arc_width(colorWheel, 20, LV_PART_MAIN);

  // Preview dot (center of wheel)
  previewDot = lv_obj_create(ledScreen);
  lv_obj_set_size(previewDot, 30, 30);
  lv_obj_align(previewDot, LV_ALIGN_CENTER, 0, -5);
  lv_obj_set_style_radius(previewDot, 15, 0);
  lv_obj_set_style_bg_color(previewDot, lv_color_hex(0xFFAA00), 0);
  lv_obj_set_style_border_width(previewDot, 0, 0);
  lv_obj_clear_flag(previewDot, LV_OBJ_FLAG_CLICKABLE);

  // Pattern label (bottom)
  patternLabel = lv_label_create(ledScreen);
  lv_label_set_text(patternLabel, "Mode: Solid");
  lv_obj_set_style_text_font(patternLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(patternLabel, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(patternLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void ledcolor_show() {
  if (ledScreen)
    lv_scr_load_anim(ledScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void ledcolor_hide() {}

void ledcolor_update() {
  // Pattern cycling via encoder on this widget is handled in main sketch
}
