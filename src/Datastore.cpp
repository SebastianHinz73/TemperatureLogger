// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Datastore.h"
#include "Configuration.h"
#include "Logger/DS18B20List.h"
#include "MessageOutput.h"

DatastoreClass Datastore;

void DatastoreClass::init(IDataStoreDevice* device)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _device = device;
}

void DatastoreClass::addSensor(uint16_t serial)
{
    std::lock_guard<std::mutex> lock(_mutex);

    uint8_t mDay = 0;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
        // In most cases there is no valid time if sensor is added
        mDay = timeinfo.tm_mday;
    }
    _list.push_back(std::make_unique<Datasensor>(serial, mDay));
}

bool DatastoreClass::validSensor(uint16_t serial)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto& entry : _list) {
        if (entry->Serial() == serial) {
            return true;
        }
    }
    return false;
}

void DatastoreClass::addValue(uint16_t serial, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_device == nullptr)
        return;

    for (const auto& entry : _list) {
        if (entry->Serial() != serial) {
            continue;
        }
        entry->addValue(_device, value);
        break;
    }
}

bool DatastoreClass::getTemperature(uint16_t serial, uint32_t& time, float& value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto& entry : _list) {
        if (entry->Serial() == serial) {
            entry->getTemperature(time, value);
            return true;
        }
    }

    return false;
}

bool DatastoreClass::valueChanged(uint16_t serial)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto& entry : _list) {
        if (entry->Serial() == serial) {
            return entry->valueChanged();
        }
    }
    return false;
}

bool DatastoreClass::getTmTime(struct tm* info, time_t time, uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) <= ms) {
        localtime_r(&time, info);
        if (info->tm_year > (2016 - 1900)) {
            return true;
        }
        delay(10);
    }
    return false;
}

bool DatastoreClass::getFileSize(uint16_t serial, const tm& timeinfo, size_t& size)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_device == nullptr)
        return false;

    return _device->getFileSize(serial, timeinfo, size);
}

bool DatastoreClass::getTemperatureFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_device == nullptr)
        return false;

    return _device->getFile(serial, timeinfo, responseFiller);
}
