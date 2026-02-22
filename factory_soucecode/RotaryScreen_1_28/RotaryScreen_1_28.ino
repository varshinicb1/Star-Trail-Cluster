/*
 * MODULE 5: NeoPixel Test
 * Tests 5-LED ring with various effects
 */

#include <Adafruit_NeoPixel.h>

#define LED_PIN 48
#define LED_COUNT 5

Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== MODULE 5: NeoPixel Test ===\n");
  
  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();
  pixels.show();
  
  Serial.println("✓ NeoPixel initialized");
  Serial.println("\nRunning test patterns...\n");
}

void loop() {
  static int pattern = 0;
  static unsigned long lastChange = 0;
  
  if (millis() - lastChange > 3000) {
    lastChange = millis();
    pattern = (pattern + 1) % 5;
    
    switch(pattern) {
      case 0:
        Serial.println("Pattern 1: All Red");
        break;
      case 1:
        Serial.println("Pattern 2: All Green");
        break;
      case 2:
        Serial.println("Pattern 3: All Blue");
        break;
      case 3:
        Serial.println("Pattern 4: Rainbow");
        break;
      case 4:
        Serial.println("Pattern 5: Chase (Retro Pink)");
        break;
    }
  }
  
  switch(pattern) {
    case 0: // Red
      for(int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      }
      pixels.show();
      delay(100);
      break;
      
    case 1: // Green
      for(int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
      }
      pixels.show();
      delay(100);
      break;
      
    case 2: // Blue
      for(int i = 0; i < LED_COUNT; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
      }
      pixels.show();
      delay(100);
      break;
      
    case 3: // Rainbow
      for(int i = 0; i < LED_COUNT; i++) {
        int hue = (i * 65536L / LED_COUNT);
        pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(hue)));
      }
      pixels.show();
      delay(100);
      break;
      
    case 4: // Chase
      static int pos = 0;
      pixels.clear();
      pixels.setPixelColor(pos, pixels.Color(255, 0, 102)); // #FF0066
      pixels.show();
      pos = (pos + 1) % LED_COUNT;
      delay(100);
      break;
  }
}
