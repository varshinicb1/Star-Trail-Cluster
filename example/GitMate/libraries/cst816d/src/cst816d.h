#ifndef CST816D_H
#define CST816D_H

#include <Wire.h>
#include <functional>

enum CST816D_Gesture {
	None = 0,
	Right,
	Left,
	Down,
	Up,
	Click,
	DoubleClick = 0x0b,
	LongPress=0x0c,
};

class CST816D {
public:
    CST816D(uint8_t rst_pin, int8_t irq_pin = -1, TwoWire& wire = Wire);
    // The width and height parameters are used only for rotation correction
    bool begin(uint16_t screen_width, uint16_t screen_height);
    // Trigger an interrupt only for touch point coordinate change
    bool mode_point();
    // Trigger an interrupt only for gestures
    bool mode_gesture();
    // Trigger an interrupt for both coordinates change and gestures (this is the default mode)
    bool mode_all();
    void on_touch(std::function<void()> callback);
    // Set the rotation for touch points
    void set_rotation(uint8_t rotation);
    uint16_t get_x();
    uint16_t get_y();
    // Set the rotation for gesture
    void set_gesture_rotation(uint8_t rotation);
    enum CST816D_Gesture get_gesture();

private:
    void reset();
    bool read_reg(uint8_t reg, uint8_t *val);
    bool write_reg(uint8_t reg, uint8_t val);
    uint16_t raw_x();
    uint16_t raw_y();
    uint8_t rst_pin;
    int8_t irq_pin;
    TwoWire& wire;
    std::function<void()> callback;
    static void callback_wrapper(CST816D *instance);
    uint16_t w, h;
    uint8_t rotation = 0, gesture_rotation = 0;
};

#endif