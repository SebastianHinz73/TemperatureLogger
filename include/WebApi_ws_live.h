// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiWsLiveClass {
public:
    WebApiWsLiveClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);
    void reload();

private:
    void generateJsonResponse(JsonVariant& root);
    void generateGraphConfigResponse(JsonVariant& root);
    void generateGraphDataResponse(JsonVariant& root);

    void onLivedataStatus(AsyncWebServerRequest* request);
    void onGraphUpdate(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebSocket _ws;
    AuthenticationMiddleware _simpleDigestAuth;

    unsigned long _lastPublishStats = 0;
    std::mutex _mutex;

    Task _wsCleanupTask;
    void wsCleanupTaskCb();

    Task _sendDataTask;
    void sendDataTaskCb();
};
