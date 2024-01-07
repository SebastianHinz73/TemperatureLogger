// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

enum SDCardState_t {
    Init,
    InitOk,
    InitFailure,
};

////////////////////////

class SDCardClass {
public:
    SDCardClass() { }
    void init(Scheduler& scheduler);
    void loop();
    static bool getTmTime(struct tm* info, time_t time, uint32_t ms);

protected:
    void writeValue(uint16_t serial, time_t time, float value);
    bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    bool getFile(uint16_t serial, const tm& timeinfo, char* buffer, size_t& size);

private:
    void scanCard();
    bool openFile(uint16_t serial, const tm timeinfo, const char* mode, File& file);

private:
    Task _loopTask;

    uint32_t _lastActionTime;
    SDCardState_t _state;

    friend class DatastoreClass; // for using addValue, getFileSize, getFile (use mutex from DatastoreClass)
};
extern SDCardClass SDCard;
