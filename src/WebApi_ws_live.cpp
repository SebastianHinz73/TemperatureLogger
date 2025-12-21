// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>
#include "mbedtls/base64.h"


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
    server.on("/api/livedata/graph", HTTP_GET, std::bind(&WebApiWsLiveClass::onGraphUpdate, this, _1));
    server.on("/api/livedata/graphdata", HTTP_GET, std::bind(&WebApiWsLiveClass::onGraphData, this, _1));

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
        std::lock_guard<std::mutex> lock(_mutex);
        JsonDocument root;
        JsonVariant var = root;

        generateJsonResponse(var);

        const CONFIG_T& config = Configuration.get();
        auto tempObject = root["updates"].to<JsonObject>();

        for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
            if (config.DS18B20.Sensors[i].Serial == 0) {
                continue;
            }
            uint32_t time;
            float value;
            bool valid = Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value);
            if(!valid) {
                continue;
            }

            String serial = String(config.DS18B20.Sensors[i].Serial, 16);
            tempObject[serial] = value;
        }

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
    auto tempArray = root["temperatures"].to<JsonArray>();

    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial == 0) {
            continue;
        }
        uint32_t time;
        float value;
        bool valid = Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value);

        JsonObject tempObj = tempArray[i].to<JsonObject>();
        tempObj["valid"] = valid;
        tempObj["serial"] = String(config.DS18B20.Sensors[i].Serial, 16);
        tempObj["name"] = config.DS18B20.Sensors[i].Name;
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
        std::lock_guard<std::mutex> lock(_mutex);
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

void WebApiWsLiveClass::generateGraphConfigResponse(JsonVariant& root)
{
    const CONFIG_T& config = Configuration.get();
    auto tempArray = root["config"].to<JsonObject>();

    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial == 0) {
            continue;
        }
        uint32_t time;
        float value;
        bool valid = Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value);
        if(!valid) {
            continue;
        }

        const char* colors[] = { "#ff0000", "#c8c8c8","#646464", "#0000FF", "#c8c8c8", "#646464" , "#00FF00", "#00aa00"};


        //JsonObject tempObj = tempArray[i].to<JsonObject>();
        //tempObj["name"] = config.DS18B20.Sensors[i].Name;
        //tempObj["serial"] = String(config.DS18B20.Sensors[i].Serial, 16);
        //tempObj["color"] = colors[i % (sizeof(colors) / sizeof(colors[0]))];

        String serial = String(config.DS18B20.Sensors[i].Serial, 16);
        tempArray[serial]["name"] = config.DS18B20.Sensors[i].Name;
        tempArray[serial]["color"] = colors[i % (sizeof(colors) / sizeof(colors[0]))];
    }
}

void WebApiWsLiveClass::generateGraphDataResponse(JsonVariant& root)
{
    const CONFIG_T& config = Configuration.get();
    auto tempArray = root["data"].to<JsonObject>();

    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial == 0) {
            continue;
        }
        uint32_t time;
        float value;
        bool valid = Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value);
        if(!valid) {
            continue;
        }

        String serial = String(config.DS18B20.Sensors[i].Serial, 16);

        tempArray[serial] = "[]";

        static int y = 20;
        tempArray[serial] = "[{\"x\":" + String(time-10) + ",\"y\":" + String(y++) + "}, {\"x\": " + String(time) + ",\"y\": " + String(y++) + "}]";
    }
}

void WebApiWsLiveClass::onGraphUpdate(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutex);

        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();

        //const CONFIG_T& config = Configuration.get();

        generateGraphConfigResponse(root);

        generateGraphDataResponse(root);

        /*
        long long timestamp = 0;
        uint32_t interval = 2 * TASK_SECOND;

        if (request->hasParam("timestamp")) {
            String s = request->getParam("timestamp")->value();

            timestamp = strtoll(s.c_str(), NULL, 10);
            if (timestamp == 0) {
                interval = 60 * TASK_SECOND;
            }
        }
*/
        //root["timestamp"] = timestamp + interval;
        //root["interval"] = interval;

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/graph temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/graph. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

void WebApiWsLiveClass::onGraphData(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutex);

        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();

        if (!request->hasParam("serial") || !request->hasParam("timestamp") || !request->hasParam("interval")) {
            root["error"] = "Missing parameter serial, timestamp or interval";
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        int serial = request->getParam("serial")->value().toInt();
        int timestamp = request->getParam("timestamp")->value().toInt();
        int interval = request->getParam("interval")->value().toInt();

        if(serial <= 0 || timestamp <= 0 || interval <= 0) {
            root["error"] = "Parameter serial, timestamp or interval have invalid values.";
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        const CONFIG_T& config = Configuration.get();
        for (uint8_t i = 0; i <= TEMPLOGGER_MAX_COUNT; i++) {
            if(i == TEMPLOGGER_MAX_COUNT) {
                root["error"] = "Sensor not found.";
                WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
                return;
            }
            if (config.DS18B20.Sensors[i].Serial == 0 || config.DS18B20.Sensors[i].Serial != serial) {
                continue;
            }

            uint32_t time;
            float value;
            bool valid = Datastore.getTemperature(serial, time, value);
            if(!valid) {
                root["error"] = "Sensor not valid.";
                WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
                continue;
            }
            break; // found
        }

        // all checks done

#if 1
        tm timeinfo;
        if (!getLocalTime(&timeinfo, 5)) {
            MessageOutput.printf("WebApiIotSensorData: getLocalTime failed. Ignore\r\n");
        }

        //uint16_t serial;
        timeinfo.tm_year = 2025 - 1900;
        timeinfo.tm_mon = 12 - 1;
        timeinfo.tm_mday = 21;

        serial = 0x76a0;

        static size_t fileSize;
        if (!Datastore.getFileSize(serial, timeinfo, fileSize)) {
            request->send(404);
            return;
        }

        static ResponseFiller responseFiller;
        if (!Datastore.getTemperatureFile(serial, timeinfo, responseFiller)) {
            MessageOutput.print("WebApiIotSensorData: Can not get file.\r\n");
            return;
        }

        AsyncWebServerResponse* response2 = request->beginResponse("text/plain", fileSize, [&](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
            return responseFiller(buffer, maxLen, alreadySent, fileSize);
        });
        response2->addHeader("Server", "ESP Async Web Server");
        request->send(response2);

#else
        //auto tempArray = root["config"].to<JsonObject>();
        char buffer[1024*7];
        memset(buffer, 1, sizeof(buffer));

        //response->_fillBuffer((uint8_t*)buffer, sizeof(buffer));
        root["0"] = MsgPackBinary(buffer, sizeof(buffer));
       // String serial = String(config.DS18B20.Sensors[i].Serial, 16);

        //root["timestamp"] = timestamp + interval;
        //root["interval"] = interval;

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
#endif
    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/graph temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/graph. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}
