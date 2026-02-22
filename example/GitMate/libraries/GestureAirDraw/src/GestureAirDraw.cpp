#include "GestureAirDraw.h"
#include <math.h>

#define RAD_TO_DEG 57.295779513082320876f

GestureAirDraw::GestureAirDraw(){}

bool GestureAirDraw::begin(TwoWire &wire, uint8_t i2c_addr, const Settings &s){
    _wire = &wire;
    _addr = i2c_addr;
    _s = s;
    _lastMs = millis();
    // basic I2C comms check
    _wire->begin();
    _wire->beginTransmission(_addr);
    if (_wire->endTransmission() != 0) {
        // device may not ack but we'll still proceed (some boards power differently)
        // return false;
    }
    // wake up MPU6050 (write to PWR_MGMT_1 = 0)
    _wire->beginTransmission(_addr);
    _wire->write(0x6B); // PWR_MGMT_1
    _wire->write(0x00);
    _wire->endTransmission();

    pinMode(_s.buttonPin, INPUT_PULLUP);
    calibrate();
    return true;
}

void GestureAirDraw::calibrate(){
    // take current read as center
    float ax,ay,az,gx,gy,gz;
    readRawAccelGyro(ax,ay,az,gx,gy,gz);
    // accel pitch/roll estimate
    _accelPitch = atan2f(ay, sqrtf(ax*ax + az*az)) * RAD_TO_DEG;
    _accelRoll  = atan2f(-ax, az) * RAD_TO_DEG;
    _pitch = _accelPitch;
    _roll = _accelRoll;
    _pitch0 = _pitch;
    _roll0 = _roll;
}

void GestureAirDraw::readRawAccelGyro(float &ax, float &ay, float &az, float &gx, float &gy, float &gz){
    // Read 14 bytes from MPU6050 starting at ACCEL_XOUT_H (0x3B)
    _wire->beginTransmission(_addr);
    _wire->write(0x3B);
    _wire->endTransmission(false);
    _wire->requestFrom((int)_addr, 14);
    if (_wire->available() < 14) {
        ax = ay = az = gx = gy = gz = 0;
        return;
    }
    int16_t rawAx = (_wire->read()<<8) | _wire->read();
    int16_t rawAy = (_wire->read()<<8) | _wire->read();
    int16_t rawAz = (_wire->read()<<8) | _wire->read();
    _wire->read(); _wire->read(); // temp
    int16_t rawGx = (_wire->read()<<8) | _wire->read();
    int16_t rawGy = (_wire->read()<<8) | _wire->read();
    int16_t rawGz = (_wire->read()<<8) | _wire->read();
    // scale
    ax = rawAx / 16384.0f;
    ay = rawAy / 16384.0f;
    az = rawAz / 16384.0f;
    gx = rawGx / 131.0f; // deg/s
    gy = rawGy / 131.0f;
    gz = rawGz / 131.0f;
}

void GestureAirDraw::update(){
    unsigned long now = millis();
    unsigned long dtMs = now - _lastMs;
    if (dtMs < _s.sampleMs) return;
    float dt = dtMs / 1000.0f;
    _lastMs = now;

    // button toggle
    static bool prevBtn = HIGH;
    bool btn = digitalRead(_s.buttonPin);
    if (btn == LOW && prevBtn == HIGH) {
        // pressed (active low)
        if (!_recording) startRecording(); else stopRecording();
    }
    prevBtn = btn;

    float ax,ay,az,gx,gy,gz;
    readRawAccelGyro(ax,ay,az,gx,gy,gz);

    // accel-based angles
    _accelPitch = atan2f(ay, sqrtf(ax*ax + az*az)) * RAD_TO_DEG;
    _accelRoll  = atan2f(-ax, az) * RAD_TO_DEG;

    // integrate gyro
    _pitch += gx * dt;
    _roll  += gy * dt;

    // complementary filter
    _pitch = _s.alpha * _pitch + (1.0f - _s.alpha) * _accelPitch;
    _roll  = _s.alpha * _roll  + (1.0f - _s.alpha) * _accelRoll;

    // map to 2D and record
    float x,y;
    mapOrientationToXY(x,y);
    if (_recording) pushPoint(x,y);
}

void GestureAirDraw::mapOrientationToXY(float &x, float &y){
    // map pitch,roll relative to calibration to screen coords 0..1000
    float dp = _pitch - _pitch0;
    float dr = _roll - _roll0;
    // sensitivity scaling
    x = 500.0f + dr * 10.0f * _s.sensitivity;
    y = 500.0f - dp * 10.0f * _s.sensitivity;
    // clamp
    if (x < 0) x = 0; if (x > 1000) x = 1000;
    if (y < 0) y = 0; if (y > 1000) y = 1000;
}

void GestureAirDraw::pushPoint(float x, float y){
    char buf[64];
    snprintf(buf, sizeof(buf), "%0.1f,%0.1f", x, y);
    _points.push_back(String(buf));
}

bool GestureAirDraw::isRecording() const { return _recording; }

void GestureAirDraw::startRecording(){
    _points.clear();
    _recording = true;
}

void GestureAirDraw::stopRecording(){
    _recording = false;
}

String GestureAirDraw::exportSVG(){
    String s;
    s += "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1000 1000'>\n";
    s += "<polyline fill='none' stroke='black' stroke-width='4' points='";
    for (size_t i=0;i<_points.size();++i){
        s += _points[i];
        if (i+1 < _points.size()) s += ' ';
    }
    s += "'/>\n</svg>\n";
    return s;
}

void GestureAirDraw::exportSVG(Print &out){
    out.print(exportSVG());
}
