// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_eventlog.h"
#include "WebApi.h"
#include <AsyncJson.h>

void WebApiEventlogClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/eventlog/status", HTTP_GET, std::bind(&WebApiEventlogClass::onEventlogStatus, this, _1));
}

void WebApiEventlogClass::onEventlogStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    request->send(404);
}