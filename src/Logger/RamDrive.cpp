// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "Logger/RamDrive.h"
#include "MessageOutput.h"
#include "PinMapping.h"

RamDriveClass* pRamDrive = nullptr;

uint8_t* RamDriveClass::_ramDrive = nullptr;
size_t RamDriveClass::_ramDriveSize = 0;
uint8_t* RamDriveClass::_cache = nullptr;
size_t RamDriveClass::_cacheSize = 0;

RamDriveClass::RamDriveClass()
{
    _ramBuffer = new RamBuffer(_ramDrive, _ramDriveSize, _cache, _cacheSize);
    if (!_ramBuffer->IntegrityCheck()) {
        MessageOutput.printf("Initialize empty RamDrive with %d entries. ", _ramBuffer->getTotalElements());
        _ramBuffer->PowerOnInitialize();
    } else {
        MessageOutput.printf("Initialize RamDrive. %d entries found. %.2f percent used. ", _ramBuffer->getUsedElements(), _ramBuffer->getUsedElements() * 100.0f / _ramBuffer->getTotalElements());
    }
}

void RamDriveClass::AllocateRamDrive()
{
    if (ESP.getPsramSize() > 0) // PSRAM available
    {
        _ramDriveSize = ESP.getPsramSize() * 0.8f;
        _ramDrive = new uint8_t[_ramDriveSize];

        uint32_t dummySize = ESP.getPsramSize() * 0.1f;
        auto dummy = new uint8_t[dummySize];

        _cacheSize = 64 * 1024;
        _cache = new uint8_t[_cacheSize]; // (uint8_t*)ps_malloc(_cacheSize);

        delete[] dummy;
    } else // use normal RAM
    {
        _ramDriveSize = 4096;
        _ramDrive = new uint8_t[_ramDriveSize];
        _cacheSize = 0;
        _cache = nullptr;
    }
}

void RamDriveClass::FreeRamDrive()
{
    if (_ramDrive != nullptr) {
        delete[] _ramDrive;
        _ramDrive = nullptr;
    }
    if (_cache != nullptr) {
        delete[] _cache;
        _cache = nullptr;
    }
}

void RamDriveClass::writeValue(uint16_t serial, time_t time, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _ramBuffer->writeValue(serial, time, value);
}

bool RamDriveClass::getFile(uint16_t serial, time_t start, uint32_t length, ResponseFiller& responseFiller)
{
    _mutex.lock();

    static dataEntry_t* act;
    act = nullptr;

    responseFiller = [&, serial, start, length](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        size_t ret = 0;

        // MessageOutput.printf("RamDriveClass::getFile responseFiller maxLen:%d, alreadySent:%d, start:%ld, length:%d\r\n", maxLen, alreadySent, start, length);

        const int EntrySize = 20; // typically entry count 17
        while (maxLen - ret > EntrySize) {
            if (!_ramBuffer->getEntry(serial, start, act)) {
                break;
            }
            if (act->time > start + length) {
                break;
            }
            // e.g. 1766675463;19.12\n
            int written = snprintf((char*)&buffer[ret], EntrySize, "%ld;%.2f\n", act->time, act->value);
            buffer[written - 1] = '\n';

            ret += written;
        }

        if (ret == 0) {
            _mutex.unlock();
        }
        return ret;
    };

    return true;
}

time_t RamDriveClass::getStartOfDay(const tm& timeinfo)
{
    tm info;
    memcpy(&info, &timeinfo, sizeof(tm));
    info.tm_hour = 0;
    info.tm_min = 0;
    info.tm_sec = 0;
    return mktime(&info);
}
