#define LGFX_USE_V1
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <Wire.h>
#include <QMC5883LCompass.h>
#include "CST816D.h"

/* --- 1. Verified LovyanGFX Config for CrowPanel 1.28" S3 --- */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST; 
      cfg.spi_mode = 0;      
      cfg.freq_write = 40000000;    
      cfg.pin_sclk = 12;         // Verified SCLK
      cfg.pin_mosi = 11;  
      cfg.pin_miso = -1; 
      cfg.pin_dc   = 42;         // Verified DC for Rotary S3 version
      _bus_instance.config(cfg);             
      _panel_instance.setBus(&_bus_instance); 
    }
    {                                       
      auto cfg = _panel_instance.config();
      cfg.pin_cs  = 10;          // Verified CS
      cfg.pin_rst = 14;       
      cfg.panel_width  = 240;
      cfg.panel_height = 240; 
      cfg.offset_x     = 0;   
      cfg.offset_y     = 0;   
      cfg.invert       = true;   
      cfg.rgb_order    = false;   
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);  
  }
};

LGFX gfx;
QMC5883LCompass compass;

// Pin Definitions
#define I2C_SDA_PIN 38
#define I2C_SCL_PIN 39
#define SCREEN_BACKLIGHT_PIN 46

/* --- 2. LVGL Drawing Logic --- */
static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 20];
lv_obj_t * heading_label;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx.startWrite();
  gfx.setAddrWindow(area->x1, area->y1, w, h);
  gfx.pushPixels((uint16_t*)&color_p->full, w * h, true);
  gfx.endWrite();
  lv_disp_flush_ready(disp);
}

/* --- 3. Setup & Main Loop --- */
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("CrowPanel Compass Starting...");

  // Force Backlight ON
  pinMode(SCREEN_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(SCREEN_BACKLIGHT_PIN, HIGH);

  // Init Display Hardware
  if (!gfx.init()) {
    Serial.println("Display Init Failed!");
  }
  gfx.fillScreen(TFT_BLACK);

  // Init LVGL
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 20);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Init I2C & Magnetometer
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  compass.init();
  
  // Simple UI
  heading_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(heading_label, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(heading_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(heading_label, LV_ALIGN_CENTER, 0, 0);
}

void loop() {
  lv_timer_handler();

  static uint32_t last_update = 0;
  if (millis() - last_update > 200) {
    compass.read();
    int x = compass.getX();
    int y = compass.getY();
    int z = compass.getZ();
    int azimuth = compass.getAzimuth();
    if (azimuth < 0) azimuth += 360;

    // 1. Print Magnetometer Values to Serial
    Serial.printf("MAG -> X:%d Y:%d Z:%d | Heading: %d\n", x, y, z, azimuth);

    // 2. Update Display
    char str[10];
    sprintf(str, "%03d", azimuth);
    lv_label_set_text(heading_label, str);

    last_update = millis();
  }
  delay(5);
}