// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "Logger/DS18B20List.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <Configuration.h>

DS18B20ListClass DS18B20List;

DS18B20ListClass::DS18B20ListClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&DS18B20ListClass::loop, this))
{
}

void DS18B20ListClass::init(Scheduler& scheduler)
{
    for (const auto& pin : PinMapping.getDS18B20List()) {
        if (pin >= 0) {
            _pins.push_back(std::make_unique<DS18B20Pin>(pin));
        }
    }
    if (_pins.size() == 0) {
        MessageOutput.print("No configured DS18B20 pins found ");
        return;
    }

    scheduler.addTask(_loopTask);
    _loopTask.enable();

    scanSensors();
    readTemperature();

    _lastScan = millis();
    _lastTemperatureUpdate = millis();
}

void DS18B20ListClass::loop()
{
    unsigned long now = millis();
    if (now - _lastScan > TIME_SCAN_NEW_SENSORS * 1000) {
        scanSensors();
        _lastScan = now;
    }

    // only 2 seconds at startup
    uint32_t interval = now < 15000 ? 2000 : Configuration.get().DS18B20.PollInterval * 1000;

    if (now - _lastTemperatureUpdate > interval) {
        readTemperature();
        _lastTemperatureUpdate = now;
    }
}

void DS18B20ListClass::scanSensors()
{
    for (const auto& pin : _pins) {
        pin->scanSensors();
    }
}

void DS18B20ListClass::readTemperature()
{
    for (const auto& pin : _pins) {
        pin->readTemperature();
    }
}
