# CoffeeBaba

Code to control a Coffee roasting popcorn air popper using the following hardware:

 * NodeMCU (ESP8266 ESP12E)
 * MAX6675 K-type Thermocouple
 * Solid State Relay (5V 10A Songle SSR)

You can find the NodeMCU pinout as well as the pins used for this project in
src/coffeebaba.ino

TODO: upload wiring diagram for the project

This project's firmware results in a Relay that turns on and off based on
websocket messages as well as reports on the temperature of the thermocouple
via websocket messages. The messages are formatted to work with artisan roaster
scope.

The device initially launches as an AP named CoffeeBaba, connect to it and
setup the device using the web browser at ip 192.168.4.1 to connect to your
router, once connected the device will be ready to control the roaster

## Device <-> Artisan Roaster Scope config

A sample settings file can be found under examples/artisan-settings-PID.aset
this file creates a few alarms as well as setup the websocket config for the
device. Things to change are the device's port (ip address) for you setup.

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

To run static code analysis, either use the inspect tab in `pio home` or run:

```
pio check --skip-packages
```

## Testing

To launch a mock websocket and http server to test the UI code, run:

    python scripts/websocket/server.py

Accessing the server at http://localhost:8080/index.html (by default) will display the UI.

To test the microcontroller, you can use client.py which communicates with a
websocket server and periodically turns the burner on and off.
