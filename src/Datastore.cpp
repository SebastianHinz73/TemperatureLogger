// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Datastore.h"
#include "Configuration.h"
#include "DS18B20List.h"
#include "MessageOutput.h"

DatastoreClass Datastore;

DatastoreClass::DatastoreClass()
    : _loopTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&DatastoreClass::loop, this))
{
}

void DatastoreClass::init(Scheduler& scheduler, IDataStoreDevice* device)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void DatastoreClass::loop()
{
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
    _list.push_back(std::make_unique<dataSensor>(serial, mDay));
}

void DatastoreClass::addValue(uint16_t serial, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (const auto& entry : _list) {
        if (entry->_serial != serial) {
            continue;
        }

        struct tm timeinfo;
        entry->_timeValid = getLocalTime(&timeinfo, 5);

        // save one value if time is not valid
        if (!entry->_timeValid) {
            MessageOutput.printf("Datastore: getLocalTime failed.\r\n");
            entry->_actValueChanged = entry->_valueTimeNotValid != value;
            entry->_valueTimeNotValid = value;
            break;
        }

        bool bNewDay = false;
        if (timeinfo.tm_mday != entry->_mDay) {
            bNewDay = true;
            entry->_mDay = timeinfo.tm_mday;
        }

        bool bShift = false;
        entry->_actValueChanged = entry->_values.size() == 0 ? true : (value != entry->_values[0]);

        // 0      1
        // 17.8   17.8
        // => If new value also 17.8 (and if no new day) update timestamp from 0
        if (entry->_values.size() > 1) {
            if ((value == entry->_values[0]) && (value == entry->_values[1])) {
                // no new day: if two values equal -> update timestamp
                if (!bNewDay) {
                    bShift = true;
                }
                // new day: if three value equal -> only update timestamp
                else if ((entry->_values.size() > 2) && (value == entry->_values[2])) {
                    bShift = true;
                }
            }
        }
        if (bShift) {
            entry->_values.shift();
            entry->_times.shift();
        }

        // 0      1
        // 17.2   17.8
        // => insert all values
        time_t now;
        time(&now);
        entry->_values.unshift(value);
        entry->_times.unshift(now);

        // PRINT_VALUES(entry->_times, entry->_values);

        if (!bShift && entry->_values.size() > 1) {
            _device->writeValue(serial, entry->_times[1], entry->_values[1]);
        }
        break;
    }
}

bool DatastoreClass::getTemperature(uint16_t serial, uint32_t& time, float& value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& entry : _list) {
        if (entry->_serial == serial) {
            if (entry->_times.size() > 0) {
                time = entry->_times.first();
                value = entry->_values.first();
                return true;
            }
            if (!entry->_timeValid) {
                time = 0;
                value = entry->_valueTimeNotValid;
                return true;
            }
        }
    }
    return false;
}

bool DatastoreClass::valueChanged(uint16_t serial)
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& entry : _list) {
        if (entry->_serial == serial) {
            bool rc = entry->_actValueChanged;
            entry->_actValueChanged = false;
            return rc;
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

    return _device->getFileSize(serial, timeinfo, size);
}

bool DatastoreClass::getTemperatureFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller)
{
    std::lock_guard<std::mutex> lock(_mutex);

    return _device->getFile(serial, timeinfo, responseFiller);
}
