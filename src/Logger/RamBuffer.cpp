// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "Logger/RamBuffer.h"
#include "MessageOutput.h"

RamBuffer::RamBuffer(uint8_t* buffer, size_t size, uint8_t* cache, size_t cacheSize)
    : _header((dataEntryHeader_t*)buffer)
    , _elements((size - sizeof(dataEntryHeader_t)) / sizeof(dataEntry_t))
    , _cache(cache)
    , _cacheSize(cacheSize)
{
    // On reset: _header, _cache and _cacheSize is set. The values in PSRAM are not changes/deleted.
    // On power on: do additional initialisation
    // https://www.esp32.com/viewtopic.php?t=35063
}

void RamBuffer::PowerOnInitialize()
{
    _header->id = RAMBUFFER_HEADER_ID;
    _header->start = (dataEntry_t*)(&_header[1]);
    _header->first = _header->start;
    _header->last = _header->start;
    _header->end = &_header->start[_elements];

    _header->first->time = 0;

    // PSRAM uses cache which is cleared after reset -> trigger a flush of the PSRAM-cache to the PSRAM.
    if (_cache != nullptr) {
        memset(_cache, 0, _cacheSize);
    }
}

bool RamBuffer::IntegrityCheck()
{
    if (_header->id != RAMBUFFER_HEADER_ID) {
        MessageOutput.printf("RamBuffer 0x%x is not expected id 0x%x\r\n", _header->id, RAMBUFFER_HEADER_ID);
        return false;
    }

    if (_elements != _header->end - _header->start) {
        MessageOutput.println("RamBuffer _elements changed");
        return false;
    }

    if (_header->first < _header->start || _header->first > _header->end || _header->last < _header->start || _header->last > _header->end) {
        MessageOutput.println("RamBuffer first/last out of range");
        return false;
    }

    dataEntry_t* act = _header->first;

    for (int i = 0; i < 2; i++) {
        while (act < _header->end) {
            if (act == _header->last) {
                return true;
            }

            if (act->value < -200 || act->value > 200) {
                MessageOutput.println("RamBuffer Value out of range");
                return false;
            }

            //MessageOutput.printf("%x, %ld, %05.2f\r\n", act->serial, act->time, act->value);
            act++;
        }
        act = _header->start;
    }

    MessageOutput.println("RamBuffer unknown error");
    return false;
}

void RamBuffer::writeValue(uint16_t serial, time_t time, float value)
{
    _header->last->serial = serial;
    _header->last->time = time;
    _header->last->value = value;
     MessageOutput.printf("writeValue: ## %d: 0x%x, (%ld, %05.2f)\r\n", toIndex(_header->last), _header->last->serial, _header->last->time, _header->last->value);

    _header->last++;

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

    if (_cache != nullptr) {
        // Here _cache is used to read from another PSRAM area and thus trigger a flush of the PSRAM-cache to the PSRAM.
        uint32_t sum = 0;
        for (uint32_t i = 0; i < 64 * 1024 / sizeof(uint32_t); i++) {
            sum += ((uint32_t*)_cache)[i];
        }
        // MessageOutput.printf("sum %d\r\n", sum);
    }
}

bool RamBuffer::getEntry(uint16_t serial, time_t time, dataEntry_t*& act)
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

bool RamBuffer::getBackup(ResponseFiller& responseFiller)
{
    static uint8_t* act = nullptr;

    responseFiller = [&](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        size_t ret = 0;
        if(alreadySent == 0) {
            act = static_cast<uint8_t*>(static_cast<void*>(_header->first));
        }

        size_t actLen = 0;

        // copy from first to end
        if (act < static_cast<uint8_t*>(static_cast<void*>(_header->end))) {
            actLen = min(maxLen, static_cast<size_t>(static_cast<uint8_t*>(static_cast<void*>(_header->end)) - act));

            memcpy(buffer, act, actLen);
            ret += actLen;
            act += actLen;
            buffer += actLen;
            if(act == static_cast<uint8_t*>(static_cast<void*>(_header->end))) {
                act = static_cast<uint8_t*>(static_cast<void*>(_header->start));
            }
        }
        // copy from start to last
        actLen = maxLen - actLen;
        if(actLen > 0) {
            actLen = min(actLen, static_cast<size_t>(static_cast<uint8_t*>(static_cast<void*>(_header->last)) - act));

            memcpy(buffer, act, actLen);
            ret += actLen;
            act += actLen;
        }
        return ret;
    };
    return true;
}

bool RamBuffer::restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len, bool final)
{
    static uint8_t* act = nullptr;

    if(alreadyWritten == 0) {
        PowerOnInitialize();
        act = static_cast<uint8_t*>(static_cast<void*>(_header->first));
    }

    if(alreadyWritten + len > _elements * sizeof(dataEntry_t)) {
        MessageOutput.printf("RamBuffer::restoreBackup overflow alreadyWritten=%d, len=%d, max=%d\r\n", alreadyWritten, len, _elements * sizeof(dataEntry_t));
        return false;
    }

    memcpy(act, data, len);
    act += len;

    if(final) {
        // adjust _header->last
        size_t entries = (alreadyWritten + len) / sizeof(dataEntry_t);
        _header->last = _header->first + entries;
        MessageOutput.printf("RamBuffer::restoreBackup final entries=%d, first=%d, last=%d\r\n", entries, toIndex(_header->first), toIndex(_header->last));
    }

    if (_cache != nullptr) {
        // Here _cache is used to read from another PSRAM area and thus trigger a flush of the PSRAM-cache to the PSRAM.
        uint32_t sum = 0;
        for (uint32_t i = 0; i < 64 * 1024 / sizeof(uint32_t); i++) {
            sum += ((uint32_t*)_cache)[i];
        }
        // MessageOutput.printf("sum %d\r\n", sum);
    }
    return true;
}
