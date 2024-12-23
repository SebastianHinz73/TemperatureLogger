// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */
#include "WebApi_iot_sensor_data.h"
#include "Configuration.h"
#include "DS18B20List.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>

void WebApiIotSensorData::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/config", HTTP_GET, std::bind(&WebApiIotSensorData::onConfig, this, _1));
    server.on("/file", HTTP_GET, std::bind(&WebApiIotSensorData::onFile, this, _1));
}

void WebApiIotSensorData::onConfig(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    char buffer[256];
    String bootTime("Unknown");
    tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
        uint32_t sec_since_boot = esp_timer_get_time() / 1000000;
        time_t now = time(nullptr);
        now -= sec_since_boot;

        Datastore.getTmTime(&timeinfo, now, 5);
        strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &timeinfo);
        bootTime = buffer;
    }

    auto config = Configuration.get();
    snprintf(buffer, sizeof(buffer), "1;%s;%s;%s;-1\n", NetworkSettingsClass::getHostname().c_str(), TEMP_LOGGER_VERSION, bootTime.c_str());

    String text = buffer;

    for (uint8_t i = 0; i < Configuration.getConfiguredSensorCnt(); i++) {
        uint16_t serial = config.DS18B20.Sensors[i].Serial;
        uint32_t time = 0;
        float value = 0;
        bool bValid = Datastore.getTemperature(serial, time, value);

        // "1;C717;44.37;WWLVL;7;0;0;977\n";
        snprintf(buffer, sizeof(buffer), "1;%04X;%.2f;%s;-1;-1;-1;-1\n", serial, bValid ? value : 0, config.DS18B20.Sensors[i].Name);
        text += buffer;
    }

    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", text);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
}

void WebApiIotSensorData::onFile(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        MessageOutput.printf("WebApiIotSensorData: getLocalTime failed. Ignore\r\n");
    }

    uint16_t serial;
    if (request->hasParam("y")) {
        timeinfo.tm_year = request->getParam("y")->value().toInt() - 1900;
    }
    if (request->hasParam("m")) {
        timeinfo.tm_mon = request->getParam("m")->value().toInt() - 1;
    }
    if (request->hasParam("d")) {
        timeinfo.tm_mday = request->getParam("d")->value().toInt();
    }
    if (request->hasParam("id")) {
        serial = strtol(request->getParam("id")->value().c_str(), 0, 16);
    } else {
        DS18B20SENSOR_CONFIG_T* config = Configuration.getFirstDS18B20Config();
        if (config == nullptr) {
            request->send(404);
            return;
        }
        serial = config->Serial;
    }

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

    AsyncWebServerResponse* response = request->beginResponse("text/plain", fileSize, [&](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        return responseFiller(buffer, maxLen, alreadySent, fileSize);
    });
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
}
