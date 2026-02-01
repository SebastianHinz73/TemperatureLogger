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
    virtual bool getFile(uint16_t serial, time_t start, uint32_t length, ResponseFiller& responseFiller);
    virtual bool getBackup(ResponseFiller& responseFiller) { return false; }
    virtual bool restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len, bool final) { return false; }

private:
    void loop();

    void scanCard();
    bool openFile(uint16_t serial, const time_t time, const char* mode, File& file);

private:
    Task _loopTask;

    uint32_t _lastActionTime;
    SDCardState_t _state;

    File _file; // used by getFile
    std::mutex _mutex;
};
extern SDCardClass* pSDCard;
