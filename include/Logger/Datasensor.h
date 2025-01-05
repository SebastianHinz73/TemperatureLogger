// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"

class Datasensor {
public:
    Datasensor(uint16_t serial, uint8_t mDay);

    inline uint16_t Serial()
    {
        return _serial;
    }
    void addValue(IDataStoreDevice* device, float value);
    void getTemperature(uint32_t& time, float& value);
    bool valueChanged() { return _actValueChanged; }

private:
    uint16_t _serial;
    bool _timeValid; // board time is synchronized
    bool _actValueChanged; // value changed between two addValue
    uint8_t _mDay; // day of month, used to detect day change -> insert extra 'end of day' value

    time_t _firstTime;
    time_t _lastTime;
    float _value;
};
