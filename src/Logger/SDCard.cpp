// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "Logger/SDCard.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "PinMapping.h"

SDCardClass* pSDCard = nullptr;

SDCardClass::SDCardClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&SDCardClass::loop, this))
    , _state(SDCardState_t::Init)
{
}

void SDCardClass::init(Scheduler& scheduler)
{
    const PinMapping_t& pin = PinMapping.get();

    SPI.begin(pin.sd_sck, pin.sd_miso, pin.sd_mosi);
    _lastActionTime = millis();

    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void SDCardClass::loop()
{
    switch (_state) {
    case SDCardState_t::Init:
        if (millis() - _lastActionTime > 150) {
            scanCard();
            _lastActionTime = millis();
        }
        break;
    default:
        return;
    }
}

void SDCardClass::scanCard()
{
    static int ScanCnt = 10;
    if (ScanCnt-- == 0) {
        _state = SDCardState_t::InitFailure;
        MessageOutput.println("Card Mount Failed");
    }

    const PinMapping_t& pin = PinMapping.get();
    if (SD.begin(pin.sd_cs, SPI)) {
        if (CARD_NONE == SD.cardType()) {
            _state = SDCardState_t::InitFailure;
            MessageOutput.println("No SD card attached");
        } else {
            MessageOutput.println("SD card attached");
            _state = SDCardState_t::InitOk;
        }
    }
}

void SDCardClass::writeValue(uint16_t serial, time_t time, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_state != SDCardState_t::InitOk) {
        MessageOutput.printf("SD card: writeValue invalid state. %d\r\n", _state);
        return;
    }
    tm timeinfo;
    if (!Datastore.getTmTime(&timeinfo, time, 5)) {
        MessageOutput.println("SD card: Get timeinfo failed.");
        return;
    }

    File file;
    if (!openFile(serial, time, FILE_APPEND, file)) {
        return;
    }

    // write "hh:mm:ss;Temperature"
    if (!file.printf("%02d:%02d:%02d;%.2f\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, value)) {
        MessageOutput.println("SD card: Append failed");
    }
    file.close();
}

bool SDCardClass::getFile(uint16_t serial, time_t start, uint32_t length, ResponseFiller& responseFiller)
{
    // restriction on start & length: start is beginning of a day, length is not longer that 24h
    responseFiller = [&](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        size_t ret = _file.readBytes((char*)buffer, maxLen);
        if (ret == 0) {
            _file.close();
            _mutex.unlock();
        }
        return ret;
    };

    if (_state != SDCardState_t::InitOk) {
        MessageOutput.println("SD card: getFile invalid state.");
        return false;
    }

    if (!openFile(serial, start, FILE_READ, _file)) {
        return false;
    }

    _mutex.lock();
    return true;
}

bool SDCardClass::openFile(uint16_t serial, const time_t time, const char* mode, File& file)
{
    struct tm timeinfo;
    Datastore.getTmTime(&timeinfo, time, 5);

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "/%04d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
    if (!SD.exists(buffer)) {
        snprintf(buffer, sizeof(buffer), "/%04d", timeinfo.tm_year + 1900);
        SD.mkdir(buffer);
        snprintf(buffer, sizeof(buffer), "/%04d/%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
        SD.mkdir(buffer);
    }
    snprintf(&buffer[strlen(buffer)], sizeof(buffer) - strlen(buffer), "/%02d_%04X.txt", timeinfo.tm_mday, serial);

    file = SD.open(buffer, mode);
    if (!file) {
        MessageOutput.printf("Failed to open file for %s\r\n", mode);
        return false;
    }
    return true;
}
