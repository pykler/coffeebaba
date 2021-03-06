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

// SSR Relay (Low ON)
//  Normally Closed configuration (NC):
//      HIGH signal – current is flowing
//      LOW signal – current is not flowing
//  Normally Open configuration (NO):
//      HIGH signal – current is not flowing
//      LOW signal – current is flowing

/// Compiling this sketch
//  Libraries
//   * WiFiManager
//   * max6675
//   * 
//   * 
//   * 

#define DEBUG       0  // Enable / Disable debug

#define NAME        "CoffeeBaba"
#define SERIAL_BAUD 74880   // boot baud for my NodeMCU
// LED
#define LED_PIN     LED_BUILTIN
#define LED_LOWON   1   // ESP-01 the led is on when low voltage
// Solid State Relay
#define SSR_PIN     5   // D1
#define SSR_NC      0   // SSR Normally Closed = 1 / Normally Open = 0 (SSR is low on)
// Thermocouple
#define TCP_SCK     14  // D5 Serial Clock Input
#define TCP_SO      12  // D6 Serial Data Output
#define TCP_CS      15  // D8 Chip Select (low to enable serial interface)
#define TCP_POLL    250 // ms to wait between thermocouple polling
#define TCP_TMAX    269 // Max temperature for system in Deg Celcius (thermalbreak)

#include <Arduino.h>
#include "led_controller.h"
#include <max6675.h>
#include <WiFiManager.h>
#define WEBSERVER_H  // to avoid conflict between ESPAsyncWebServer and WifiManager
#include "coffeebaba_web.h"

#if DEBUG
#define debug_print(msg) Serial.print(msg);
#define debug_println(msg) Serial.println(msg);
#else
#define debug_print(msg)
#define debug_println(msg)
#endif

WiFiManager wm;
LedController led(LED_PIN);
CoffeeBabaWeb server(80, "/WebSocket");
MAX6675 thermocouple(TCP_SCK, TCP_CS, TCP_SO);
unsigned long thermocouple_poll = 0;
byte burner = 0;

void wifi_setup()
{
    Serial.println(F("WiFi: setup in progress ..."));
    WiFi.mode(WIFI_STA);

    wm.setConfigPortalBlocking(false);
    wm.setSaveConfigCallback(wifi_completedCallback);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect(NAME))
    {
        Serial.println(F("WiFi: connected!"));
        // note that wifi_completedCallback will not be called in this case
    }
    else 
    {
        Serial.println(F("WiFi: Configportal running ..."));
        led.blink(300);
    }
}

void wifi_reset()
{
    Serial.println(F("WiFi: Resetting settings"));
    wm.resetSettings();
}

void wifi_completedCallback()
{
    Serial.println(F("WiFi: completed callback"));
    // led.fix(HIGH & ~LED_LOWON);
    // reboot so that we get the webserver
    reboot();
}

float thermocouple_c_to_f(float deg_c)
{
    return deg_c * 9.0 / 5.0 + 32;
}

void thermocouple_loop()
{
    unsigned long now = millis();
    if (now - thermocouple_poll < TCP_POLL)
    {
        // should wait atleast 250ms between reads
        return;
    }
    thermocouple_poll = now;
    server.temp = thermocouple.readCelsius();
    debug_print("C = ");
    debug_println(server.temp);
    debug_print("F = ");
    debug_println(thermocouple_c_to_f(server.temp));
}

void ssr_setup()
{
    Serial.println(F("Setting up SS Relay"));
    pinMode(SSR_PIN, OUTPUT);
    ssr_open(); // for safety kill the current
}

void ssr_close() // close the circuit
{
    Serial.print("SSR circuit closed ... pin_value: ");
    // High if in normally closed mode
    byte pin_value = LOW | SSR_NC;
    digitalWrite(SSR_PIN, pin_value);
    Serial.println(pin_value);
}

void ssr_open() // open the ssr circuit
{
    Serial.print("SSR circuit open ... pin_value: ");
    // Low if in normally closed mode
    byte pin_value = HIGH & ~SSR_NC;
    digitalWrite(SSR_PIN, pin_value);
    Serial.println(pin_value);
}

void burner_loop()
{
    if (server.burner != burner)
    {
        burner = server.burner;
        if (burner) ssr_close();
        else ssr_open();
    }
}

void reboot()
{
    Serial.println(F("Rebooting ..."));
    delay(100);
    ESP.restart();
}

void admin_loop()
{
    if (ADMIN_RESET == server.admin_action) { 
        // factory reset
        wifi_reset();
        // reboot after factory reset
        reboot();
    }
    if (ADMIN_REBOOT == server.admin_action) { reboot(); }
}

void setup() 
{
    Serial.begin(SERIAL_BAUD);  
    Serial.println(F(NAME "Starting ..."));
    led.setup();
    ssr_setup();
    wifi_setup();
    server.setup();
    Serial.println(F(NAME "Setup complete, loop starting ..."));
}

void loop() 
{
    admin_loop();
    led.loop();
    wm.process();
    thermocouple_loop();
    burner_loop();
#if DEBUG
    delay(1000);
#endif
}
