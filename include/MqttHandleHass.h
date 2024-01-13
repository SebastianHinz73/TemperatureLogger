// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Configuration.h"
#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>

class MqttHandleHassClass {
public:
    void init(Scheduler& scheduler);
    void publishConfig();
    void forceUpdate();

private:
    void loop();
    void publishSensor(const DS18B20SENSOR_CONFIG_T& sensorConfig);
    void publish(const String& subtopic, const String& payload);

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
    String _macAddr;
};

extern MqttHandleHassClass MqttHandleHass;