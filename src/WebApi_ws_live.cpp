// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "Logger/RamDrive.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>
#include "mbedtls/base64.h"
#include <RestartHelper.h>


WebApiWsLiveClass::WebApiWsLiveClass()
    : _ws("/livedata")
    , _wsCleanupTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsLiveClass::wsCleanupTaskCb, this))
    , _sendDataTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsLiveClass::sendDataTaskCb, this))
{
}

void WebApiWsLiveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    server.on("/api/livedata/status", HTTP_GET, std::bind(&WebApiWsLiveClass::onLivedataStatus, this, _1));
    server.on("/api/livedata/graphdata", HTTP_GET, std::bind(&WebApiWsLiveClass::onGraphData, this, _1));
    server.on("/api/livedata/backup", HTTP_GET, std::bind(&WebApiWsLiveClass::onBackup, this, _1));
    server.on("/api/livedata/backup", HTTP_POST,
        std::bind(&WebApiWsLiveClass::onBackupUploadFinish, this, _1),
        std::bind(&WebApiWsLiveClass::onBackupUpload, this, _1, _2, _3, _4, _5, _6));

    server.addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));

    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.enable();

    scheduler.addTask(_sendDataTask);
    _sendDataTask.enable();
    _simpleDigestAuth.setUsername(AUTH_USERNAME);
    _simpleDigestAuth.setRealm("live websocket");

    reload();
}

void WebApiWsLiveClass::reload()
{
    _ws.removeMiddleware(&_simpleDigestAuth);

    auto const& config = Configuration.get();

    if (config.Security.AllowReadonly) {
        return;
    }

    _ws.enable(false);
    _simpleDigestAuth.setPassword(config.Security.Password);
    _ws.addMiddleware(&_simpleDigestAuth);
    _ws.closeAll();
    _ws.enable(true);
}

void WebApiWsLiveClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

void WebApiWsLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    bool bValueChanged = false;
    const CONFIG_T& config = Configuration.get();
    for (uint8_t i = 0; !bValueChanged && i < Configuration.getConfiguredSensorCnt(); i++) {
        if (!Datastore.validSensor(config.DS18B20.Sensors[i].Serial)) {
            continue;
        }
        bValueChanged |= Datastore.valueChanged(config.DS18B20.Sensors[i].Serial, 5);
    }
    bool bForce = millis() - _lastPublishStats > (5/*60*/ * 1000);

    // Update at least after 60 seconds
    if (!(bValueChanged || bForce)) {
        return;
    }
    //MessageOutput.printf("bValueChanged %d , %d\r\n", bValueChanged, millis() - _lastPublishStats);

    _lastPublishStats = millis();

    try {
        std::lock_guard<std::mutex> lock(_mutexStatus);
        JsonDocument root;
        JsonVariant var = root;

        generateJsonResponse(var);

        String buffer;
        serializeJson(root, buffer);

        _ws.textAll(buffer);

    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/status temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/status. Reason: \"%s\".\r\n", exc.what());
    }
}

void WebApiWsLiveClass::generateJsonResponse(JsonVariant& root)
{
    const CONFIG_T& config = Configuration.get();
    auto arrayConfig = root["config"].to<JsonArray>();
    auto arrayUpdates = root["updates"].to<JsonArray>();

    int indexUpdates = 0;
    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial == 0) {
            continue;
        }
        uint32_t time;
        float value;
        bool valid = Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value);

        JsonObject tempObj = arrayConfig[i].to<JsonObject>();
        tempObj["valid"] = valid;
        tempObj["serial"] = String(config.DS18B20.Sensors[i].Serial, 16);
        tempObj["name"] = config.DS18B20.Sensors[i].Name;

        if(valid) {
            tempObj = arrayUpdates[indexUpdates++].to<JsonObject>();
            tempObj["serial"] = String(config.DS18B20.Sensors[i].Serial, 16);
            tempObj["value"] = value;
        }
    }

    JsonObject hintObj = root["hints"].to<JsonObject>();
    struct tm timeinfo;
    hintObj["time_sync"] = !getLocalTime(&timeinfo, 5);
    hintObj["radio_problem"] = false;
    hintObj["default_password"] = strcmp(Configuration.get().Security.Password, ACCESS_POINT_PASSWORD) == 0;
}

void WebApiWsLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] connect\r\n", server->url(), client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] disconnect\r\n", server->url(), client->id());
    }
}

void WebApiWsLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutexStatus);
        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();

        generateJsonResponse(root);

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/status temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/status. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

void WebApiWsLiveClass::onGraphData(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        MessageOutput.printf("WebApiIotSensorData: getLocalTime failed. Ignore\r\n");
        request->send(200);
        return;
    }

    static ResponseFiller responseFiller;
    AsyncWebServerResponse* response = nullptr;

    try {
        if (request->hasParam("id") && request->hasParam("start") && request->hasParam("length") &&
            _mutexFileReponse.TryLock(0, 1500)) {

            uint16_t serial = strtol(request->getParam("id")->value().c_str(), 0, 16);
            time_t start = request->getParam("start")->value().toInt();
            uint32_t length = request->getParam("length")->value().toInt();

            if (!Datastore.getTemperatureFile(serial, start, length, responseFiller)) {
                MessageOutput.print("WebApi_ws_live: Can not get file.\r\n");
                request->send(200);
                _mutexFileReponse.unlock();
                return;
            }

            response = request->beginChunkedResponse("text/plain", [&](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
                int send = responseFiller(buffer, maxLen, alreadySent);
                if(send == 0) {
                    _mutexFileReponse.unlock();
                }
                return send;
            });
        }
        else{
            MessageOutput.printf("WebApiIotSensorData: Parameter id, start or length or busy\r\n");
            request->send(200);
            return;
        }

        response->addHeader("Server", "ESP Async Web Server");
        request->send(response);
    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/graph temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
        _mutexFileReponse.unlock();
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/graph. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
        _mutexFileReponse.unlock();
    }
}

void WebApiWsLiveClass::onBackup(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        MessageOutput.printf("WebApiIotSensorData: getLocalTime failed. Ignore\r\n");
        request->send(200);
        return;
    }

    if(pRamDrive == nullptr) {
        MessageOutput.printf("WebApiIotSensorData: No ramdrive available.\r\n");
        request->send(200);
        return;
    }

    static ResponseFiller responseFiller;
    AsyncWebServerResponse* response = nullptr;

    try {

        if(!_mutexFileReponse.TryLock(0, 15000)) {
            request->send(200);
            return;
        }

        size_t usedBytes = pRamDrive->getUsedBytes();
        if (!Datastore.getBackup(responseFiller)) {
            MessageOutput.print("WebApi_ws_live: Can not get backup.\r\n");
            request->send(200);
            _mutexFileReponse.unlock();
            return;
        }

        response = request->beginResponse("text/plain", usedBytes, [&, usedBytes](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
            int send = responseFiller(buffer, maxLen, alreadySent);

            if(alreadySent + send >= usedBytes) {
                _mutexFileReponse.unlock();
            }
            return send;
        });

        response->addHeader("Server", "ESP Async Web Server");
        request->send(response);
    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/backup temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
        _mutexFileReponse.unlock();
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/backup. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
        _mutexFileReponse.unlock();
    }
}

void WebApiWsLiveClass::onBackupUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    if(pRamDrive == nullptr) {
        MessageOutput.printf("WebApiIotSensorData: No ramdrive available.\r\n");
        return;
    }

    if(index == 0)
    {
        if(!_mutexFileReponse.TryLock(0, 15000)) {
            MessageOutput.print("WebApi_ws_live: Backup upload busy.\r\n");
            request->send(503);
            return;
        }
    }
    if (!Datastore.restoreBackup(index, data, len, final)) {
        MessageOutput.print("WebApi_ws_live: Can not restore backup. No valid backup file.\r\n");
        request->send(404);
        _mutexFileReponse.unlock();
        return;
    }

    if (final) {
        // close the file handle as the upload is now done
        request->_tempFile.close();
        _mutexFileReponse.unlock();
    }
}

void WebApiWsLiveClass::onBackupUploadFinish(AsyncWebServerRequest* request)
{
    // This path is used when POST is invoked without multipart upload support.
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}
