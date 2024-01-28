// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>

enum SDCardState_t {
    Init,
    InitOk,
    InitFailure,
};

typedef std::function<size_t(uint8_t* buffer, size_t maxLen, size_t alreadySent, size_t fileSize)> ResponseFiller;

////////////////////////

class SDCardClass {
public:
    SDCardClass() { }
    void init(Scheduler& scheduler);
    void loop();
    static bool getTmTime(struct tm* info, time_t time, uint32_t ms);

    void writeValue(uint16_t serial, time_t time, float value);
    bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller);

private:
    void scanCard();
    bool openFile(uint16_t serial, const tm timeinfo, const char* mode, File& file);

private:
    Task _loopTask;

    uint32_t _lastActionTime;
    SDCardState_t _state;

    File _file; // used by getFile
    std::mutex _mutex;
};
extern SDCardClass SDCard;
