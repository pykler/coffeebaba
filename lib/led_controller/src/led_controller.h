// Led Controller Library, library to help blink, keep on or turn off an led

#ifndef led_controller_h
#define led_controller_h

class LedController
{
    public:
        LedController(int);
        void setup();
        void loop();
        void blink(unsigned long);
        void fix(byte);
        void displayState();
        int pin;
        byte state;
        unsigned long speed;
        unsigned long _time;
};

#endif
