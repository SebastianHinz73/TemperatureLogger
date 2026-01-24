// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"
#include "RamBuffer.h"
#include <Arduino.h>
#include <memory>
#include <mutex>
#include <vector>

class RamDriveClass : public IDataStoreDevice {
public:
    RamDriveClass();
    ~RamDriveClass()
    {
        delete _ramBuffer;
    }

    static void AllocateRamDrive();
    static void FreeRamDrive();

    size_t getSizeBytes() const { return _ramBuffer->getTotalElements() * sizeof(dataEntry_t); }
    size_t getUsedBytes() const { return _ramBuffer->getUsedElements() * sizeof(dataEntry_t); }
    time_t getOldestTime() const { return _ramBuffer->getOldestTime(); }
    time_t getNewestTime() const { return _ramBuffer->getNewestTime(); }

    // IDataStoreDevice
    virtual void writeValue(uint16_t serial, time_t time, float value);
    virtual bool getFile(uint16_t serial, time_t start, uint32_t length, ResponseFiller& responseFiller);
    virtual bool getBackup(size_t bytes, ResponseFiller& responseFiller);

private:
    time_t getStartOfDay(const tm& timeinfo);

private:
    RamBuffer* _ramBuffer;
    std::mutex _mutex;

private:
    static uint8_t* _ramDrive;
    static size_t _ramDriveSize;
    static uint8_t* _cache;
    static size_t _cacheSize;
};
extern RamDriveClass* pRamDrive;
