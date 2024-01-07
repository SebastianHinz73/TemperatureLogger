// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <mutex>

class WebApiWsLiveClass {
public:
    WebApiWsLiveClass();
    void init(AsyncWebServer& server);
    void loop();

private:
    void generateJsonResponse(JsonVariant& root);
    void addTotalField(JsonObject& root, const String& name, const float value, const String& unit, const uint8_t digits);
    void onLivedataStatus(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebServer* _server;
    AsyncWebSocket _ws;

    uint32_t _lastWsPublish = 0;
    uint32_t _lastUpdateCheck = 0;
    uint32_t _lastWsCleanup = 0;
    bool _bNewClient = false;
    std::mutex _mutex;
};