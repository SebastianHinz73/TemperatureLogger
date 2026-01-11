// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

#pragma pack(2)
typedef struct
{
    uint16_t serial;
    time_t time;
    float value;
} dataEntry_t; // 2 + 4 + 4 => 10 Bytes

typedef struct
{
    uint32_t id;
    dataEntry_t* start;
    dataEntry_t* first;
    dataEntry_t* last;
    dataEntry_t* end;
} dataEntryHeader_t;

#pragma pack()

#define RAMBUFFER_HEADER_ID 0x12345678

class RamBuffer {
public:
    RamBuffer(uint8_t* buffer, size_t size, uint8_t* cache, size_t cacheSize);
    void PowerOnInitialize();
    bool IntegrityCheck();

    void writeValue(uint16_t serial, time_t time, float value);
    bool getEntry(uint16_t serial, time_t time, dataEntry_t*& act);
    bool backupEntry(dataEntry_t*& act);

    time_t getOldestTime() const { return _header->first->time; }
    time_t getNewestTime() const { return _header->last->time; }

    size_t getTotalElements() const { return _elements; }
    size_t getUsedElements() const { return _header->last >= _header->first ? _header->last - _header->first : _elements; }

private:
    int toIndex(const dataEntry_t* entry) const { return entry - _header->start; }

public:
    dataEntryHeader_t* _header;
    size_t _elements;
    uint8_t* _cache;
    size_t _cacheSize;
};
