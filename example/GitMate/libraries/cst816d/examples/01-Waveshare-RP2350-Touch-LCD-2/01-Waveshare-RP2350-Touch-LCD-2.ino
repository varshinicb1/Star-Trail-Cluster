#include <Arduino.h>
#include <Wire.h>
#include "cst816d.h"

CST816D cst816d(/* rst_pin */ 20, /* irq pin */ 29);
/* If you want to use Wire1 instead of Wire, you can specify it as a third argument :
   CST816D cst816d(20, 29, Wire1);
*/

void callback() {
    Serial.printf("%u\t%u\n", cst816d.get_x(), cst816d.get_y());
    Serial.print("gesture: ");
    switch(cst816d.get_gesture()) {
    case None:
        Serial.println("None");
        break;
    case CST816D_Gesture::Right:
        Serial.println("Right");
        break;
    case CST816D_Gesture::Down:
        Serial.println("Down");
        break;
    case CST816D_Gesture::Left:
        Serial.println("Left");
        break;
    case CST816D_Gesture::Up:
        Serial.println("Up");
        break;
    case CST816D_Gesture::Click:
        Serial.println("Click");
        break;
    case CST816D_Gesture::DoubleClick:
        Serial.println("DoubleClick");
        break;
    case CST816D_Gesture::LongPress:
        Serial.println("LongPress");
        break;
    default:
        Serial.println("unknown gesture");
    }
}

void setup() {
    Serial.begin(115200);
    Wire.setSCL(13);
    Wire.setSDA(12);
    if(!cst816d.begin(240, 320)) {
        for(;;) {
            Serial.println("failed to initialize the chip");
            delay(1000);
        }
    }
    cst816d.set_rotation(0);
    cst816d.set_gesture_rotation(3);

    cst816d.on_touch(callback);
    // Uncomment one of these to configure when the callback function will be called :
    //cst816d.mode_all(); // mode_all is the default : trigger an interrupt for both coordinates change and gestures
    //cst816d.mode_gesture(); // trigger an interrupt only for gestures
    //cst816d.mode_point(); // trigger an interrupt only for touch point coordinate change
}

void loop() {
    
}