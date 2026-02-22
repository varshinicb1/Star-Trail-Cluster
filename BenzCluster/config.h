#ifndef CONFIG_H
#define CONFIG_H

// ============== Hardware Pins ==============
// Display (SPI - GC9A01) - GitMate config
#define TFT_SCLK 10
#define TFT_MOSI 11
#define TFT_DC 3
#define TFT_CS 9
#define TFT_RST 14
#define TFT_BL 46

// Touch (I2C - CST816D on Wire1)
#define TP_SDA 6
#define TP_SCL 7
#define TP_RST 13
#define TP_INT 5

// External Sensors (I2C - Wire)
#define I2C_SDA 38
#define I2C_SCL 39

// Rotary Encoder
#define ENC_A 45
#define ENC_B 42
#define ENC_SW 41

// NeoPixel LEDs
#define LED_PIN 48
#define LED_COUNT 5

// ============== Sensor Addresses ==============
#define MPU9250_ADDR 0x68
#define QMC5883L_ADDR 0x0D // External magnetometer via MPU I2C master
#define BME280_ADDR 0x76

// ============== Location Config ==============
// Bangalore, India
#define MAGNETIC_DECLINATION -1.09f // Bangalore: -1°5' West

// Approximate sea level pressure (hPa) - will be updated via API
#define DEFAULT_SEA_LEVEL_PRESSURE 1013.25f

// Bangalore elevation for reference
#define LOCAL_ELEVATION_METERS 920.0f
#define LOCAL_ELEVATION_FEET 3018.0f

// ============== WiFi Config ==============
#define WIFI_SSID "V"
#define WIFI_PASSWORD "varshu99"

// OpenWeatherMap API for sea-level pressure
#define WEATHER_API_KEY "" // Optional: Add your API key
#define WEATHER_LAT "12.9716"
#define WEATHER_LON "77.5946"

// NTP Server
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 19800 // IST = UTC+5:30
#define DST_OFFSET_SEC 0

// ============== Display Config ==============
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

// ============== Calibration ==============
#define CALIBRATION_FILE "/calibration/mag.json"
#define PRESSURE_FILE "/calibration/pressure.json"

#endif // CONFIG_H
