// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiTempLoggerClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onTempLoggerAdminGet(AsyncWebServerRequest* request);
    void onTempLoggerAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};