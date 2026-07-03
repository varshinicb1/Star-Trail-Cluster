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

// Persistent framebuffer for headless screenshot capture (RGB565, as stored by
// LVGL). Updated on every flush so partial redraws accumulate.
static uint16_t g_fb[240 * 240];

// When true (screenshot mode) the flush callback skips all SDL calls so the
// simulator can render headlessly with no window/display available.
static bool g_headless = false;

static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  // Copy the flushed region into the persistent framebuffer.
  lv_color_t *p = color_p;
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      if (x >= 0 && x < 240 && y >= 0 && y < 240)
        g_fb[y * 240 + x] = p->full;
      p++;
    }
  }
  if (!g_headless) {
    SDL_UpdateTexture(texture, nullptr, color_p, 240 * sizeof(lv_color_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }
  lv_disp_flush_ready(disp);
}

// Write the current framebuffer to a 24-bit BMP (no external deps). g_fb holds
// RGB565 exactly as SDL renders it, so decode directly.
static void save_bmp(const char *path) {
  const int W = 240, H = 240;
  const int rowSize = (W * 3 + 3) & ~3;  // padded to 4 bytes
  const int dataSize = rowSize * H;
  const int fileSize = 54 + dataSize;
  unsigned char hdr[54] = {0};
  hdr[0] = 'B'; hdr[1] = 'M';
  hdr[2] = fileSize; hdr[3] = fileSize >> 8; hdr[4] = fileSize >> 16; hdr[5] = fileSize >> 24;
  hdr[10] = 54;
  hdr[14] = 40;
  hdr[18] = W; hdr[19] = W >> 8;
  hdr[22] = H; hdr[23] = H >> 8;
  hdr[26] = 1; hdr[28] = 24;
  hdr[34] = dataSize; hdr[35] = dataSize >> 8; hdr[36] = dataSize >> 16; hdr[37] = dataSize >> 24;

  FILE *f = fopen(path, "wb");
  if (!f) { fprintf(stderr, "cannot write %s\n", path); return; }
  fwrite(hdr, 1, 54, f);
  unsigned char *row = (unsigned char *)malloc(rowSize);
  for (int y = H - 1; y >= 0; y--) {  // BMP is bottom-up
    memset(row, 0, rowSize);
    for (int x = 0; x < W; x++) {
      uint16_t c = g_fb[y * W + x];  // RGB565
      unsigned char r = ((c >> 11) & 0x1F) << 3;
      unsigned char g = ((c >> 5) & 0x3F) << 2;
      unsigned char b = (c & 0x1F) << 3;
      row[x * 3 + 0] = b;  // BMP stores BGR
      row[x * 3 + 1] = g;
      row[x * 3 + 2] = r;
    }
    fwrite(row, 1, rowSize, f);
  }
  free(row);
  fclose(f);
  printf("saved %s\n", path);
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
  if (!g_headless) {
    char title[64];
    snprintf(title, sizeof(title), "Star Trail - %s", widget_names[idx]);
    SDL_SetWindowTitle(window, title);
  }
}

// Render each widget with representative sample data and dump a BMP into
// `dir`. Pumps several update+timer cycles first so smoothing/animation settle.
static void run_screenshots(const char *dir) {
  struct Shot { const char *name; int idx; };
  static const Shot shots[] = {
    {"clock", 0},   {"compass", 1}, {"attitude", 2}, {"alttemp", 3},
    {"gforce", 4},  {"music", 5},   {"airplane", 6},
  };
  for (const auto &s : shots) {
    show_widget(s.idx);
    // Advance ~800ms of LVGL time (manual ticks) so the 250ms screen-load
    // animation and any value smoothing fully settle before capture.
    for (int i = 0; i < 50; i++) {
      switch (s.idx) {
        case 1: compass_update(45.0f); break;
        case 2: attitude_update(10.0f, -15.0f); break;
        case 3: alttemp_update(24.5f, 6300.0f); break;
        case 4: gforce_update(0.30f, -0.20f, 1.0f); break;
        case 6: airplane_update(5.0f, 10.0f, 120.0f); break;
        case 0: clock_update(); break;
        case 5: music_update(); break;
      }
      lv_tick_inc(16);
      lv_timer_handler();
    }
    // Force a full redraw so the whole framebuffer is current, then capture.
    lv_obj_invalidate(lv_scr_act());
    for (int i = 0; i < 4; i++) { lv_tick_inc(16); lv_timer_handler(); }
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.bmp", dir, s.name);
    save_bmp(path);

    // Capture the three alternate attitude styles too.
    if (s.idx == 2) {
      for (uint8_t st = 1; st <= 3; st++) {
        attitude_set_style(st);
        for (int i = 0; i < 20; i++) {
          attitude_update(10.0f, -15.0f);
          lv_tick_inc(16);
          lv_timer_handler();
        }
        lv_obj_invalidate(lv_scr_act());
        for (int i = 0; i < 3; i++) { lv_tick_inc(16); lv_timer_handler(); }
        snprintf(path, sizeof(path), "%s/attitude_style%d.bmp", dir, st);
        save_bmp(path);
      }
      attitude_set_style(0);
    }
  }
}

int main(int argc, char **argv) {
  // Screenshot mode renders headlessly (no window/display required).
  g_headless = (argc >= 3 && strcmp(argv[1], "--shots") == 0);

  if (g_headless) {
    // Timer subsystem only, so millis()/SDL_GetTicks() work without a display.
    SDL_Init(SDL_INIT_TIMER);
  } else {
    // Init SDL2 (interactive mode only)
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
  }

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

  // Headless screenshot mode:  simulator --shots <dir>
  if (g_headless) {
    run_screenshots(argv[2]);
    free(buf1);
    return 0;
  }

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
