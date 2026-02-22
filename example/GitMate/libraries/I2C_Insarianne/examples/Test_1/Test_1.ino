#include <I2C_Insarianne.h>

MPU6050 mpu;

void setup() {
    Serial.begin(9600);
    Serial.print("Version Librairie : "); Serial.println(VERSION_LIB);
    Wire.begin();

    mpu.begin();
    Serial.println(mpu.test());
}

void loop() {
}