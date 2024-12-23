// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_power.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>

void WebApiPowerClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/power/status", HTTP_GET, std::bind(&WebApiPowerClass::onPowerStatus, this, _1));
    server.on("/api/power/config", HTTP_POST, std::bind(&WebApiPowerClass::onPowerPost, this, _1));
}

void WebApiPowerClass::onPowerStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    request->send(404);
}

void WebApiPowerClass::onPowerPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }
    request->send(404);
}