#include "coffeebaba_web.h"
#include <cstring>
#include "Arduino.h"
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define COMMAND_SETCP "setControlParams"
#ifndef TCP_TSTART
#define TCP_TSTART  21  // Initial system temp in Deg Celcius (~room temp)
#endif

// admin flags
CoffeeBabaWeb::CoffeeBabaWeb(uint16_t port, const char * websocket_path)
{
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket(websocket_path); // access at ws://[esp ip]/<websocket_path>
    admin_action = ADMIN_NONE;
    temp = TCP_TSTART;
}

void CoffeeBabaWeb::logr(AsyncWebServerRequest *request)
{
  Serial.print(request->methodToString());
  Serial.print(F(" "));
  Serial.println(request->url());
}

void CoffeeBabaWeb::error404(AsyncWebServerRequest *request)
{
    //Handle Unknown Request
    logr(request);
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->setCode(404);
    response->print(CB_HTML_HEAD);
    response->print("<h1> CoffeeBaba </h1>");
    response->print("<div class=\"msg\"><strong>404!</strong> This page is lost, if you find it let us know </div>");
    response->print(CB_HTML_FOOT);
    request->send(response);
}

void CoffeeBabaWeb::index_get(AsyncWebServerRequest *request)
{
    logr(request);
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

void CoffeeBabaWeb::index_post(AsyncWebServerRequest *request)
{
    logr(request);
    // process the request
    if(request->hasParam("action", true))
    {
        AsyncWebParameter* p = request->getParam("action", true);
        if (p->value() == "reboot")
        {
            admin_action = ADMIN_REBOOT;
        }
        else if(p->value() == "reset")
        {
            admin_action = ADMIN_RESET;
        }
        Serial.print(F("requested action = "));
        Serial.println(p->value());
    }
    // redirect to / to avoid resubmitting the action
    request->redirect("/");
}


void CoffeeBabaWeb::ws_onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
    if(type == WS_EVT_DATA){
        StaticJsonDocument<192> doc_in;
        StaticJsonDocument<192> doc_out;
        data[len] = 0; // set the null char

        // Parse and process input
        DeserializationError error = deserializeJson(doc_in, (char *)data, len);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        if (!strncmp(doc_in["command"], COMMAND_SETCP, strlen(COMMAND_SETCP)))
        {
            // command == COMMAND_SETCP
            burner = (byte) doc_in["params"]["burner"];
            // console output
            Serial.print(F("Burner set to: "));
            Serial.println(burner);
        }

        // Create and Send Response
        doc_out["id"] = doc_in["id"];
        doc_out.createNestedObject("data");
        doc_out["data"]["beanTemp"] = temp;
        doc_out["data"]["burner"] = burner;
        size_t len = measureJson(doc_out);
        AsyncWebSocketMessageBuffer * buffer = ws->makeBuffer(len); // creates buffer of len + 1
        if (buffer) {
            serializeJson(doc_out, (char *)buffer->get(), len + 1);
            client->text(buffer);
        }
    } // end if (type == WS_EVT_DATA)
    else if (type == WS_EVT_CONNECT)
    {
        //client connected
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    }
    else if(type == WS_EVT_DISCONNECT)
    {
        //client disconnected
        Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    }
    else if(type == WS_EVT_ERROR)
    {
        //error was received from the other end
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
    }
    else if(type == WS_EVT_PONG)
    {
        //pong message was received (in response to a ping request maybe)
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
    }
}

/* externals */

void CoffeeBabaWeb::setup()
{
    Serial.println(F("Server setup ..."));
    ws->onEvent(std::bind(&CoffeeBabaWeb::ws_onEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    server->addHandler(ws);

    server->on("/", HTTP_GET, std::bind(&CoffeeBabaWeb::index_get, this, std::placeholders::_1));
    server->on("/", HTTP_POST, std::bind(&CoffeeBabaWeb::index_post, this, std::placeholders::_1));
    server->onNotFound(std::bind(&CoffeeBabaWeb::error404, this, std::placeholders::_1));
    server->begin();
}
