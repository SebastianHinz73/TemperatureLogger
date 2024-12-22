// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */
#include "WebApi_templogger.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiTempLoggerClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/templogger/config", HTTP_GET, std::bind(&WebApiTempLoggerClass::onTempLoggerAdminGet, this, _1));
    server.on("/api/templogger/config", HTTP_POST, std::bind(&WebApiTempLoggerClass::onTempLoggerAdminPost, this, _1));
}

void generateJsonResponse(JsonVariant& root)
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
        tempObj["time"] = valid ? time : 0;
        tempObj["value"] = valid ? value : 0;
    }
}

void WebApiTempLoggerClass::onTempLoggerAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["pollinterval"] = config.DS18B20.PollInterval;
    root["fahrenheit"] = config.DS18B20.Fahrenheit;
    auto sensors = root["sensors"].to<JsonArray>();

    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial != 0) {
            JsonObject sensor = sensors[i].to<JsonObject>();
            sensor["serial"] = String(config.DS18B20.Sensors[i].Serial, 16);
            sensor["connected"] = config.DS18B20.Sensors[i].Connected;
            sensor["name"] = config.DS18B20.Sensors[i].Name;
        }
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiTempLoggerClass::onTempLoggerAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 128 * (1 + Configuration.getConfiguredSensorCnt())) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    JsonDocument root;
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (!(root["pollinterval"].is<JsonObject>() && root["fahrenheit"].is<JsonObject>() && root["sensors"].is<JsonObject>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["pollinterval"].as<uint32_t>() < 5) {
        retMsg["message"] = "Poll interval minimum is 5 seconds!";
        retMsg["code"] = WebApiError::DS18B20BasePollIntervallToSmall;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["pollinterval"].as<uint32_t>() > 3600) {
        retMsg["message"] = "Poll interval maximum is 3600 seconds!";
        retMsg["code"] = WebApiError::DS18B20BasePollIntervallToBig;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    JsonArray sensors = root["sensors"];

    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        JsonObject sensor = sensors[i].as<JsonObject>();

        if (sensor.isNull()) {
            break;
        }
        if (!(sensor["serial"].is<JsonObject>() && sensor["connected"].is<JsonObject>() && sensor["name"].is<JsonObject>() && strlen(sensor["name"]) > 0)) {
            retMsg["message"] = "Values are missing!";
            retMsg["code"] = WebApiError::GenericValueMissing;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }
    }

    auto guard = Configuration.getWriteGuard();
    auto& config = guard.getConfig();

    config.DS18B20.PollInterval = root["pollinterval"].as<int>();
    config.DS18B20.Fahrenheit = root["fahrenheit"].as<bool>();
    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        JsonObject sensor = sensors[i].as<JsonObject>();
        if (!sensor.isNull()) {
            String s = sensor["serial"];
            config.DS18B20.Sensors[i].Serial = strtoull(s.c_str(), 0, 16);
            config.DS18B20.Sensors[i].Connected = sensor["connected"];
            strlcpy(config.DS18B20.Sensors[i].Name, sensor["name"], sizeof(config.DS18B20.Sensors[i].Name));
        } else {
            // reset unused
            config.DS18B20.Sensors[i].Serial = 0;
            config.DS18B20.Sensors[i].Connected = false;
            strlcpy(config.DS18B20.Sensors[i].Name, "undefined", sizeof(config.DS18B20.Sensors[i].Name));
        }
    }

    retMsg["type"] = "success";
    retMsg["message"] = "Settings saved!";
    retMsg["code"] = WebApiError::GenericSuccess;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    MqttSettings.performReconnect();
}
