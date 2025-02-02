// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */
#include "Logger/Datasensor.h"
#include "MessageOutput.h"
#include <limits.h>

Datasensor::Datasensor(uint16_t serial, uint8_t mDay)
    : _serial(serial)
    , _mDay(mDay)
{
    _firstTime = 0;
    _lastTime = 0;
    _value = std::numeric_limits<float>::min();
}

void Datasensor::addValue(IDataStoreDevice* device, float value)
{
    struct tm timeinfo;
    _timeValid = getLocalTime(&timeinfo, 5);

    _actValueChanged = value != _value;

    // save one value if time is not valid
    if (!_timeValid) {
        MessageOutput.printf("Datasensor: getLocalTime failed.\r\n");
        _value = value;
        return;
    }

    bool bNewDay = timeinfo.tm_mday != _mDay;
    _mDay = timeinfo.tm_mday;

    time_t now;
    time(&now);

    if (_actValueChanged || bNewDay) {
        // write last value
        if (_firstTime != _lastTime && _lastTime > 0) {
            device->writeValue(_serial, _lastTime, _value);
        }
        device->writeValue(_serial, now, value);

        _firstTime = now;
        _lastTime = now;
        _value = value;
    } else {
        _lastTime = now;
    }
}

bool Datasensor::getTemperature(uint32_t& time, float& value)
{
    time = _timeValid ? _lastTime : 0;
    value = _value;
    return _value != std::numeric_limits<float>::min();
}
