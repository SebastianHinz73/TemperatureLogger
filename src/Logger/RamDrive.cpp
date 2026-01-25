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
    //_mutex.lock();

    
    static dataEntry_t* act;
    act = nullptr;

    responseFiller = [&, serial, start, length](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        size_t ret = 0;

        //MessageOutput.printf("RamDriveClass::getFile 0x%X responseFiller maxLen:%d, alreadySent:%d, start:%ld, length:%d\r\n", serial, maxLen, alreadySent, start, length);
        const int EntrySize = 20; // typically entry count 17
        while (maxLen - ret > EntrySize) {
            if (!_ramBuffer->getEntry(serial, start, act)) {
                break;
            }
            if (act->time > start + length) {
                //_mutex.unlock();
                break;
            }
            // e.g. 1766675463;19.12\n
            //int written = snprintf((char*)entryBuffer, sizeof(entryBuffer), "%ld;%.2f\n", act->time, act->value);
            int written = snprintf((char*)&buffer[ret], EntrySize, "%ld;%.2f\n", act->time, act->value);
            ret += written;
            buffer[written - 1] = '\n';
        }

        if (ret == 0) {
            //_mutex.unlock();
        } else { // important to fill the buffer completely, otherwise chunked response ends too early
            for(; ret < maxLen; ret++) {
                buffer[ret] = (ret == maxLen - 1) ? '\n' : ' ';
            }
        }
        return ret;
    };
    return true;
}

bool RamDriveClass::getBackup(size_t bytes, ResponseFiller& responseFiller)
{
    //_mutex.lock();
    static dataEntry_t* act;
    act = nullptr;
    static int written = sizeof(dataEntry_t);

    responseFiller = [&, bytes](uint8_t* buffer, size_t maxLen, size_t alreadySent) -> size_t {
        size_t ret = 0;

        //MessageOutput.printf("RamDriveClass::getBackup responseFiller maxLen:%d, alreadySent:%d, written=%d\r\n", maxLen, alreadySent, written);
        // copy rest from previous call
        if((written != sizeof(dataEntry_t)) && (maxLen > sizeof(dataEntry_t)))
        {
            memcpy(buffer, &((char*)act)[written], sizeof(dataEntry_t) - written);
            ret += sizeof(dataEntry_t) - written;
            //MessageOutput.printf("RamDriveClass::getBackup ret=%d\r\n", ret);
        }

        while (maxLen - ret > 0) {
            if (!_ramBuffer->backupEntry(act)) {
                break;
            }
            // copy partially if not enough space
            written = maxLen - ret >= sizeof(dataEntry_t) ? sizeof(dataEntry_t) : maxLen - ret;
            memcpy(&buffer[ret], act, written);
            ret += written;
        }

        if(bytes < alreadySent + 100000)
        {
            MessageOutput.printf(" %d %d\r\n", alreadySent + ret, bytes);
        }

        if (alreadySent + ret >= bytes) {
            //_mutex.unlock();
            MessageOutput.printf("_mutex.unlock(); %d %d\r\n", alreadySent + ret, bytes);
        }
        //MessageOutput.printf("ret=%d\r\n", ret);
        return ret;
    };
    return true;
}

bool RamDriveClass::restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len)
{
    // Simple restore: append entries from provided buffer into the ram buffer.
    // The buffer is expected to contain a sequence of packed dataEntry_t structures.
    if (data == nullptr || len < (int)sizeof(dataEntry_t)) {
        return false;
    }
    static dataEntry_t incompleteEntry;
    static size_t incompleteLen = 0;

    if(alreadyWritten == 0) {
        memset(&incompleteEntry, 0, sizeof(dataEntry_t));
        incompleteLen = 0;

        _ramBuffer->PowerOnInitialize();
    }

    if(incompleteLen > 0) {
        size_t toCopy = sizeof(dataEntry_t) - incompleteLen;
        if(len < toCopy) {
            toCopy = len;
        }
        memcpy((uint8_t*)&incompleteEntry + incompleteLen, data, toCopy);
        incompleteLen += toCopy;
        data += toCopy;
        len -= toCopy;

        if (incompleteEntry.value >= -200.0f && incompleteEntry.value <= 200.0f) {
            _ramBuffer->writeValue(incompleteEntry.serial, incompleteEntry.time, incompleteEntry.value);
        }
        else
        {
            MessageOutput.printf("RamDriveClass::restoreBackup: Skipping invalid value %.2f\r\n", incompleteEntry.value);
        }
        incompleteLen = 0;
    }

    size_t count = len / sizeof(dataEntry_t);
    incompleteLen = len % sizeof(dataEntry_t);

    const dataEntry_t* entries = (const dataEntry_t*)data;

    // write each entry into ram buffer
    for (size_t i = 0; i < count; i++) {
        const dataEntry_t& e = entries[i];
        // validate value range
        if (e.value < -200.0f || e.value > 200.0f) {
            continue;
        }
        _ramBuffer->writeValue(e.serial, e.time, e.value);
    }

    if(incompleteLen > 0) {
        memcpy((uint8_t*)&incompleteEntry, &data[count], incompleteLen);
    }

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
