# CoffeeBaba

Code to control a Coffee roasting popcorn air popper using the following hardware:

 * NodeMCU (ESP8266)
 * MAX6675 Thermocouple
 * Solid State Relay

You can find the NodeMCU pinout as well as the pins used for this project in
coffeebaba.ino 

## Compiling

This project uses platformio, so first have that installed and then ...

to download all dependencies and compile:

```
pio run
```

to upload firmware to device:

```
pio run -t upload

# you may need to specify the upload port if its not detected automatically
pio run -t upload --upload-port /dev/cu...example
```
