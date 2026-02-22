#pragma once
#include <Arduino.h>
#include <Wire.h>

class GestureAirDraw {
public:
    struct Settings {
        uint8_t buttonPin = 2; // INPUT_PULLUP button to toggle recording
        float alpha = 0.98; // complementary filter constant
        unsigned long sampleMs = 10; // update rate ms
        float sensitivity = 1.0; // mapping sensitivity
    };
    GestureAirDraw();
    bool begin(TwoWire &wire = Wire, uint8_t i2c_addr = 0x68, const Settings &s = Settings());
    void update(); // call frequently from loop()
    bool isRecording() const;
    void startRecording();
    void stopRecording();
    String exportSVG(); // returns SVG string
    void exportSVG(Print &out); // print to Serial or SD file
    void calibrate(); // set current orientation as center
private:
    TwoWire * _wire;
    uint8_t _addr;
    Settings _s;
    unsigned long _lastMs;
    // orientation state
    float _pitch, _roll;
    float _accelPitch, _accelRoll;
    // calibration center
    float _pitch0 = 0, _roll0 = 0;
    // recording
    bool _recording = false;
    std::vector<String> _points;
    void readRawAccelGyro(float &ax, float &ay, float &az, float &gx, float &gy, float &gz);
    void pushPoint(float x, float y);
    void mapOrientationToXY(float &x, float &y);
};
