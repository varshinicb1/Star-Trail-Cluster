#include "cst816d.h"

#define CST816D_ADDR 0x15

enum CST816D_Register {

	GestureID = 0x01,
	FingerNum,
	XposH,
	XposL,
	YposH,
	YposL,

	ChipID = 0xA7,
	ProjID,
	FwVersion,
	MotionMask,

	BPC0H = 0xB0,
	BPC0L,
	BPC1H,
	BPC1L,

	IrqPulseWidth = 0xED,
	NorScanPer,
	MotionSlAngle,
	LpScanRaw1H =0XF0,
	LpScanRaw1L,
	LpScanRaw2H,
	LpScanRaw2L,
	LpAutoWakeTime,
	LpScanTH,
	LpScanWin,
	LpScanFreq,
	LpScanIdac,
	AutoSleepTime,
	IrqCtl,
	AutoReset,
	LongPressTime,
	IOCtl,
	DisAutoSleep
};

CST816D::CST816D(uint8_t rst_pin, int8_t irq_pin, TwoWire& wire) : rst_pin(rst_pin), irq_pin(irq_pin), wire(wire){ }

bool CST816D::begin(uint16_t screen_width, uint16_t screen_height) {
    uint8_t reg_val = 0;
    wire.begin();
    pinMode(rst_pin, OUTPUT);
    if(irq_pin >= 0) pinMode(irq_pin, INPUT);
    reset();
    if(!read_reg(CST816D_Register::ChipID, &reg_val)) return false;
    if(reg_val != 0xB6) return false;
    if(!write_reg(CST816D_Register::DisAutoSleep, 0x01)) return false;
    if(!mode_point()) return false;
    if(!write_reg(CST816D_Register::IrqPulseWidth, 0x01)) return false;
    if(!write_reg(CST816D_Register::NorScanPer, 0x01)) return false;
    this->w = screen_width;
    this->h = screen_height;
    return true;
}

bool CST816D::mode_point() {
    return write_reg(CST816D_Register::IrqCtl, 0x41);
}

bool CST816D::mode_gesture() {
    if(!write_reg(CST816D_Register::IrqCtl, 0x11)) return false;
    if(!write_reg(CST816D_Register::MotionMask, 0x01)) return false;
    return true;
}

bool CST816D::mode_all() {
    return write_reg(CST816D_Register::IrqCtl, 0x71);
}

void CST816D::on_touch(std::function<void()> callback) {
    if(irq_pin < 0) return;
    this->callback = callback;
    attachInterruptParam(digitalPinToInterrupt(irq_pin), (voidFuncPtrParam)CST816D::callback_wrapper, RISING, this);
}

void CST816D::set_rotation(uint8_t rotation) {
    this->rotation = rotation % 4;
}

void CST816D::callback_wrapper(CST816D *instance) {
    if(instance->callback) instance->callback();
}

uint16_t CST816D::raw_x() {
    uint8_t xh, xl;
    read_reg(CST816D_Register::XposH, &xh);
    read_reg(CST816D_Register::XposL, &xl);
    return ((xh & 0x0f) << 8) | xl;
}
uint16_t CST816D::raw_y() {
    uint8_t yh, yl;
    read_reg(CST816D_Register::YposH, &yh);
    read_reg(CST816D_Register::YposL, &yl);
    return ((yh & 0x0f) << 8) | yl;
}

uint16_t CST816D::get_x() {
    switch(rotation) {
    case 0:
        return raw_x();
    case 1:
        return raw_y();
    case 2:
        return MAX(0, w - raw_x());
    case 3:
        return MAX(0, h - raw_y());
        break;
    default:
        return 0;
    }
}

uint16_t CST816D::get_y() {
    switch(rotation) {
    case 0:
        return raw_y();
    case 1:
        return MAX(0, w - raw_x());
    case 2:
        return MAX(0, h - raw_y());
    case 3:
        return raw_x();
    default:
        return 0;
    }
}

void CST816D::set_gesture_rotation(uint8_t rotation) {
    this->gesture_rotation = rotation;
}

enum CST816D_Gesture CST816D::get_gesture() {
    
    uint8_t g = 0;
    read_reg(CST816D_Register::GestureID, &g);
    
    if(g >= CST816D_Gesture::Right && g <= CST816D_Gesture::Up) {
        // If the gesture is a direction we rotate it that way :
        // we convert to clockwise (right, down, left, up), we subtract gesture_direction modulo 4, and we convert back to the CST816D_Gesture order.
        const uint8_t to_cw[5] = { /* unused: */ 0, 0, 2, 1, 3};
        const enum CST816D_Gesture from_cw[4] = {CST816D_Gesture::Right, CST816D_Gesture::Down, CST816D_Gesture::Left, CST816D_Gesture::Up};
        g = to_cw[g];
        g -= gesture_rotation;
        g %= 4;
        return from_cw[g];
    }
    return (enum CST816D_Gesture)g;
}


void CST816D::reset() {
    digitalWrite(rst_pin, LOW);
    delay(100);
    digitalWrite(rst_pin, HIGH);
    delay(100);
}

bool CST816D::read_reg(uint8_t reg, uint8_t *val) {
    wire.beginTransmission(CST816D_ADDR);
    if(wire.write(reg) != 1) return false;
    if(wire.endTransmission(true) != 0) return false;
    if(wire.requestFrom(CST816D_ADDR, 1) != 1) return false;
    *val = wire.read();
    return true;
}

bool CST816D::write_reg(uint8_t reg, uint8_t val) {
    wire.beginTransmission(CST816D_ADDR);
    if(wire.write(reg) != 1) return false;
    if(wire.write(val) != 1) return false;
    if(wire.endTransmission(true) != 0) return false;
    return true;
}
