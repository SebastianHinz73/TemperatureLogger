// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "WebApi_device.h"
#include "WebApi_devinfo.h"
#include "WebApi_errors.h"
#include "WebApi_eventlog.h"
#include "WebApi_file.h"
#include "WebApi_firmware.h"
#include "WebApi_i18n.h"
#include "WebApi_iot_sensor_data.h"
#include "WebApi_maintenance.h"
#include "WebApi_mqtt.h"
#include "WebApi_network.h"
#include "WebApi_ntp.h"
#include "WebApi_power.h"
#include "WebApi_security.h"
#include "WebApi_sysstatus.h"
#include "WebApi_templogger.h"
#include "WebApi_webapp.h"
#include "WebApi_ws_console.h"
#include "WebApi_ws_live.h"
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiClass {
public:
    WebApiClass();
    void init(Scheduler& scheduler);
    void reload();

    static bool checkCredentials(AsyncWebServerRequest* request);
    static bool checkCredentialsReadonly(AsyncWebServerRequest* request);

    static void sendTooManyRequests(AsyncWebServerRequest* request);

    static void writeConfig(JsonVariant& retMsg, const WebApiError code = WebApiError::GenericSuccess, const String& message = "Settings saved!");

    static bool parseRequestData(AsyncWebServerRequest* request, AsyncJsonResponse* response, JsonDocument& json_document);
    static uint64_t parseSerialFromRequest(AsyncWebServerRequest* request, String param_name = "inv");
    static bool sendJsonResponse(AsyncWebServerRequest* request, AsyncJsonResponse* response, const char* function, const uint16_t line);

private:
    AsyncWebServer _server;

    WebApiDeviceClass _webApiDevice;
    WebApiDevInfoClass _webApiDevInfo;
    WebApiEventlogClass _webApiEventlog;
    WebApiFileClass _webApiFile;
    WebApiFirmwareClass _webApiFirmware;
    WebApiI18nClass _webApiI18n;
    WebApiMaintenanceClass _webApiMaintenance;
    WebApiMqttClass _webApiMqtt;
    WebApiTempLoggerClass _webApiTempLogger;
    WebApiNetworkClass _webApiNetwork;
    WebApiNtpClass _webApiNtp;
    WebApiPowerClass _webApiPower;
    WebApiSecurityClass _webApiSecurity;
    WebApiSysstatusClass _webApiSysstatus;
    WebApiWebappClass _webApiWebapp;
    WebApiWsConsoleClass _webApiWsConsole;
    WebApiWsLiveClass _webApiWsLive;
    WebApiIotSensorData _webApiIotSensorData;
};

extern WebApiClass WebApi;