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

    File file;
    if (!openFile(serial, time, FILE_APPEND, file)) {
        return;
    }

    // write "time_t;Temperature"
    if (!file.printf("%ld;%.2f\n", time, value)) {
        MessageOutput.println("SD card: Append failed");
    }
    file.close();
}

bool SDCardClass::getFile(uint16_t serial, time_t time_start, uint32_t length, ResponseFiller& responseFiller)
{
    responseFiller = [&, serial, time_start](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {

        //MessageOutput.printf("responseFiller 0x%X maxLen:%d, alreadySent:%d, start:%ld\r\n", serial, maxLen, alreadySent, time_start);

        auto findStart = [](uint8_t* buffer, size_t len, time_t start) -> long {
            uint8_t* act = buffer;
            uint8_t* end = buffer + len;

            bool lastNewLine = true;
            while (act < end)
            {
                switch (*act)
                {
                case '\r':
                case '\n':
                    lastNewLine = true;
                    break;
                case ';':
                    break;
                default:
                    if (lastNewLine && (end - act > 11)) // e.g. 1771715258;33.19
                    {
                        lastNewLine = false;
                        time_t t = atoi((const char*)act); // time
                        if (t >= start) {
                            return static_cast<long>(act - buffer);
                        }
                    }
                    break;
                }
                act++;
            }
            return -1;
        };

        long ret = 0;
        if(alreadySent == 0)
        {
            while (true)
            {
                ret = _file.readBytes((char*)buffer, maxLen);
                if (ret == 0)
                {
                    // error, file end
                    MessageOutput.printf("file close 1\r\n");
                    if(_fileOpen)
                    {
                        _fileOpen = false;
                        _file.close();
                        _mutex.unlock();
                    }
                    return ret;
                }
                long start = findStart(buffer, ret, time_start); // maybe one incomplete line -> ignored
                if (start == 0)
                {
                    break; // 24h mode
                }
                else if (start > 0)
                {
                    // found, move file pointer back to start
                    _file.seek(_file.position() - (ret - start));
                    ret = _file.readBytes((char*)buffer, maxLen);
                    break; // OK
                }
            }
        }
        else
        {
            ret = _file.readBytes((char*)buffer, maxLen);
        }

        ret = (ret < 0) ? 0 : ret;

        if (ret < maxLen) {
            if(_fileOpen)
            {
                _fileOpen = false;
                _file.close();
                _mutex.unlock();
            }
        }
        return ret;
    };

    // ignore length here (max 24h)
    // restriction on start & length: start is beginning of a day, length is not longer that 24h
    if (_state != SDCardState_t::InitOk) {
        MessageOutput.println("SD card: getFile invalid state.");
        return false;
    }

    if (!openFile(serial, time_start, FILE_READ, _file)) {
        return false;
    }

    _mutex.lock();
    _fileOpen = true;

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
