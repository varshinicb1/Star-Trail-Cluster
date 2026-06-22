#include "display.h"
#include "config.h"
#include <LovyanGFX.hpp>
#include <Wire.h>

// ============== CST816D Touch Controller ==============
#define CST816D_ADDR 0x15

class CST816D {
public:
  void begin() {
    pinMode(TP_RST, OUTPUT);
    digitalWrite(TP_RST, LOW);
    delay(10);
    digitalWrite(TP_RST, HIGH);
    delay(50);

    // Enable gesture mode
    Wire1.beginTransmission(CST816D_ADDR);
    Wire1.write(0xFE);
    Wire1.write(0xFF);
    Wire1.endTransmission();
  }

  bool getTouch(uint16_t *x, uint16_t *y, uint8_t *gesture) {
    Wire1.beginTransmission(CST816D_ADDR);
    Wire1.write(0x01);
    Wire1.endTransmission(false);
    Wire1.requestFrom(CST816D_ADDR, 6);

    if (Wire1.available() >= 6) {
      *gesture = Wire1.read();
      uint8_t touchPoints = Wire1.read();
      uint8_t xHigh = Wire1.read();
      uint8_t xLow = Wire1.read();
      uint8_t yHigh = Wire1.read();
      uint8_t yLow = Wire1.read();

      *x = ((xHigh & 0x0F) << 8) | xLow;
      *y = ((yHigh & 0x0F) << 8) | yLow;

      return (touchPoints > 0);
    }
    return false;
  }
};

// ============== LovyanGFX Display Driver ==============
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 80000000; // 80MHz per GitMate config
      cfg.freq_read = 20000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = -1;
      cfg.pin_dc = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = TFT_CS;
      cfg.pin_rst = TFT_RST;
      cfg.pin_busy = -1;
      cfg.memory_width = 240;
      cfg.memory_height = 240;
      cfg.panel_width = 240;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

// ============== Global Instances ==============
static LGFX gfx;
static CST816D touch;

// LVGL display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// Encoder state
static volatile int8_t encoderDelta = 0;
static volatile bool buttonPressed = false;
static volatile int8_t lastA = 0;

// ============== LVGL Display Flush ==============
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                          lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  gfx.startWrite();
  gfx.setAddrWindow(area->x1, area->y1, w, h);
  gfx.pushPixels((uint16_t *)&color_p->full, w * h, true);
  gfx.endWrite();

  lv_disp_flush_ready(disp);
}

// ============== LVGL Touch Read ==============
static void my_touchpad_read(lv_indev_drv_t *indev_driver,
                             lv_indev_data_t *data) {
  uint16_t touchX, touchY;
  uint8_t gesture;

  if (touch.getTouch(&touchX, &touchY, &gesture)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

// ============== Encoder ISR ==============
void IRAM_ATTR encoderISR() {
  int8_t currentA = digitalRead(ENC_A);
  if (currentA != lastA && currentA == 1) {
    if (digitalRead(ENC_B) != currentA) {
      encoderDelta++;
    } else {
      encoderDelta--;
    }
  }
  lastA = currentA;
}

void IRAM_ATTR buttonISR() {
  // ISR kept for future tap events if needed
}

// ============== Public Functions ==============
void display_init() {
  // Force Backlight ON first (like working example)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initialize display
  if (!gfx.init()) {
    Serial.println("[DISPLAY] GFX init failed!");
  }
  gfx.fillScreen(TFT_BLACK);

  // Initialize touch
  touch.begin();

  // Initialize encoder
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_SW), buttonISR, FALLING);

  // Initialize LVGL
  lv_init();

  // Allocate display buffers in PSRAM
  size_t buf_size = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t);
  buf1 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
  buf2 = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

  if (!buf1 || !buf2) {
    Serial.println("[DISPLAY] Failed to allocate LVGL buffers!");
    // Fallback to smaller buffer
    buf1 = (lv_color_t *)malloc(SCREEN_WIDTH * 40 * sizeof(lv_color_t));
    buf2 = NULL;
  }

  lv_disp_draw_buf_init(&draw_buf, buf1, buf2,
                        buf2 ? SCREEN_WIDTH * SCREEN_HEIGHT
                             : SCREEN_WIDTH * 40);

  // Register display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Register touch input
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

void display_set_brightness(int percent) {
  int pwm = map(constrain(percent, 0, 100), 0, 100, 0, 255);
  analogWrite(TFT_BL, pwm);
}

uint8_t touch_get_gesture() {
  static uint32_t lastGestureTime = 0;
  static uint8_t lastGesture = GESTURE_NONE;

  // Debounce - only check every 100ms
  if (millis() - lastGestureTime < 100) {
    return GESTURE_NONE;
  }

  uint16_t x, y;
  uint8_t gesture;

  if (touch.getTouch(&x, &y, &gesture)) {
    // Only accept specific gesture codes
    if (gesture >= GESTURE_SWIPE_UP && gesture <= GESTURE_SWIPE_RIGHT) {
      // Don't repeat same gesture within 500ms
      if (gesture != lastGesture || millis() - lastGestureTime > 500) {
        lastGesture = gesture;
        lastGestureTime = millis();
        return gesture;
      }
    }
  } else {
    // No touch - reset last gesture
    lastGesture = GESTURE_NONE;
  }

  return GESTURE_NONE;
}

bool touch_get_point(uint16_t *x, uint16_t *y) {
  uint8_t gesture;
  return touch.getTouch(x, y, &gesture);
}

int8_t encoder_read() {
  int8_t delta = encoderDelta;
  encoderDelta = 0;
  return delta;
}

bool encoder_button_pressed() {
  // Read live pin state for continuous hold detection
  // ENC_SW is pulled up, so LOW = pressed
  static uint32_t lastDebounce = 0;
  static bool lastState = false;
  bool raw = (digitalRead(ENC_SW) == LOW);
  if (raw != lastState) {
    lastDebounce = millis();
  }
  lastState = raw;
  // 50ms debounce
  if (millis() - lastDebounce > 50) {
    return raw;
  }
  return false;
}
