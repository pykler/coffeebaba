
// Webserver related code for coffeebaba
// includes template strings for coffeebaba html gen 

#ifndef coffeebaba_web_h
#define coffeebaba_web_h

#include "Arduino.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// first const, is for the char* (string data),
// second is for the pointer (int)
// this is why we can define it in the header and not have a linker error
const char* const CB_HTML_HEAD = 
#include "coffeebaba_web_head.htmli"
;

const char* const CB_HTML_FOOT =
#include "coffeebaba_web_foot.htmli"
;

enum AdminAction { ADMIN_NONE, ADMIN_REBOOT, ADMIN_RESET };

class CoffeeBabaWeb
{
    public:
        CoffeeBabaWeb(uint16_t, const char *);
        void setup();
        AdminAction admin_action;
        AsyncWebServer * server;
        AsyncWebSocket * ws;
        byte burner;
        float temp;
        
    private:
        void logr(AsyncWebServerRequest *request);
        void error404(AsyncWebServerRequest *request);
        void index_get(AsyncWebServerRequest *request);
        void index_post(AsyncWebServerRequest *request);
        void ws_onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
};

#endif
