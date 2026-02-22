#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>

/* ================= DISPLAY ================= */

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 80000000;
      cfg.pin_sclk = 10;
      cfg.pin_mosi = 11;
      cfg.pin_miso = -1;
      cfg.pin_dc   = 3;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = 9;
      cfg.pin_rst = 14;
      cfg.memory_width  = 240;
      cfg.memory_height = 240;
      cfg.panel_width   = 240;
      cfg.panel_height  = 240;
      cfg.invert = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

LGFX lcd;

/* ================= LVGL ================= */

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[240 * 20];

void my_disp_flush(lv_disp_drv_t *disp,
                   const lv_area_t *area,
                   lv_color_t *color_p)
{
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
  lcd.pushPixels((uint16_t *)color_p, w * h);
  lcd.endWrite();

  lv_disp_flush_ready(disp);
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.setBrightness(200);
  lcd.fillScreen(TFT_BLACK);

  lv_init();

  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 240 * 20);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /* ===== TEST UI ===== */
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "HELLO");
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
  lv_obj_center(label);

  Serial.println("LVGL OK");
}

/* ================= LOOP ================= */

void loop() {
  lv_timer_handler();   // <-- THIS IS ENOUGH
  delay(5);
}
