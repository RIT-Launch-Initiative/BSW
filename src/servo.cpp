#include "servo.h"

#include <Arduino.h>
#include <Servo.h>

static Servo servo;

void servoInit(uint8_t pin) {
    servo.attach(pin);
}

void servoClose(uint8_t pin) {
    servo.write(0);
}

void servoOpen(uint8_t pin) {
    servo.write(28);
}
