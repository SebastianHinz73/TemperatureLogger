// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"
#include <Arduino.h>
#include <memory>
#include <mutex>
#include <vector>

#pragma pack(2)
typedef struct
{
    uint16_t serial;
    time_t time;
    float value;
} dataEntry_t; // 2 + 4 + 4 => 10 Bytes

typedef struct
{
    uint32_t buffer[64 * 1024 / sizeof(uint32_t)]; // used to trigger flush of PSRAM cache ( https://www.esp32.com/viewtopic.php?t=35063 )
} dataEntryEnd_t;

typedef struct
{
    uint32_t id;
    dataEntry_t* start;
    dataEntry_t* first;
    dataEntry_t* last;
    dataEntry_t* end;
} dataEntryHeader_t;

#pragma pack()

class PSRamSensorBuffer {
public:
    PSRamSensorBuffer(u8_t* buffer, size_t size, u8_t* cache, size_t cacheSize, bool powerOn);

    void writeValue(uint16_t serial, time_t time, float value);
    bool getEntry(uint16_t serial, time_t time, dataEntry_t*& act);

private:
    int toIndex(const dataEntry_t* act);
    void debugPrint(const dataEntry_t* act);

public:
    dataEntryHeader_t* _header;
    u8_t* _cache;
    size_t _cacheSize;
};

////////////////////////
#define ENTRY_TO_STRING_SIZE 15
#define RAMDISK_HEADER_ID 0x12345678

class RamDiskClass : public IDataStoreDevice {
public:
    RamDiskClass();
    ~RamDiskClass()
    {
        delete _psRamSensorBuffer;
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
    PSRamSensorBuffer* _psRamSensorBuffer;
    std::mutex _mutex;

private:
    static u8_t* _ramDisk;
    static size_t _ramDiskSize;
    static u8_t* _cache;
    static size_t _cacheSize;
};
extern RamDiskClass* pRamDisk;
