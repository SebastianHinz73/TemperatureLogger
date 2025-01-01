// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"
#include "RamBuffer.h"
#include <Arduino.h>
#include <memory>
#include <mutex>
#include <vector>

////////////////////////
#define ENTRY_TO_STRING_SIZE 15

class RamDiskClass : public IDataStoreDevice {
public:
    RamDiskClass();
    ~RamDiskClass()
    {
        delete _ramBuffer;
    }

    static void AllocateRamDisk();
    static void FreeRamDisk();

    // IDataStoreDevice
    virtual void writeValue(uint16_t serial, time_t time, float value);
    virtual bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    virtual bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller);

private:
    time_t getStartOfDay(const tm& timeinfo);

private:
    RamBuffer* _ramBuffer;
    std::mutex _mutex;

private:
    static u8_t* _ramDisk;
    static size_t _ramDiskSize;
    static u8_t* _cache;
    static size_t _cacheSize;
};
extern RamDiskClass* pRamDisk;
