// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <CircularBuffer.h>
#include <TaskSchedulerDeclarations.h>
#include <TimeoutHelper.h>
#include <memory>
#include <mutex>
#include <vector>

#define LAST_DATA_ENTRIES_PER_SENSOR 10

class dataSensor {
public:
    dataSensor(uint16_t serial, uint8_t mDay)
        : _serial(serial)
        , _mDay(mDay)
    {
    }
    uint16_t _serial;
    CircularBuffer<time_t, LAST_DATA_ENTRIES_PER_SENSOR> _times; // seconds since 1970
    CircularBuffer<float, LAST_DATA_ENTRIES_PER_SENSOR> _values;
    bool _timeValid; // board time is synchronized
    float _valueTimeNotValid; // save actual value if time is not synchronized
    bool _actValueChanged; // value changed between two addValue
    uint8_t _mDay; // day of month, used to detect day change -> insert extra 'end of day' value
};

class DatastoreClass {
public:
    void init();

    void loop();

    void addSensor(uint16_t serial);
    void addValue(uint16_t serial, float value);

    bool getTemperature(uint16_t serial, uint32_t& time, float& value);
    bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    bool getTemperatureFile(uint16_t serial, const tm& timeinfo, char* buffer, size_t& size);
    bool valueChanged(uint16_t serial);

private:
    Task _loopTask;
    std::mutex _mutex;

    std::vector<std::unique_ptr<dataSensor>> _list;
};

extern DatastoreClass Datastore;