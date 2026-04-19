// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "Logger/RamBuffer.h"
#include "MessageOutput.h"
#include <memory>

RamBuffer::RamBuffer(uint8_t* buffer, size_t size, uint8_t* cache, size_t cacheSize)
    : _header((dataEntryHeader_t*)buffer)
    , _elements((size - sizeof(dataEntryHeader_t)) / sizeof(dataEntryFEC_t))
    , _cache(cache)
    , _cacheSize(cacheSize)
{
    // On reset: _header, _cache and _cacheSize is set. The values in PSRAM are not changes/deleted.
    // On power on: do additional initialisation
    // https://www.esp32.com/viewtopic.php?t=35063
}

void RamBuffer::PowerOnInitialize()
{
    _header->start = (dataEntryFEC_t*)(&_header[1]);
    _header->first = _header->start;
    _header->last = _header->start;
    _header->end = &_header->start[_elements];
    _header->rebootCount = 0;
    _header->errorCount = 0;

    _rsHeader.EncodeBlock(_header, _header->ecc);

    _header->first->entry.time = 0;

    // PSRAM uses cache which is cleared after reset -> trigger a flush of the PSRAM-cache to the PSRAM.
    if (_cache != nullptr) {
        memset(_cache, 0, _cacheSize);
    }
}

bool RamBuffer::IntegrityCheck()
{
    MessageOutput.println("IntegrityCheck ...");

    if (_rsHeader.Decode(_header, _header) > 0)
    {
        MessageOutput.println("RamBuffer header ecc failed");
        return false;
    }
    _header->rebootCount++;

    dataEntryFEC_t* act = _header->first;
    size_t oldErrors = 0;
    size_t newErrors = 0;

    for (int i = 0; i < 2; i++) {
        while (act < _header->end) {
            if (act == _header->last) {
                size_t usedElements = getUsedElements();
                float errorRate = usedElements == 0 ? 0.0f : (float)(oldErrors + newErrors) / usedElements * 100.0f;
                MessageOutput.printf("Old errors: %d, New errors: %d, Total errors: %.2f%%\r\n", oldErrors, newErrors, errorRate);

                if(errorRate > 10.0f) {
                    MessageOutput.println("IntegrityCheck failed: Too many errors");
                    return false;
                }
                _header->errorCount = oldErrors + newErrors;
                _rsHeader.EncodeBlock(_header, _header->ecc);
                flushCache();

                MessageOutput.println("IntegrityCheck done");
                return true;
            }

            if(act->entry.time == 0)
            {
                oldErrors++;
            }
            else if (_rsData.Decode(act, act) > 0)
            {
                act->entry.time = 0; // set time to 0 on ecc error, so it is ignored in getEntry
                newErrors++;
            }

            //MessageOutput.printf("%x, %ld, %05.2f\r\n", act->entry.serial, act->entry.time, act->entry.value);
            act++;
        }
        act = _header->start;
    }

    MessageOutput.println("RamBuffer ecc error");
    return false;
}

void RamBuffer::writeValue(uint16_t serial, time_t time, float value)
{
    _header->last->entry.serial = serial;
    _header->last->entry.time = time;
    _header->last->entry.value = value;
     //MessageOutput.printf("writeValue: ## %d: 0x%x, (%ld, %05.2f)\r\n", toIndex(_header->last), _header->last->serial, _header->last->time, _header->last->value);

    _rsData.EncodeBlock(_header->last, _header->last->ecc);

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
    _rsHeader.EncodeBlock(_header, _header->ecc);
    flushCache();
}

bool RamBuffer::getEntry(uint16_t serial, time_t time, dataEntry_t*& act)
{
    // start with _header->first, then increment
    if (act == nullptr) {
        act = findStart(time);
    } else if (act == toEntry(_header->last)) {
        return false;
    } else {
        act = toEntry(toFec(act) + 1);
    }

    for (int i = 0; i < 2; i++) {
        while (act < toEntry(_header->end)) {

            // end check
            if (act->time >= time + 24 * 60 * 60 || act == toEntry(_header->last)) {
                return false;
            }

            // ecc && serial && time check
            if (act->time == 0 || serial != act->serial || act->time < time) {
                act = toEntry(toFec(act) + 1);
                continue;
            }

            return true;
        }
        act = toEntry(_header->start); // start again with _header->start
    }
    return false;
}

