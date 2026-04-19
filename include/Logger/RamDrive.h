// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"
#include "RamBuffer.h"
#include <Arduino.h>
#include <memory>
#include <mutex>
#include <vector>
#include "TimeoutMutex.h"

class RamDriveClass : public IDataStoreDevice {
public:
    RamDriveClass();
    ~RamDriveClass()
    {
        delete _ramBuffer;
    }

    static void AllocateRamDrive();
    static void FreeRamDrive();

    size_t getSizeBytes() const { return _ramBuffer->getTotalElements() * sizeof(dataEntryFEC_t); }
    size_t getUsedBytes() const { return _ramBuffer->getUsedElements() * sizeof(dataEntryFEC_t); }
    time_t getOldestTime() const { return _ramBuffer->getOldestTime(); }
    size_t getRebootCount() const { return _ramBuffer->getRebootCount(); }
    size_t getErrorCount() const { return _ramBuffer->getErrorCount(); }

    // IDataStoreDevice
    virtual void writeValue(uint16_t serial, time_t time, float value);
    virtual bool getFile(uint16_t serial, time_t start, uint32_t length, ResponseFiller& responseFiller);
    virtual bool getBackup(ResponseFiller& responseFiller);
    virtual bool restoreBackup(size_t alreadyWritten, const uint8_t* data, size_t len, bool final);

private:
    void startupCheck();
    time_t getStartOfDay(const tm& timeinfo);

private:
    RamBuffer* _ramBuffer;
    TimeoutMutex _mutexRamDrive;
    volatile bool _restoreInProgress = false;

private:
    static uint8_t* _ramDrive;
    static size_t _ramDriveSize;
    static uint8_t* _cache;
    static size_t _cacheSize;
};
extern RamDriveClass* pRamDrive;
