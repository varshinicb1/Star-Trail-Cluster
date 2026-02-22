#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <MPU6050_tockn.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>

// ======================= PINS ===========================
#define TFT_SCLK 10
#define TFT_MOSI 11
#define TFT_DC    3
#define TFT_CS    9
#define TFT_RES   14
#define TFT_BLK   46
#define LCD_PWR_EN1 1
#define LCD_PWR_EN2 2

#define MPU_SDA 38
#define MPU_SCL 39

#define LED_PIN 48
#define LED_NUM 5

#define BTN_PIN 41   // active LOW

// ================== DISPLAY SETUP ========================
Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(
  TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED, FSPI, true
);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0, true);

// ================== SENSOR ==========================
MPU6050 mpu(Wire);

// ================== RGB =============================
Adafruit_NeoPixel strip(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

// ================== CONSTANTS ======================
const int SCREEN_W = 240, SCREEN_H = 240;
const int CX = SCREEN_W / 2, CY = SCREEN_H / 2;

const int GAUGE_R = 120;
const int INNER_R = 60;

const float GYRO_FS = 250.0f;
const float ACCEL_FS_G = 2.0f;

float gyro_bias_x = 0, gyro_bias_y = 0, gyro_bias_z = 0;
float accel_bias_x = 0, accel_bias_y = 0, accel_bias_z = 0;

float smoothVal = 0.0f;

// ============= EMBLEM INFO =====================
const char *EMB_PATH = "/benz_140x140.rgb565";
const int EMB_W = 140;
const int EMB_H = 140;

// ============= LED HELPERS ======================
void ledsYellow() { for(int i=0;i<LED_NUM;i++) strip.setPixelColor(i, strip.Color(255,150,0)); strip.show(); }
void ledsRed()    { for(int i=0;i<LED_NUM;i++) strip.setPixelColor(i, strip.Color(255,0,0)); strip.show(); }
void ledsBlue()   { for(int i=0;i<LED_NUM;i++) strip.setPixelColor(i, strip.Color(0,150,255)); strip.show(); }

// ============================================================
//           DISPLAY BENZ EMBLEM FROM LITTLEFS
// ============================================================
bool drawEmblem() {
  if (!LittleFS.exists(EMB_PATH)) return false;

  File f = LittleFS.open(EMB_PATH, "r");
  if (!f) return false;

  uint8_t rowbuf[EMB_W * 2];

  int startX = CX - EMB_W/2;
  int startY = CY - EMB_H/2 - 10;

  for (int y=0; y<EMB_H; y++) {
    f.read(rowbuf, EMB_W * 2);
    for (int x=0; x<EMB_W; x++) {
      uint16_t px = (rowbuf[x*2] << 8) | rowbuf[x*2+1];
      gfx->drawPixel(startX + x, startY + y, px);
    }
  }

  f.close();
  return true;
}

// ============================================================
//                 AUTOMOTIVE GAUGE FACE
// ============================================================
void drawGaugeFace() {
  gfx->fillScreen(RGB565_BLACK);

  // outer ring
  for (int r = GAUGE_R; r > GAUGE_R - 5; r--)
    gfx->drawCircle(CX, CY, r, RGB565_WHITE);

  // ticks + numbers
  for (int deg = -120; deg <= 120; deg += 5) {
    float rad = deg * DEG_TO_RAD;

    int outer = GAUGE_R - 4;
    int inner = (deg % 30 == 0) ? (GAUGE_R - 40) : (GAUGE_R - 25);

    int x1 = CX + cos(rad) * outer;
    int y1 = CY + sin(rad) * outer;
    int x2 = CX + cos(rad) * inner;
    int y2 = CY + sin(rad) * inner;

    uint16_t col = (deg % 30 == 0) ? RGB565_WHITE : RGB565_DARKGREY;
    gfx->drawLine(x1,y1,x2,y2,col);

    if (deg % 30 == 0) {
      int val = map(deg, -120, 120, 0, 250);
      char buf[6]; sprintf(buf,"%d", val);

      gfx->setTextSize(3);
      gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);

      int tx = CX + cos(rad)*(GAUGE_R-60) - (strlen(buf)*7);
      int ty = CY + sin(rad)*(GAUGE_R-60) - 8;
      gfx->setCursor(tx,ty);
      gfx->print(buf);
    }
  }

  // benz text small
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(CX - 20, CY - GAUGE_R + 20);
  gfx->print("Benz");
}