dataEntry_t* RamBuffer::findStart(time_t time)
{
    size_t count = getUsedElements();
    if (count == 0) {
        return toEntry(_header->last);
    }

    size_t capacity = _header->end - _header->start;
    size_t lo = 0, hi = count;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        // index to pointer
        dataEntryFEC_t* p = _header->first + mid;
        if (p >= _header->end) {
            p -= capacity;
        }

        if (p->entry.time == 0 || p->entry.time < time) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    // calculate result pointer from index
    dataEntryFEC_t* result = _header->first + lo;
    if (result >= _header->end) {
        result -= capacity;
    }

    return (lo >= count) ? toEntry(_header->last) : toEntry(result);
}

bool RamBuffer::getBackup(ResponseFiller& responseFiller)
{
    auto act = std::make_shared<uint8_t*>(nullptr);

    responseFiller = [this, act](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        size_t ret = 0;
        if(alreadySent == 0) {
            *act = reinterpret_cast<uint8_t*>(_header->first);
        }

        size_t actLen = 0;

        // copy from first to end
        if (*act < reinterpret_cast<uint8_t*>(_header->end)) {
            actLen = min(maxLen, static_cast<size_t>(reinterpret_cast<uint8_t*>(_header->end) - *act));

            memcpy(buffer, *act, actLen);
            ret += actLen;
            *act += actLen;
            buffer += actLen;
            if(*act == reinterpret_cast<uint8_t*>(_header->end)) {
                *act = reinterpret_cast<uint8_t*>(_header->start);
            }
        }
        // copy from start to last
        actLen = maxLen - actLen;
        if(actLen > 0) {
            actLen = min(actLen, static_cast<size_t>(reinterpret_cast<uint8_t*>(_header->last) - *act));

            memcpy(buffer, *act, actLen);
            ret += actLen;
            *act += actLen;
        }
        return ret;
    };
    return true;
}

bool RamBuffer::restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len, bool final)
{
    if(alreadyWritten == 0) {
        PowerOnInitialize();
        _restorePos = reinterpret_cast<uint8_t*>(_header->first);
    }

    if(alreadyWritten <= getTotalElements() * sizeof(dataEntryFEC_t)) {
        len = min(len, getTotalElements() * sizeof(dataEntryFEC_t) - alreadyWritten);
        memcpy(_restorePos, data, len);
        _restorePos += len;
    }
    else
    {
        MessageOutput.printf("RamBuffer::restoreBackup overflow alreadyWritten=%d, len=%d, max=%d\r\n", alreadyWritten, len, getTotalElements() * sizeof(dataEntryFEC_t));
    }

    if(final) {
        // adjust _header->last
        size_t entries = (_restorePos - reinterpret_cast<uint8_t*>(_header->first)) / sizeof(dataEntryFEC_t);
        _header->last = _header->first + entries;
        _rsHeader.EncodeBlock(_header, _header->ecc);
        MessageOutput.printf("RamBuffer::restoreBackup final entries=%d, first=%d, last=%d\r\n", entries, toIndex(_header->first), toIndex(_header->last));

        flushCache();
        _restorePos = nullptr;
    }

    return true;
}

void RamBuffer::flushCache()
{
    if (_cache != nullptr) {
        // Here _cache is used to read from another PSRAM area and thus trigger a flush of the PSRAM-cache to the PSRAM.
        volatile uint32_t sum = 0;
        for (uint32_t i = 0; i < 64 * 1024 / sizeof(uint32_t); i++) {
            sum += ((volatile uint32_t*)_cache)[i];
        }
    }
}
