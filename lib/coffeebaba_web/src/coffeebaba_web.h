
// Webserver related code for coffeebaba
// includes template strings for coffeebaba html gen 

#ifndef coffeebaba_web_h
#define coffeebaba_web_h

#include "Arduino.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define CB_HTML_HEAD "<!DOCTYPE html> " \
    "<html> " \
    "    <head>\n" \
    "        <meta charset=\"UTF-8\">\n" \
    "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" \
    "        <title>CoffeeBaba</title>\n" \
    "        <link href=\"//mincss.com/entireframework.min.css\" rel=\"stylesheet\" type=\"text/css\">\n" \
    "        <style>\n" \
    "            h1 {\n" \
    "                margin: 0.2em 0;\n" \
    "                border-bottom: 5px solid;\n" \
    "                margin-bottom: 15px;\n" \
    "                padding-bottom: 5px;\n" \
    "            }\n" \
    "        </style>\n" \
    "    </head>\n" \
    "    <body>\n" \
    "        <div class=\"container\">\n"
 
#define CB_HTML_FOOT "        </div>\n" \
    "    </body>\n" \
    "</html>\n"

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
