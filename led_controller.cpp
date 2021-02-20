#include "Arduino.h"
#include "led_controller.h"

LedController::LedController(int led_pin)
{
    pin = led_pin;
    state = 0;
    speed = 0;
    _time = millis();

}

void LedController::setup()
{
    pinMode(pin, OUTPUT);
}

void LedController::loop()
{
    if (speed > 0)
    {
        unsigned long new_time = millis();
        if (new_time - _time > speed)
        {
            state = ~state;
            _time = new_time;
        }
        displayState();
    }
}

void LedController::blink(unsigned long new_speed)
{
    speed = new_speed;
}

void LedController::fix(byte new_state)
{
    speed = 0;
    state = new_state;
    displayState();
}

void LedController::displayState()
{
    digitalWrite(pin, state);
}
