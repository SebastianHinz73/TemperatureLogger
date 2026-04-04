// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include "IDataStoreDevice.h"
#include "RS-FEC.h"

#pragma pack(2)
const uint8_t DATAENTRY_ECC_LENGTH = 6;  // Max corrected bytes ECC_LENGTH/2

typedef struct
{
    uint16_t serial;
    time_t time;
    float value;
} dataEntry_t; // 2 + 4 + 4 => 10 Bytes

typedef struct
{
    uint16_t serial;
    time_t time;
    float value;
    char ecc[DATAENTRY_ECC_LENGTH];
} dataEntryFEC_t; // 2 + 4 + 4 + 6 => 16 Bytes

#define TO_ENTRY(x) static_cast<dataEntry_t*>(static_cast<void*>(x))
#define TO_FEC(x) static_cast<dataEntryFEC_t*>(static_cast<void*>(x))

///
const uint8_t HEADER_ECC_LENGTH = 64;  // Max corrected bytes ECC_LENGTH/2
const int HEADER_MSG_LENGTH = 4*sizeof(dataEntryFEC_t*);

typedef struct
{
    dataEntryFEC_t* start;
    dataEntryFEC_t* first;
    dataEntryFEC_t* last;
    dataEntryFEC_t* end;
    char ecc[HEADER_ECC_LENGTH];
} dataEntryHeader_t;

#pragma pack()

class RamBuffer {
public:
    RamBuffer(uint8_t* buffer, size_t size, uint8_t* cache, size_t cacheSize);
    void PowerOnInitialize();
    bool IntegrityCheck();

    void writeValue(uint16_t serial, time_t time, float value);
    bool getEntry(uint16_t serial, time_t time, dataEntry_t*& act);
    bool getBackup(ResponseFiller& responseFiller);
    bool restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len, bool final);

    time_t getOldestTime() const {
        if (_header->first == _header->last) return 0;
        return _header->first->time;
    }
    time_t getNewestTime() const { return _header->last->time; }

    size_t getTotalElements() const { return _elements-1; }
    size_t getUsedElements() const { return _header->last >= _header->first ? _header->last - _header->first : getTotalElements(); }

private:
    int toIndex(const dataEntryFEC_t* entry) const { return entry - _header->start; }
    dataEntry_t* findStart(time_t time);

public:
    dataEntryHeader_t* _header;
    size_t _elements;
    uint8_t* _cache;
    size_t _cacheSize;
    RS::ReedSolomon<sizeof(dataEntry_t), DATAENTRY_ECC_LENGTH> _rsData;
    RS::ReedSolomon<HEADER_MSG_LENGTH, HEADER_ECC_LENGTH> _rsHeader;
};
