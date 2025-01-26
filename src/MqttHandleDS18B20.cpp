// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */
#include "MqttHandleDS18B20.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"

MqttHandleDS18B20Class MqttHandleDS18B20;

MqttHandleDS18B20Class::MqttHandleDS18B20Class()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleDS18B20Class::loop, this))
{
}

void MqttHandleDS18B20Class::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleDS18B20Class::loop()
{
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected()) {
        _loopTask.forceNextIteration();
        return;
    }

    const CONFIG_T& config = Configuration.get();

    for (uint8_t i = 0; i < Configuration.getConfiguredSensorCnt(); i++) {
        uint32_t time = 0;
        float value = 0;
        if (Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value)) {
            MqttSettings.publish("ds18b20/" + String(config.DS18B20.Sensors[i].Serial, 16), String(value));
        }
    }
}
