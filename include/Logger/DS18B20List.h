// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "DS18B20Pin.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <TaskSchedulerDeclarations.h>
#include <cstdint>
#include <memory>
#include <vector>

#define TEMP_LOGGER_VERSION "2.1.1"
#define TIME_SCAN_NEW_SENSORS 60

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

    std::vector<std::unique_ptr<DS18B20Pin>> _pins;
};

extern DS18B20ListClass DS18B20List;
