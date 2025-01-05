// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Logger/Datasensor.h"
#include "Logger/IDataStoreDevice.h"
#include <TaskSchedulerDeclarations.h>
#include <TimeoutHelper.h>
#include <memory>
#include <mutex>
#include <vector>

class DatastoreClass {
public:
    DatastoreClass()
        : _device(nullptr)
    {
    }
    void init(IDataStoreDevice* device);

    void addSensor(uint16_t serial);
    bool validSensor(uint16_t serial);
    void addValue(uint16_t serial, float value);

    static bool getTmTime(struct tm* info, time_t time, uint32_t ms);

    bool getTemperature(uint16_t serial, uint32_t& time, float& value);
    bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    bool getTemperatureFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller);
    bool valueChanged(uint16_t serial);

private:
    std::mutex _mutex;
    IDataStoreDevice* _device;

    std::vector<std::unique_ptr<Datasensor>> _list;
};

extern DatastoreClass Datastore;