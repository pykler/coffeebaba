/// CoffeeBaba -> Artisan Coffee Controller
// NodeMCU + MAX6675 + SSR + 1.8" TFT 

// Reference Nodemcu gpio https://www.electronicwings.com/nodemcu/nodemcu-gpio-with-arduino-ide
// D0       -> GPIO 16  // GPIO Read/Write, no special funcs / High during Boot -> 1v after
// D1       -> GPIO 5
// D2       -> GPIO 4
// D3       -> GPIO 0   // oscillates to High while booting, needs 100ms
// D4       -> GPIO 2   // oscillates to High while booting, needs 100ms // LED_BUILTIN ?
// D5       -> GPIO 14
// D6       -> GPIO 12
// D7       -> GPIO 13
// D8       -> GPIO 15  // Low at Boot, Boot failure if Pulled High
// Below not commonly used for dev
// D9 / RX  -> GPIO 3   // Low for ~50ms, then High 
// D10 / TX -> GPIO 1   // oscillates to High while booting, needs 50ms
// D12 / S3 -> GPIO 9   // High at boot
// D13 / S2 -> GPIO 10  // High at boot

/// Compiling this sketch
//  Libraries
//   * WiFiManager
//   * max6675
//   * 
//   * 
//   * 

#define NAME        "CoffeeBaba"
#define TEMP_MAX    269       // Max temperature for system
#define SERIAL_BAUD 74880   // boot baud for my NodeMCU
#define LED_PIN     LED_BUILTIN
#define LED_LOWON   1   // ESP-01 the led is on when low voltage
#define SSR_PIN     5   // D1
#define TCP_SCK     14  // D5
#define TCPL_CS     12  // D6
#define TCP_SO      13  // D7

#include "led_controller.h"
#include <max6675.h>
#include <WiFiManager.h>

WiFiManager wm;
LedController led(LED_PIN);

void wifi_setup()
{
    Serial.println("WiFi: setup in progress ...");
    WiFi.mode(WIFI_STA);
    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    wm.setConfigPortalBlocking(false);
    wm.setSaveConfigCallback(wifi_completedCallback);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect(NAME))
    {
        Serial.println("WiFi: connected!");
        // led.fix(HIGH & ~LED_LOWON);
    }
    else 
    {
        Serial.println("WiFi: Configportal running ...");
        led.blink(300);
    }
}

void wifi_completedCallback()
{
    led.fix(HIGH & ~LED_LOWON);
}

void setup() 
{
  Serial.begin(SERIAL_BAUD);  
  Serial.println('CoffeeBaba Starting ...');
  led.setup();
  wifi_setup();
}

void loop() 
{
  led.loop();
  wm.process();
}
