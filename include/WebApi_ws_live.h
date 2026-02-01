// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>
#include "Logger/TimeoutMutex.h"

class WebApiWsLiveClass {
public:
    WebApiWsLiveClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);
    void reload();

private:
    void generateJsonResponse(JsonVariant& root);

    void onLivedataStatus(AsyncWebServerRequest* request);
    void onGraphUpdate(AsyncWebServerRequest* request);
    void onGraphData(AsyncWebServerRequest* request);
    void onBackup(AsyncWebServerRequest* request);
    void onBackupUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
    void onBackupUploadFinish(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebSocket _ws;
    AuthenticationMiddleware _simpleDigestAuth;

    unsigned long _lastPublishStats = 0;
    std::mutex _mutexStatus;

    TimeoutMutex _mutexFileReponse;

    Task _wsCleanupTask;
    void wsCleanupTaskCb();

    Task _sendDataTask;
    void sendDataTaskCb();
};
