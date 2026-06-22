#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <lvgl.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "compass.h"
#include "attitude.h"
#include "alttemp.h"
#include "clock.h"
#include "gforce.h"
#include "music.h"
#include "airplane.h"

// Stubs for ble_media functions (called by music.cpp)
bool ble_media_connected() { return false; }
void ble_media_next() {}
void ble_media_prev() {}
void ble_media_vol_up() {}
void ble_media_vol_down() {}

// SDL display driver globals
static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;
static SDL_Texture *texture = nullptr;

// LVGL display buffer
static lv_color_t *buf1 = nullptr;
#define BUF_SIZE (240 * 240)

static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int w = area->x2 - area->x1 + 1;
  int h = area->y2 - area->y1 + 1;
  SDL_UpdateTexture(texture, nullptr, color_p, 240 * sizeof(lv_color_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
  lv_disp_flush_ready(disp);
}

static const char *widget_names[] = {
  "Clock", "Compass", "Attitude", "AltTemp", "G-Force", "Music", "Airplane"
};
static const int NUM_WIDGETS = 7;
static int current_widget = 0;

static void show_widget(int idx) {
  switch (idx) {
    case 0: clock_show(); break;
    case 1: compass_show(); break;
    case 2: attitude_show(); break;
    case 3: alttemp_show(); break;
    case 4: gforce_show(); break;
    case 5: music_show(); break;
    case 6: airplane_show(); break;
  }
  char title[64];
  snprintf(title, sizeof(title), "Star Trail - %s", widget_names[idx]);
  SDL_SetWindowTitle(window, title);
}

int main(int, char **) {
  // Init SDL2
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  window = SDL_CreateWindow("Star Trail Simulator",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    240, 240, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetLogicalSize(renderer, 240, 240);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
    SDL_TEXTUREACCESS_STREAMING, 240, 240);

  // Init LVGL
  lv_init();

  buf1 = (lv_color_t *)malloc(BUF_SIZE * sizeof(lv_color_t));
  if (!buf1) {
    fprintf(stderr, "Failed to allocate LVGL buffer\n");
    return 1;
  }

  static lv_disp_draw_buf_t draw_buf;
  lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, BUF_SIZE);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Init all widgets
  clock_init();
  compass_init();
  attitude_init();
  alttemp_init();
  gforce_init();
  music_init();
  airplane_init();

  show_widget(0);

  // Simulated sensor data
  struct { float heading, pitch, roll, temp, alt_ft, ax, ay, az; } sim = {};
  sim.az = 1.0f;

  Uint32 last_update = SDL_GetTicks();
  Uint32 last_tick = SDL_GetTicks();
  bool running = true;

  while (running) {
    Uint32 now = SDL_GetTicks();

    // SDL events
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) {
        running = false;
      } else if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.sym) {
          case SDLK_RIGHT: case SDLK_DOWN:
            current_widget = (current_widget + 1) % NUM_WIDGETS;
            show_widget(current_widget);
            break;
          case SDLK_LEFT: case SDLK_UP:
            current_widget = (current_widget - 1 + NUM_WIDGETS) % NUM_WIDGETS;
            show_widget(current_widget);
            break;
          case SDLK_ESCAPE:
            running = false;
            break;
        }
      }
    }

    // Update simulated data every 50ms
    if (now - last_update >= 50) {
      last_update = now;
      float t = now / 1000.0f;
      sim.heading = fmodf(sim.heading + 0.5f, 360.0f);
      sim.pitch = sinf(t * 0.5f) * 5.0f;
      sim.roll = cosf(t * 0.4f) * 5.0f;
      sim.temp = 25.0f + sinf(t * 0.1f) * 2.0f;
      sim.alt_ft = 6300.0f + sinf(t * 0.2f) * 500.0f;
      sim.ax = sinf(t * 0.7f) * 0.3f;
      sim.ay = cosf(t * 0.6f) * 0.3f;
      sim.az = 1.0f + sinf(t * 0.8f) * 0.2f;

      compass_update(sim.heading);
      attitude_update(sim.pitch, sim.roll);
      alttemp_update(sim.temp, sim.alt_ft);
      clock_update();
      gforce_update(sim.ax, sim.ay, sim.az);
      music_update();
      airplane_update(sim.pitch, sim.roll, sim.heading);
    }

    lv_tick_inc(5);
    lv_timer_handler();
    SDL_Delay(5);
  }

  free(buf1);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
