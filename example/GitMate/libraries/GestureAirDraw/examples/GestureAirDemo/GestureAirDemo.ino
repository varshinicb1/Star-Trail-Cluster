#include <GestureAirDraw.h>
#include <Wire.h>

GestureAirDraw::Settings settings;
GestureAirDraw g;

void setup(){
  Serial.begin(115200);
  Wire.begin();
  settings.buttonPin = 2; // wire a button to D2 (GND when pressed)
  settings.sampleMs = 20;
  settings.alpha = 0.98;
  settings.sensitivity = 1.0;
  if (!g.begin(Wire, 0x68, settings)){
    Serial.println("Warning: IMU did not ACK. Check wiring."); 
  }
  Serial.println("GestureAirDraw ready. Press button to start/stop recording. Move the IMU to draw."); 
}

void loop(){
  g.update();
}
