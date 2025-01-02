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
        MessageOutput.print("No configured DS18B20 pins found");
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
    if (millis() - _lastScan > TIME_SCAN_NEW_SENSORS * 1000) {
        scanSensors();
        _lastScan = millis();
    }

    auto config = Configuration.get();
    if (millis() - _lastTemperatureUpdate > config.DS18B20.PollInterval * 1000) {
        readTemperature();
        _lastTemperatureUpdate = millis();
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
