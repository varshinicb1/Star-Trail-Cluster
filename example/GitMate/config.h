#ifndef CONFIG_H
#define CONFIG_H

// ===========================
// HARDWARE PIN CONFIGURATION
// (Extracted from existing example code)
// ===========================

// Rotary Encoder Pins
#define ENCODER_A_PIN 45 // CLK pin
#define ENCODER_B_PIN 42 // DT pin
#define SWITCH_PIN 41    // Press button (with interrupt)

// LED Configuration
#define LED_PIN 48         // RGB LED data pin (NeoPixel WS2812)
#define LED_NUM 5          // Number of LEDs
#define LED_BRIGHTNESS 25  // Default brightness (0-255)
#define POWER_LIGHT_PIN 40 // Power indicator LED

// Power Pins
#define POWER_PIN_1 1 // Power output pin 1
#define POWER_PIN_2 2 // Power output pin 2

// Display SPI Pins (ST7789/GC9A01 - 240x240 Round)
#define TFT_SCLK 10             // SPI Clock
#define TFT_MOSI 11             // SPI MOSI
#define TFT_DC 3                // Data/Command
#define TFT_CS 9                // Chip Select
#define TFT_RST 14              // Reset
#define SCREEN_BACKLIGHT_PIN 46 // Backlight (PWM controlled)

// Display Configuration
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define SPI_FREQUENCY 80000000      // 80 MHz write
#define SPI_READ_FREQUENCY 20000000 // 20 MHz read

// Touch Screen I2C Pins (CST816D)
#define TP_I2C_SDA_PIN 6      // Touch SDA
#define TP_I2C_SCL_PIN 7      // Touch SCL
#define TP_INT 5              // Touch interrupt
#define TP_RST 13             // Touch reset
#define I2C_ADDR_CST816D 0x15 // Touch I2C address

// Additional I2C Bus
#define I2C_SDA_PIN 38 // Generic I2C SDA
#define I2C_SCL_PIN 39 // Generic I2C SCL

// ===========================
// NETWORK CONFIGURATION
// ===========================

// WiFi Settings (CHANGE THESE!)
#define WIFI_SSID "V"
#define WIFI_PASSWORD "varshu99"
#define WIFI_CONNECT_TIMEOUT 100000 // 10 seconds

// Desktop Agent Settings
#define AGENT_IP "10.200.106.44" // Your WiFi IP address from ipconfig
#define AGENT_PORT 8888
#define HTTP_TIMEOUT 10000   // 10 seconds
#define RECONNECT_DELAY 5000 // 5 seconds between reconnect attempts

// Pairing Token (generated on first run, stored in preferences)
#define PAIRING_TOKEN_LENGTH 16

// ===========================
// TIMING CONSTANTS
// ===========================

// Encoder Debouncing
#define DEBOUNCE_TIME 20      // milliseconds
#define DOUBLE_CLICK_TIME 300 // milliseconds for double-click detection
#define LONG_PRESS_TIME 1000  // milliseconds for long press

// UI Update Rates
#define UI_UPDATE_INTERVAL 50       // milliseconds (20 FPS)
#define STATUS_UPDATE_INTERVAL 1000 // milliseconds (1 Hz)

// LED Animation
#define LED_BREATHING_PERIOD 2000 // milliseconds for breathing cycle
#define LED_PULSE_PERIOD 1000     // milliseconds for pulse cycle
#define LED_FAST_PULSE_PERIOD 500 // milliseconds for fast pulse

// Backlight PWM
#define PWM_FREQ 5000 // 5 kHz
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8 // 8-bit (0-255)

// ===========================
// GIT COMMAND CONFIGURATION
// ===========================

#define MAX_COMMANDS 14       // Total number of Git commands
#define COMMAND_TIMEOUT 30000 // 30 seconds max for command execution

// ===========================
// SYSTEM CONFIGURATION
// ===========================

#define SERIAL_BAUD 115200
#define BOOT_TIMEOUT 3000 // 3 seconds boot time target

// Task Priorities (FreeRTOS)
#define TASK_PRIORITY_ENCODER 2
#define TASK_PRIORITY_TOUCH 2
#define TASK_PRIORITY_LED 1
#define TASK_PRIORITY_NETWORK 3
#define TASK_PRIORITY_UI 2

// Task Stack Sizes
#define TASK_STACK_ENCODER 4096
#define TASK_STACK_TOUCH 2048
#define TASK_STACK_LED 2048
#define TASK_STACK_NETWORK 8192
#define TASK_STACK_UI 8192

// ===========================
// DEBUG CONFIGURATION
// ===========================

// Uncomment to enable debug output
// #define DEBUG_ENCODER
// #define DEBUG_TOUCH
// #define DEBUG_NETWORK
// #define DEBUG_LED
// #define DEBUG_UI

#ifdef DEBUG_ENCODER
#define DEBUG_PRINT_ENCODER(x) Serial.print(x)
#define DEBUG_PRINTLN_ENCODER(x) Serial.println(x)
#else
#define DEBUG_PRINT_ENCODER(x)
#define DEBUG_PRINTLN_ENCODER(x)
#endif

#ifdef DEBUG_TOUCH
#define DEBUG_PRINT_TOUCH(x) Serial.print(x)
#define DEBUG_PRINTLN_TOUCH(x) Serial.println(x)
#else
#define DEBUG_PRINT_TOUCH(x)
#define DEBUG_PRINTLN_TOUCH(x)
#endif

#ifdef DEBUG_NETWORK
#define DEBUG_PRINT_NETWORK(x) Serial.print(x)
#define DEBUG_PRINTLN_NETWORK(x) Serial.println(x)
#else
#define DEBUG_PRINT_NETWORK(x)
#define DEBUG_PRINTLN_NETWORK(x)
#endif

#ifdef DEBUG_LED
#define DEBUG_PRINT_LED(x) Serial.print(x)
#define DEBUG_PRINTLN_LED(x) Serial.println(x)
#else
#define DEBUG_PRINT_LED(x)
#define DEBUG_PRINTLN_LED(x)
#endif

#ifdef DEBUG_UI
#define DEBUG_PRINT_UI(x) Serial.print(x)
#define DEBUG_PRINTLN_UI(x) Serial.println(x)
#else
#define DEBUG_PRINT_UI(x)
#define DEBUG_PRINTLN_UI(x)
#endif

#endif // CONFIG_H
