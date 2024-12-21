// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"
#include <Arduino.h>
#include <memory>
#include <mutex>
#include <vector>

typedef struct
{
    int serial;
    time_t time;
    float value;
} dataEntry_t;

class MyCircularBuffer {
public:
    MyCircularBuffer(int elements);
    ~MyCircularBuffer();

    void writeValue(uint16_t serial, time_t time, float value);
    int getEntryCnt(uint16_t serial, time_t time);
    bool getEntry(uint16_t serial, time_t time, dataEntry_t*& act);

private:
    int toIndex(const dataEntry_t* act);
    void debugPrint(const dataEntry_t* act);

private:
    dataEntry_t* _start;
    dataEntry_t* _first;
    dataEntry_t* _last;
    dataEntry_t* _end;
};

////////////////////////
#define ENTRY_TO_STRING_SIZE 15

class RamDiskClass : public IDataStoreDevice {
public:
    RamDiskClass()
    {
        _myBuffer = std::make_unique<MyCircularBuffer>(5000); // 110 kb free
        //_myBuffer = std::make_unique<MyCircularBuffer>(100); // 154 kb free
    }
    void init();

    // IDataStoreDevice
    virtual void writeValue(uint16_t serial, time_t time, float value);
    virtual bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    virtual bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller);

private:
    time_t getStartOfDay(const tm& timeinfo);

private:
    std::unique_ptr<MyCircularBuffer> _myBuffer;

    std::mutex _mutex;
};
extern RamDiskClass* pRamDisk;
