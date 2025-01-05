// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>
#include <TaskSchedulerDeclarations.h>
#include <cstdint>
#include <memory>
#include <vector>

class DS18B20 {
public:
    DS18B20(const DeviceAddress& addr, uint16_t serial)
    {
        memcpy(&_addr, addr, sizeof(_addr));
        _serial = serial;
    }

public:
    DeviceAddress _addr;
    uint16_t _serial;
};

////////////////////////

class DS18B20Pin {
public:
    DS18B20Pin(int pin);
    void scanSensors();
    void readTemperature();

private:
    int _pin;
    OneWire _ow;
    DallasTemperature _sensors;

    std::vector<std::unique_ptr<DS18B20>> _list;
};
