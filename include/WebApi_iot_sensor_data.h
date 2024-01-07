// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

class WebApiIotSensorData {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onConfig(AsyncWebServerRequest* request);
    void onFile(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};