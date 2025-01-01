// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "Logger/RamDisk.h"
#include "MessageOutput.h"
#include "PinMapping.h"

//////////////////////////////////////////
PSRamSensorBuffer::PSRamSensorBuffer(u8_t* buffer, size_t size, u8_t* cache, size_t cacheSize, bool powerOn)
    : _header((dataEntryHeader_t*)buffer)
    , _cache(cache)
    , _cacheSize(cacheSize)
{
    uint32_t elements = (size - sizeof(dataEntryHeader_t)) / sizeof(dataEntry_t);
    MessageOutput.printf("elements in use %d\r\n", _header->last - _header->first);

    if (!powerOn) {
        return;
    }
    // https://www.esp32.com/viewtopic.php?t=35063

    _header->start = (dataEntry_t*)(buffer + sizeof(dataEntryHeader_t));
    _header->first = _header->start;
    _header->last = _header->start;

    _header->end = &_header->start[elements]; // TODO calculate, zero memory
    //_header->cache = (dataEntryEnd_t*)&buffer[size - sizeof(dataEntryEnd_t)];
    memset(_cache, 0, _cacheSize);
}

void PSRamSensorBuffer::writeValue(uint16_t serial, time_t time, float value)
{
    _header->last->serial = serial;
    _header->last->time = time;
    _header->last->value = value;
    MessageOutput.printf("writeValue1: ");
    debugPrint(_header->last);
    _header->last++;
    // MessageOutput.printf("writeValue1: _header->start(%d), _header->first(%d), _header->last(%d), _header->end(%d)\r\n", toIndex(_header->start), toIndex(_header->first), toIndex(_header->last), toIndex(_header->end));
    MessageOutput.printf("### &header: 0x%x\r\n", &_header);
    MessageOutput.printf("### header: 0x%x\r\n", _header);
    MessageOutput.printf("### start: 0x%x\r\n", _header->start);
    MessageOutput.printf("### end: 0x%x\r\n", _header->end);
    MessageOutput.printf("### first: 0x%x\r\n", _header->first);
    MessageOutput.printf("### last: 0x%x\r\n", _header->last);
    MessageOutput.printf("### in use: %d\r\n", _header->last - _header->first);

    // last on end -> begin with start
    if (_header->last == _header->end) {
        _header->last = _header->start;
    }

    // last overwrites first -> increase first
    if (_header->last == _header->first) {
        _header->first++;
        if (_header->first == _header->end) {
            _header->first = _header->start;
        }
    }

    uint32_t sum = 0;
    for (uint32_t i = 0; i < 64 * 1024; i++) {
        sum += _cache[i];
    }
    MessageOutput.printf("sum %d\r\n", sum);
    // MessageOutput.printf("writeValue2: _header->start(%d), _header->first(%d), _header->last(%d), _header->end(%d)\r\n", toIndex(_header->start), toIndex(_header->first), toIndex(_header->last), toIndex(_header->end));
}

int PSRamSensorBuffer::toIndex(const dataEntry_t* entry)
{
    return (entry - _header->start);
}

void PSRamSensorBuffer::debugPrint(const dataEntry_t* entry)
{
    MessageOutput.printf("## %d: 0x%x, (%d, %05.2f)\r\n", toIndex(entry), entry->serial, entry->time, entry->value);
}

bool PSRamSensorBuffer::getEntry(uint16_t serial, time_t time, dataEntry_t*& act)
{
    // start with _header->first, then increment
    if (act == nullptr) {
        act = _header->first;
    } else if (act == _header->last) {
        return false;
    } else {
        act++;
    }

    for (int i = 0; i < 2; i++) {
        while (act < _header->end) {

            // end check
            if (act->time >= time + 24 * 60 * 60 || act == _header->last) {
                return false;
            }

            // serial && time check
            if (serial != act->serial || act->time < time) {
                act++;
                continue;
            }

            return true;
        }
        act = _header->start; // start again with _header->start
    }
    return false;
}

//////////////////////////////////////////

RamDiskClass* pRamDisk = nullptr;

u8_t* RamDiskClass::_ramDisk = nullptr;
size_t RamDiskClass::_ramDiskSize = 0;
u8_t* RamDiskClass::_cache = nullptr;
size_t RamDiskClass::_cacheSize = 0;

RamDiskClass::RamDiskClass()
{
    MessageOutput.printf("RamDiskClass 0x%x, %d => %d\r\n", _ramDisk, _ramDiskSize, _ramDisk[0]);

    dataEntryHeader_t* header = (dataEntryHeader_t*)_ramDisk;

    if (header->id == RAMDISK_HEADER_ID) {
        MessageOutput.printf("Reset Try to restore data\r\n");
        _psRamSensorBuffer = new PSRamSensorBuffer(_ramDisk, _ramDiskSize, _cache, _cacheSize, false);
        // TODO checks
    } else {
        header->id = RAMDISK_HEADER_ID;

        MessageOutput.printf("Power On Reset\r\n");

        _psRamSensorBuffer = new PSRamSensorBuffer(_ramDisk, _ramDiskSize, _cache, _cacheSize, true);
    }
}

void RamDiskClass::AllocateRamDisk()
{
    if (ESP.getPsramSize() > 0) // PSRAM available
    {
        _ramDiskSize = ESP.getPsramSize() * 0.8f;
        _ramDisk = new u8_t[_ramDiskSize];

        uint32_t dummySize = ESP.getPsramSize() * 0.1f;
        auto dummy = new u8_t[dummySize];

        _cacheSize = 64 * 1024;
        _cache = new u8_t[_cacheSize]; // (u8_t*)ps_malloc(_cacheSize);

        delete[] dummy;
    } else // use normal RAM
    {
        _ramDiskSize = 500;
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
    _psRamSensorBuffer->writeValue(serial, time, value);
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
            if (!_psRamSensorBuffer->getEntry(serial, start_of_day, act)) {
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
