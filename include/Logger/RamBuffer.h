// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include "IDataStoreDevice.h"
#include "RS-FEC.h"

#pragma pack(push, 2)
const uint8_t DATAENTRY_ECC_LENGTH = 6;  // Max corrected bytes ECC_LENGTH/2

typedef struct
{
    uint16_t serial;
    time_t time;
    float value;
} dataEntry_t; // 2 + 4 + 4 => 10 Bytes

typedef struct
{
    dataEntry_t entry;
    char ecc[DATAENTRY_ECC_LENGTH];
} dataEntryFEC_t;

static inline dataEntry_t* toEntry(dataEntryFEC_t* p) { return &p->entry; }
static inline dataEntryFEC_t* toFec(dataEntry_t* p) { return reinterpret_cast<dataEntryFEC_t*>(p); }

///
const uint8_t HEADER_ECC_LENGTH = 96;  // Max corrected bytes ECC_LENGTH/2
typedef struct
{
    dataEntryFEC_t* start;
    dataEntryFEC_t* first;
    dataEntryFEC_t* last;
    dataEntryFEC_t* end;
    size_t rebootCount;
    size_t errorCount;
    char ecc[HEADER_ECC_LENGTH];
} dataEntryHeader_t;

const int HEADER_MSG_LENGTH = sizeof(dataEntryHeader_t) - HEADER_ECC_LENGTH;

#pragma pack(pop)

class RamBuffer {
public:
    RamBuffer(uint8_t* buffer, size_t size, uint8_t* cache, size_t cacheSize);
    void PowerOnInitialize();
    bool IntegrityCheck();
    void flushCache();

    void writeValue(uint16_t serial, time_t time, float value);
    bool getEntry(uint16_t serial, time_t time, dataEntry_t*& act);
    bool getBackup(ResponseFiller& responseFiller);
    bool restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len, bool final);

    time_t getOldestTime() const {
        if (_header->first == _header->last) return 0;
        return _header->first->entry.time;
    }

    size_t getTotalElements() const { return _elements-1; }
    size_t getUsedElements() const { return _header->last >= _header->first ? _header->last - _header->first : getTotalElements(); }
    size_t getRebootCount() const { return _header->rebootCount; }
    size_t getErrorCount() const { return _header->errorCount; }

private:
    int toIndex(const dataEntryFEC_t* entry) const { return entry - _header->start; }
    dataEntry_t* findStart(time_t time);

private:
    dataEntryHeader_t* _header;
    size_t _elements;
    uint8_t* _cache;
    size_t _cacheSize;
    uint8_t* _restorePos = nullptr;
    RS::ReedSolomon<sizeof(dataEntry_t), DATAENTRY_ECC_LENGTH> _rsData;
    RS::ReedSolomon<HEADER_MSG_LENGTH, HEADER_ECC_LENGTH> _rsHeader;
};
