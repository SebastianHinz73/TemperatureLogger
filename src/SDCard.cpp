// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "SDCard.h"
#include "MessageOutput.h"

SDCardClass SDCard;

void SDCardClass::init(Scheduler& scheduler)
{
    // const PinMapping_t& pin = PinMapping.get();

    SPI.begin(12, 15, 13);
    _lastActionTime = millis();

    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&SDCardClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
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

    if (SD.begin(14, SPI)) {
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
    if (_state != SDCardState_t::InitOk) {
        MessageOutput.println("SD card: writeValue invalid state.");
        return;
    }
    tm timeinfo;
    if (!getTmTime(&timeinfo, time, 5)) {
        MessageOutput.println("SD card: Get timeinfo failed.");
        return;
    }

    File file;
    if (!openFile(serial, timeinfo, FILE_APPEND, file)) {
        return;
    }

    // write "hh:mm:ss;Temperature"
    if (!file.printf("%02d:%02d:%02d;%.2f\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, value)) {
        MessageOutput.println("SD card: Append failed");
    }
    file.close();
}

bool SDCardClass::getFileSize(uint16_t serial, const tm& timeinfo, size_t& size)
{
    if (_state != SDCardState_t::InitOk) {
        MessageOutput.println("SD card: getFileSize invalid state.");
        return false;
    }

    File file;
    if (!openFile(serial, timeinfo, FILE_READ, file)) {
        return false;
    }

    size = file.size();
    file.close();

    return true;
}

bool SDCardClass::getFile(uint16_t serial, const tm& timeinfo, char* buffer, size_t& size)
{
    if (_state != SDCardState_t::InitOk) {
        MessageOutput.println("SD card: getFile invalid state.");
        return false;
    }

    File file;
    if (!openFile(serial, timeinfo, FILE_READ, file)) {
        return false;
    }
    size_t ret = file.readBytes(buffer, size);
    if (ret != size) {
        MessageOutput.println("SD card: unexpected ret != size.");
        if (ret < size) {
            size = ret;
        }
    }
    file.close();

    return true;
}

bool SDCardClass::getTmTime(struct tm* info, time_t time, uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) <= ms) {
        localtime_r(&time, info);
        if (info->tm_year > (2016 - 1900)) {
            return true;
        }
        delay(10);
    }
    return false;
}

bool SDCardClass::openFile(uint16_t serial, const tm timeinfo, const char* mode, File& file)
{
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
