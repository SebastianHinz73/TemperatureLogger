// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "Logger/RamDisk.h"
#include "MessageOutput.h"
#include "PinMapping.h"

RamDiskClass* pRamDisk = nullptr;

u8_t* RamDiskClass::_ramDisk = nullptr;
size_t RamDiskClass::_ramDiskSize = 0;
u8_t* RamDiskClass::_cache = nullptr;
size_t RamDiskClass::_cacheSize = 0;

RamDiskClass::RamDiskClass()
{
    _ramBuffer = new RamBuffer(_ramDisk, _ramDiskSize, _cache, _cacheSize);
    if (!_ramBuffer->IntegrityCheck()) {
        MessageOutput.printf("Initialize empty RamDisk with %d entries.\r\n", _ramBuffer->getTotalElements());
        _ramBuffer->PowerOnInitialize();
    } else {
        MessageOutput.printf("Initialize RamDisk. %d entries found. %.2f percent used.\r\n", _ramBuffer->getUsedElements(), _ramBuffer->getUsedElements() * 100.0f / _ramBuffer->getTotalElements());
    }
}

void RamDiskClass::AllocateRamDisk()
{
    if (ESP.getPsramSize() > 0) // PSRAM available
    {
        _ramDiskSize = ESP.getPsramSize() * 0.8f;
        //_ramDiskSize = 600;
        _ramDisk = new u8_t[_ramDiskSize];

        uint32_t dummySize = ESP.getPsramSize() * 0.1f;
        auto dummy = new u8_t[dummySize];

        _cacheSize = 64 * 1024;
        _cache = new u8_t[_cacheSize]; // (u8_t*)ps_malloc(_cacheSize);

        delete[] dummy;
    } else // use normal RAM
    {
        _ramDiskSize = 4096;
        _ramDisk = new u8_t[_ramDiskSize];
        _cacheSize = 0;
        _cache = nullptr;
    }
}

void RamDiskClass::FreeRamDisk()
{
    if (_ramDisk != nullptr) {
        delete[] _ramDisk;
        _ramDisk = nullptr;
    }
    if (_cache != nullptr) {
        delete[] _cache;
        _cache = nullptr;
    }
}

void RamDiskClass::writeValue(uint16_t serial, time_t time, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _ramBuffer->writeValue(serial, time, value);
}

bool RamDiskClass::getFileSize(uint16_t serial, const tm& timeinfo, size_t& size)
{
    std::lock_guard<std::mutex> lock(_mutex);
    size = 0;

    return true;
}

bool RamDiskClass::getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller)
{
    _mutex.lock();

    static dataEntry_t* act;
    act = nullptr;
    time_t start_of_day = getStartOfDay(timeinfo);

    responseFiller = [&, serial, start_of_day](uint8_t* buffer, size_t maxLen, size_t alreadySent, size_t fileSize) -> size_t {
        size_t ret = 0;
        size_t maxCnt = maxLen / ENTRY_TO_STRING_SIZE;

        // MessageOutput.printf("RamDiskClass::getFile responseFiller maxLen:%d, alreadySent:%d, fileSize:%d, maxCnt:%d\r\n", maxLen, alreadySent, fileSize, maxCnt);
        for (size_t cnt = 0; cnt < maxCnt; cnt++) {
            if (!_ramBuffer->getEntry(serial, start_of_day, act)) {
                break;
            }
            int h = (act->time - start_of_day) / 3600;
            int min = ((act->time - start_of_day) - h * 3600) / 60;
            int sec = (act->time - start_of_day) - h * 3600 - min * 60;
            snprintf((char*)&buffer[cnt * ENTRY_TO_STRING_SIZE], ENTRY_TO_STRING_SIZE, "%02d:%02d:%02d;%05.2f\n", h, min, sec, act->value);

            buffer[(cnt + 1) * ENTRY_TO_STRING_SIZE - 1] = '\n';
            ret += ENTRY_TO_STRING_SIZE;
        }

        if (ret == 0) {
            _mutex.unlock();
        }
        return ret;
    };

    return true;
}

time_t RamDiskClass::getStartOfDay(const tm& timeinfo)
{
    tm info;
    memcpy(&info, &timeinfo, sizeof(tm));
    info.tm_hour = 0;
    info.tm_min = 0;
    info.tm_sec = 0;
    return mktime(&info);
}
