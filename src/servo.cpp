#include "servo.h"

#include <Arduino.h>

void servoInit(uint8_t pin) {
    pinMode(pin, OUTPUT);
    analogWrite(pin, 0);
}

void servoClose(uint8_t pin) {
    analogWrite(pin, 0);
}

void servoOpen(uint8_t pin) {
    analogWrite(pin, 255);
}
