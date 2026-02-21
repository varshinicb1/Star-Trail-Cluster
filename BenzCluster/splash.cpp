#include "splash.h"
#include "benz_logo.h"
#include "config.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

static lv_obj_t *splashScreen = NULL;
static lv_obj_t *welcomeLabel = NULL;
static lv_obj_t *progressBar = NULL;

// Star trail particles
#define NUM_STARS 20
static lv_obj_t *stars[NUM_STARS];

static void star_orbit_cb(void *obj, int32_t angle) {
  lv_obj_t *s = (lv_obj_t *)obj;
  int idx = (int)(intptr_t)lv_obj_get_user_data(s);
  float baseRadius = 72 + (idx % 4) * 7;
  float rad = angle * 3.14159f / 180.0f;
  int x = (int)(cosf(rad) * baseRadius);
  int y = (int)(sinf(rad) * baseRadius);
  lv_obj_align(s, LV_ALIGN_CENTER, x, y - 15);
  int opa = 60 + (int)(195.0f * (1.0f + sinf(rad)) / 2.0f);
  lv_obj_set_style_opa(s, opa, 0);
}

void splash_show() {
  splashScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(splashScreen, lv_color_hex(0x000000), 0);
  lv_scr_load(splashScreen);

  // === Star Trail Particles ===
  uint32_t starColors[] = {0xFFFFFF, 0xDDDDFF, 0xBBBBFF, 0xAAAADD, 0x8888CC};
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i] = lv_obj_create(splashScreen);
    int sz = 2 + (i % 3);
    lv_obj_set_size(stars[i], sz, sz);
    lv_obj_set_style_radius(stars[i], sz, 0);
    lv_obj_set_style_bg_color(stars[i], lv_color_hex(starColors[i % 5]), 0);
    lv_obj_set_style_border_width(stars[i], 0, 0);
    lv_obj_set_style_opa(stars[i], 0, 0);
    lv_obj_set_user_data(stars[i], (void *)(intptr_t)i);

    lv_anim_t sa;
    lv_anim_init(&sa);
    lv_anim_set_var(&sa, stars[i]);
    int startAngle = (i * (360 / NUM_STARS));
    lv_anim_set_values(&sa, startAngle, startAngle + 360);
    lv_anim_set_time(&sa, 2800 + (i % 4) * 300);
    lv_anim_set_repeat_count(&sa, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&sa, star_orbit_cb);
    lv_anim_start(&sa);
  }

  // === Official Benz Logo Image (120x120) ===
  lv_obj_t *logoImg = lv_img_create(splashScreen);
  lv_img_set_src(logoImg, &benz_logo);
  lv_obj_align(logoImg, LV_ALIGN_CENTER, 0, -15);

  // Brand text
  lv_obj_t *brandLabel = lv_label_create(splashScreen);
  lv_label_set_text(brandLabel, "STAR TRAIL");
  lv_obj_set_style_text_font(brandLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(brandLabel, lv_color_hex(0xC0C0C0), 0);
  lv_obj_set_style_text_letter_space(brandLabel, 6, 0);
  lv_obj_align(brandLabel, LV_ALIGN_CENTER, 0, 80);

  lv_obj_t *subBrand = lv_label_create(splashScreen);
  lv_label_set_text(subBrand, "Mercedes-Benz");
  lv_obj_set_style_text_font(subBrand, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(subBrand, lv_color_hex(0x666666), 0);
  lv_obj_set_style_text_letter_space(subBrand, 2, 0);
  lv_obj_align(subBrand, LV_ALIGN_CENTER, 0, 95);

  // Progress bar (3s gyro cal)
  progressBar = lv_bar_create(splashScreen);
  lv_obj_set_size(progressBar, 140, 4);
  lv_obj_align(progressBar, LV_ALIGN_CENTER, 0, 110);
  lv_obj_set_style_bg_color(progressBar, lv_color_hex(0x222222), LV_PART_MAIN);
  lv_obj_set_style_bg_color(progressBar, lv_color_hex(0xFFFFFF),
                            LV_PART_INDICATOR);
  lv_obj_set_style_radius(progressBar, 2, LV_PART_MAIN);
  lv_obj_set_style_radius(progressBar, 2, LV_PART_INDICATOR);
  lv_bar_set_range(progressBar, 0, 100);
  lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, progressBar);
  lv_anim_set_values(&a, 0, 100);
  lv_anim_set_time(&a, 3000);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
    lv_bar_set_value((lv_obj_t *)obj, v, LV_ANIM_ON);
  });
  lv_anim_start(&a);

  lv_timer_handler();
}

void splash_welcome(const char *name) {
  lv_obj_clean(splashScreen);
  lv_obj_set_style_bg_color(splashScreen, lv_color_hex(0x000000), 0);

  welcomeLabel = lv_label_create(splashScreen);
  char buf[64];
  snprintf(buf, sizeof(buf), "Welcome, %s", name);
  lv_label_set_text(welcomeLabel, buf);
  lv_obj_set_style_text_font(welcomeLabel, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(welcomeLabel, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(welcomeLabel, LV_ALIGN_CENTER, 0, -20);

  lv_obj_t *subLabel = lv_label_create(splashScreen);
  lv_label_set_text(subLabel, "Star Trail Ready");
  lv_obj_set_style_text_font(subLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(subLabel, lv_color_hex(0x808080), 0);
  lv_obj_align(subLabel, LV_ALIGN_CENTER, 0, 30);

  lv_obj_set_style_opa(welcomeLabel, 0, 0);
  lv_obj_set_style_opa(subLabel, 0, 0);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, welcomeLabel);
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_time(&a, 800);
  lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
  });
  lv_anim_start(&a);

  lv_anim_set_var(&a, subLabel);
  lv_anim_set_delay(&a, 400);
  lv_anim_start(&a);

  unsigned long start = millis();
  while (millis() - start < 1500) {
    lv_timer_handler();
    delay(5);
  }
}

void splash_hide() {
  if (splashScreen) {
    lv_obj_del(splashScreen);
    splashScreen = NULL;
  }
}
