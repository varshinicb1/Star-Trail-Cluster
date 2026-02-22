#include <Arduino.h>
#include <Wire.h>
#include <GestureAirDraw.h>

GestureAirDraw::Settings s;
GestureAirDraw g;

void setup(){
    Serial.begin(115200);
    Wire.begin();
    s.buttonPin = 2;
    g.begin(Wire, 0x68, s);
    Serial.println("GestureAirDraw (PlatformIO) ready");
}
void loop(){
    g.update();
}
