// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <DallasTemperature.h>
#include <OneWire.h>
#include <TaskSchedulerDeclarations.h>
#include <cstdint>
#include <memory>
#include <vector>

#define TEMP_LOGGER_VERSION "2.0.0Beta"

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

class DS18B20ListClass {
public:
    DS18B20ListClass();
    void init(Scheduler& scheduler);

private:
    void loop();
    void scanSensors();
    void readTemperature();

private:
    Task _loopTask;

    uint32_t _lastScan;
    uint32_t _lastTemperatureUpdate;

    OneWire _ow;
    DallasTemperature _sensors;

    std::vector<std::unique_ptr<DS18B20>> _list;
};

extern DS18B20ListClass DS18B20List;
