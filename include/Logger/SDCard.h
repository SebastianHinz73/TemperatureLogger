// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "FS.h"
#include "IDataStoreDevice.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>

enum SDCardState_t {
    Init,
    InitOk,
    InitFailure,
};

////////////////////////

class SDCardClass : public IDataStoreDevice {
public:
    SDCardClass();
    void init(Scheduler& scheduler);

    // IDataStoreDevice
    virtual void writeValue(uint16_t serial, time_t time, float value);
    virtual bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    virtual bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller);

private:
    void loop();

    void scanCard();
    bool openFile(uint16_t serial, const tm timeinfo, const char* mode, File& file);

private:
    Task _loopTask;

    uint32_t _lastActionTime;
    SDCardState_t _state;

    File _file; // used by getFile
    std::mutex _mutex;
};
extern SDCardClass* pSDCard;
