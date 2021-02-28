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

#define DEBUG       1  // Enable / Disable debug

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
#define TCP_TSTART  21  // Initial system temp in Deg Celcius (room temp)

#include <Arduino.h>
#include "led_controller.h"
#include "coffeebaba_html.h"
#include <max6675.h>
#include <WiFiManager.h>
#include <ESPAsyncTCP.h>
#define WEBSERVER_H  // to avoid conflict between ESPAsyncWebServer and WifiManager
#include <ESPAsyncWebServer.h>

#if DEBUG
#define debug_print(msg) Serial.print(msg);
#define debug_println(msg) Serial.println(msg);
#else
#define debug_print(msg)
#define debug_println(msg)
#endif

WiFiManager wm;
LedController led(LED_PIN);
MAX6675 thermocouple(TCP_SCK, TCP_CS, TCP_SO);
unsigned long thermocouple_poll = 0;
float thermocouple_temp = TCP_TSTART;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
// admin flags
bool should_reboot = false;  // flag to allow remote reboot
bool should_reset = false;  // flag to allow reset of internal state

/* WebSocket Server Code */

void server_setup()
{
    Serial.println("Server setup ...");
    // ws.onEvent(server_ws_onEvent)
    server.addHandler(&ws);

    server.on("/", HTTP_GET, server_get_index);
    server.on("/", HTTP_POST, server_post_index);
    server.onNotFound(server_404);
    server.begin();
}

void server_logr(AsyncWebServerRequest *request)
{
  Serial.print(request->methodToString());
  Serial.print(" ");
  Serial.println(request->url());
}

void server_404(AsyncWebServerRequest *request)
{
    //Handle Unknown Request
    server_logr(request);
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->setCode(404);
    response->print(CB_HTML_HEAD);
    response->print("<h1> CoffeeBaba </h1>");
    response->print("<div class=\"msg\"><strong>404!</strong> This page is lost, if you find it let us know </div>");
    response->print(CB_HTML_FOOT);
    request->send(response);
}

void server_get_index(AsyncWebServerRequest *request)
{
    server_logr(request);
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->setCode(200);
    response->print(CB_HTML_HEAD);
    response->print("<h1> CoffeeBaba </h1>");
    // Admin actions
    response->print("<h2> Admin Actions </h2>");
    response->print("<div>");
    response->print("<form method=\"POST\"><button class=\"btn btn-c btn-sm smooth\" type=\"submit\" name=\"action\" value=\"reset\">Factory Reset</button></form>");
    response->print("<form method=\"POST\"><button class=\"btn btn-a btn-sm smooth\" type=\"submit\" name=\"action\" value=\"reboot\">Reboot</button></form>");
    response->print("</div>");
    // Stats Table
    response->print("<h2> Stats </h2>");
    response->printf("<table class=\"table\"><tbody><tr><th>Free Memory</th><td>%d</td></tr><tr><th>Used Sketch</th><td>%d</td></tr><tr><th>Free Sketch</th><td>%d</td></tr><tr><th>Uptime (mins)</th><td>%.2f</td></tr><tr><th>Last Reset Reason</th><td>%s</td></tr></tbody></table>", ESP.getFreeHeap(), ESP.getSketchSize(), ESP.getFreeSketchSpace(), millis() / 60000.0, ESP.getResetReason().c_str());
    response->print("<h2> Chip Info </h2>");
    response->printf("<table class=\"table\"><tbody><tr><th>CPU Mhz</th><td>%d</td></tr><tr><th>Chip ID</th><td>%06X</td></tr><tr><th>SDK</th><td>%s</td></tr><tr><th>Version</th><td>%s</td></tr></tbody></table>", ESP.getCpuFreqMHz(), ESP.getChipId(), system_get_sdk_version(), ESP.getCoreVersion().c_str());
    // Admin area
    response->print(CB_HTML_FOOT);
    request->send(response);
}

void server_post_index(AsyncWebServerRequest *request)
{
    server_logr(request);
    // process the request
    if(request->hasParam("action", true))
    {
        AsyncWebParameter* p = request->getParam("action", true);
        if (p->value() == "reboot")
        {
            should_reboot = true;
        }
        else if(p->value() == "reset")
        {
            should_reset = true;
        }
        Serial.print("requested action = ");
        Serial.println(p->value());
    }
    // redirect to / to avoid resubmitting the action
    request->redirect("/");
}



/* END - WebSocket Server Code */

void wifi_setup()
{
    Serial.println("WiFi: setup in progress ...");
    WiFi.mode(WIFI_STA);

    wm.setConfigPortalBlocking(false);
    wm.setSaveConfigCallback(wifi_completedCallback);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect(NAME))
    {
        Serial.println("WiFi: connected!");
        // note that wifi_completedCallback will not be called here
    }
    else 
    {
        Serial.println("WiFi: Configportal running ...");
        led.blink(300);
    }
}

void wifi_reset()
{
    Serial.println("WiFi: Resetting settings");
    wm.resetSettings();
}

void wifi_completedCallback()
{
    Serial.println("WiFi: completed callback");
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
    thermocouple_temp = thermocouple.readCelsius();
    debug_print("C = ");
    debug_println(thermocouple_temp);
    debug_print("F = ");
    debug_println(thermocouple_c_to_f(thermocouple_temp));
}

void ssr_setup()
{
    Serial.println("Setting up SS Relay");
    pinMode(SSR_PIN, OUTPUT);
    ssr_open(); // for safety kill the current
}

void ssr_close() // close the circuit
{
    debug_print("SSR circuit closed ... pin_value: ");
    // High if in normally closed mode
    byte pin_value = LOW | SSR_NC;
    digitalWrite(SSR_PIN, pin_value);
    debug_println(pin_value);
}

void ssr_open() // open the ssr circuit
{
    debug_print("SSR circuit open ... pin_value: ");
    // Low if in normally closed mode
    byte pin_value = HIGH & ~SSR_NC;
    digitalWrite(SSR_PIN, pin_value);
    debug_println(pin_value);
}

void reboot()
{
    Serial.println("Rebooting ...");
    delay(100);
    ESP.restart();
}

void admin_loop()
{
    if (should_reset) { 
        // disable the flag, as it has been actioned
        should_reset = false; 
        // action it (factory reset actions)
        wifi_reset();
        // reboot after factory reset
        reboot();
    }
    if (should_reboot) { reboot(); }
}

void setup() 
{
    Serial.begin(SERIAL_BAUD);  
    Serial.println(NAME "Starting ...");
    led.setup();
    ssr_setup();
    wifi_setup();
    server_setup();
    Serial.println(NAME "Setup complete, loop starting ...");
}

void loop() 
{
    admin_loop();
    led.loop();
    wm.process();
    thermocouple_loop();
#if DEBUG
    delay(1000);
#endif
}