// ============================================================
//                NEEDLE (THIN & AUTOMOTIVE)
// ============================================================
void drawNeedle(float value) {
  float angle = -120 + value * 240;
  float rad = angle * DEG_TO_RAD;

  int nx = CX + cos(rad) * (GAUGE_R - 28);
  int ny = CY + sin(rad) * (GAUGE_R - 28);

  // erase center
  gfx->fillCircle(CX, CY, INNER_R, RGB565_BLACK);

  // needle
  gfx->drawLine(CX, CY, nx, ny, gfx->color565(30,150,255));

  // cap
  gfx->fillCircle(CX, CY, 5, RGB565_WHITE);
  gfx->fillCircle(CX, CY, 3, RGB565_BLACK);
}

// ============================================================
//            FUSED SENSOR METRIC (0..1)
// ============================================================
float fusedMotion(float gx,float gy,float gz,float ax,float ay,float az) {
  float gmag = sqrt(gx*gx+gy*gy+gz*gz);
  float gnorm = constrain(gmag / GYRO_FS, 0, 1);

  float amag = sqrt(ax*ax+ay*ay+az*az);
  float a_dev = fabs(amag - 1.0f);
  float anorm = constrain(a_dev / ACCEL_FS_G, 0, 1);

  float f = 0.62f * gnorm + 0.38f * anorm;
  return constrain(f, 0, 1);
}

// ============================================================
//                CALIBRATION ROUTINE
// ============================================================
void runCalibration() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setCursor(20, 60); gfx->print("Calibration Mode");
  gfx->setCursor(20, 90); gfx->print("Hold still...");
  ledsRed();

  const int N = 200;
  float gx=0, gy=0, gz=0;
  float ax=0, ay=0, az=0;

  for (int i=0;i<N;i++) {
    mpu.update();
    gx += mpu.getGyroX();
    gy += mpu.getGyroY();
    gz += mpu.getGyroZ();
    ax += mpu.getAccX();
    ay += mpu.getAccY();
    az += mpu.getAccZ();
    delay(8);
  }

  gyro_bias_x = gx/N;
  gyro_bias_y = gy/N;
  gyro_bias_z = gz/N;

  accel_bias_x = ax/N;
  accel_bias_y = ay/N;
  accel_bias_z = az/N;

  gfx->setCursor(20, 150); gfx->print("Done. Press button");
  while (digitalRead(BTN_PIN) == HIGH) delay(10);
  delay(200);
}

// ============================================================
//                  INTRO SEQUENCE
// ============================================================
void intro() {
  gfx->fillScreen(RGB565_BLACK);
  ledsYellow();
  drawEmblem();

  gfx->setTextSize(3);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setCursor(CX - 80, CY + 80);
  gfx->print("Welcome DR");

  delay(2000);

  gfx->setTextSize(2);
  gfx->setCursor(CX - 100, CY + 110);
  gfx->print("Press button to calibrate");

  while (digitalRead(BTN_PIN) == HIGH) delay(10);
  delay(200);
}

// ============================================================
//                       SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  LittleFS.begin();

  pinMode(LCD_PWR_EN1, OUTPUT);
  pinMode(LCD_PWR_EN2, OUTPUT);
  digitalWrite(LCD_PWR_EN1, HIGH);
  digitalWrite(LCD_PWR_EN2, HIGH);

  pinMode(TFT_BLK, OUTPUT);
  analogWrite(TFT_BLK, 200);

  pinMode(BTN_PIN, INPUT_PULLUP);

  strip.begin(); strip.show();

  Wire.begin(MPU_SDA, MPU_SCL);
  mpu.begin();
  mpu.calcGyroOffsets(true);

  gfx->begin();

  intro();
  runCalibration();

  ledsBlue();
  drawGaugeFace();
}

// ============================================================
//                        LOOP
// ============================================================
void loop() {
  mpu.update();

  float gx = mpu.getGyroX() - gyro_bias_x;
  float gy = mpu.getGyroY() - gyro_bias_y;
  float gz = mpu.getGyroZ() - gyro_bias_z;

  float ax = mpu.getAccX() - accel_bias_x;
  float ay = mpu.getAccY() - accel_bias_y;
  float az = mpu.getAccZ() - accel_bias_z;

  float fused = fusedMotion(gx,gy,gz,ax,ay,az);

  smoothVal = smoothVal * 0.85f + fused * 0.15f;

  drawNeedle(smoothVal);

  ledsBlue();
  delay(20);
}