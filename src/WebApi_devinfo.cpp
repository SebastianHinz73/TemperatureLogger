// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_devinfo.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <ctime>

void WebApiDevInfoClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/devinfo/status", HTTP_GET, std::bind(&WebApiDevInfoClass::onDevInfoStatus, this, _1));
}

void WebApiDevInfoClass::loop()
{
}

void WebApiDevInfoClass::onDevInfoStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    request->send(404);
}