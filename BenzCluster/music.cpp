#include "music.h"
#include "ble_media.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

static lv_obj_t *musicScreen = NULL;
static lv_obj_t *statusLabel = NULL;
static lv_obj_t *volLabel = NULL;
static lv_obj_t *trackLabel = NULL;
static lv_obj_t *volArc = NULL;
static lv_obj_t *playIcon = NULL;

static int volume = 50;
static bool isPlaying = false;

void music_init() {
  musicScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(musicScreen, lv_color_hex(0x000000), 0);
  lv_obj_clear_flag(musicScreen, LV_OBJ_FLAG_SCROLLABLE);

  // ===== Volume Arc (outer ring) =====
  volArc = lv_arc_create(musicScreen);
  lv_obj_set_size(volArc, 200, 200);
  lv_obj_align(volArc, LV_ALIGN_CENTER, 0, 0);
  lv_arc_set_rotation(volArc, 135);     // Start from bottom-left
  lv_arc_set_bg_angles(volArc, 0, 270); // 270 degree sweep
  lv_arc_set_range(volArc, 0, 100);
  lv_arc_set_value(volArc, volume);
  lv_obj_remove_style(volArc, NULL, LV_PART_KNOB); // No knob
  lv_obj_set_style_arc_color(volArc, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_arc_color(volArc, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(volArc, 6, LV_PART_MAIN);
  lv_obj_set_style_arc_width(volArc, 6, LV_PART_INDICATOR);
  lv_obj_clear_flag(volArc, LV_OBJ_FLAG_CLICKABLE);

  // BLE status indicator
  statusLabel = lv_label_create(musicScreen);
  lv_label_set_text(statusLabel, LV_SYMBOL_BLUETOOTH " Star Trail");
  lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x777777), 0);
  lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 30);

  // Play/pause icon - large centered (persistent for toggling)
  playIcon = lv_label_create(musicScreen);
  lv_label_set_text(playIcon, LV_SYMBOL_PLAY);
  lv_obj_set_style_text_font(playIcon, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(playIcon, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(playIcon, LV_ALIGN_CENTER, 0, -15);

  // Prev / Next arrows
  lv_obj_t *prevLbl = lv_label_create(musicScreen);
  lv_label_set_text(prevLbl, LV_SYMBOL_PREV);
  lv_obj_set_style_text_font(prevLbl, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(prevLbl, lv_color_hex(0x777777), 0);
  lv_obj_align(prevLbl, LV_ALIGN_CENTER, -55, -15);

  lv_obj_t *nextLbl = lv_label_create(musicScreen);
  lv_label_set_text(nextLbl, LV_SYMBOL_NEXT);
  lv_obj_set_style_text_font(nextLbl, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(nextLbl, lv_color_hex(0x777777), 0);
  lv_obj_align(nextLbl, LV_ALIGN_CENTER, 55, -15);

  // Track info area
  trackLabel = lv_label_create(musicScreen);
  lv_label_set_text(trackLabel,
                    "Swipe " LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " tracks");
  lv_obj_set_style_text_font(trackLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(trackLabel, lv_color_hex(0x888888), 0);
  lv_obj_align(trackLabel, LV_ALIGN_CENTER, 0, 25);

  // Volume label (shows percentage)
  volLabel = lv_label_create(musicScreen);
  lv_label_set_text(volLabel, LV_SYMBOL_VOLUME_MAX " 50%");
  lv_obj_set_style_text_font(volLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(volLabel, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(volLabel, LV_ALIGN_CENTER, 0, 60);

  // Encoder hint
  lv_obj_t *hint = lv_label_create(musicScreen);
  lv_label_set_text(hint, "Knob = Volume");
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x444444), 0);
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 80);
}

void music_show() {
  if (musicScreen)
    lv_scr_load_anim(musicScreen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 250, 0, false);
}

void music_hide() {}

void music_next_track() { ble_media_next(); }

void music_prev_track() { ble_media_prev(); }

void music_set_volume(int vol) {
  volume = constrain(vol, 0, 100);

  // Send BLE volume commands
  static int lastBleVol = 50;
  while (lastBleVol < volume) {
    ble_media_vol_up();
    lastBleVol += 5;
  }
  while (lastBleVol > volume) {
    ble_media_vol_down();
    lastBleVol -= 5;
  }
  lastBleVol = volume;

  // Update the visual arc
  lv_arc_set_value(volArc, volume);

  // Update label
  char buf[24];
  const char *icon = volume == 0 ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX;
  snprintf(buf, sizeof(buf), "%s %d%%", icon, volume);
  lv_label_set_text(volLabel, buf);
  lv_obj_align(volLabel, LV_ALIGN_CENTER, 0, 60);
}

void music_update() {
  // Update BLE connection status
  static bool lastConnected = false;
  bool connected = ble_media_connected();
  if (connected != lastConnected) {
    if (connected) {
      lv_label_set_text(statusLabel, LV_SYMBOL_BLUETOOTH " Connected");
      lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xFFFFFF), 0);
    } else {
      lv_label_set_text(statusLabel, LV_SYMBOL_BLUETOOTH " Star Trail");
      lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x777777), 0);
    }
    lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 30);
    lastConnected = connected;
  }
}

void music_toggle_play() {
  isPlaying = !isPlaying;
  lv_label_set_text(playIcon, isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  lv_obj_align(playIcon, LV_ALIGN_CENTER, 0, -15);
}
