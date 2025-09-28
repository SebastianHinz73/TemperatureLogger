// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ramdrive.h"
#include "Configuration.h"
#include "RestartHelper.h"
#include "Utils.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Datastore.h>
#include <LittleFS.h>
#include <Logger/IDataStoreDevice.h>
#include <MessageOutput.h>

void WebApiRamDriveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    server.on("/api/ram/get", HTTP_GET, std::bind(&WebApiRamDriveClass::onFileGet, this, _1));
}

void WebApiRamDriveClass::onFileGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    static size_t fileSize = 0;
    static ResponseFiller responseFiller;
    if (!Datastore.backupAll(responseFiller)) {
        MessageOutput.print("WebApiRamDriveClass: Can not get file.\r\n");
        return;
    }

    AsyncWebServerResponse* response = request->beginResponse("application/octet-stream", fileSize, [&](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        return responseFiller(buffer, maxLen, alreadySent, fileSize);
    });
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);

    /*

    String requestFile = CONFIG_FILENAME;
    if (request->hasParam("file")) {
        String name = "/" + request->getParam("file")->value();
        if (LittleFS.exists(name)) {
            requestFile = name;
        } else {
            request->send(404);
            return;
        }
    }

    request->send(LittleFS, requestFile, String(), true);
    */
}
