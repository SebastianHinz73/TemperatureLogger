// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "MqttHandleHass.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "__compiled_constants.h"
#include "defaults.h"
#include "Logger/DS18B20List.h"

MqttHandleHassClass MqttHandleHass;

MqttHandleHassClass::MqttHandleHassClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleHassClass::loop, this))
{
}

void MqttHandleHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void MqttHandleHassClass::loop()
{
    if (_updateForced) {
        publishConfig();
        _updateForced = false;
    }

    if (MqttSettings.getConnected() && !_wasConnected) {
        // Connection established
        _wasConnected = true;
        publishConfig();
    } else if (!MqttSettings.getConnected() && _wasConnected) {
        // Connection lost
        _wasConnected = false;
    }
}

void MqttHandleHassClass::forceUpdate()
{
    _updateForced = true;
}

void MqttHandleHassClass::publishConfig()
{
    if (!Configuration.get().Mqtt.Hass.Enabled) {
        return;
    }

    if (!MqttSettings.getConnected()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();
    for (uint8_t i = 0; i < Configuration.getConfiguredSensorCnt(); i++) {
        publishSensor(config.DS18B20.Sensors[i]);
    }
}

void MqttHandleHassClass::publishSensor(const DS18B20SENSOR_CONFIG_T& sensorConfig)
{
    const CONFIG_T& config = Configuration.get();

    String configTopic = "sensor/logger_" + _macAddr + "/DS18B20_" + String(sensorConfig.Serial, 16) + "/config";

    JsonDocument root;
    root["name"] = sensorConfig.Name;
    root["stat_t"] = MqttSettings.getPrefix() + _macAddr + "/" + String(sensorConfig.Serial, 16);
    root["uniq_id"] = _macAddr + "_" + String(sensorConfig.Serial, 16) + "_temperature";
    root["unit_of_meas"] = config.DS18B20.Fahrenheit ? "°F" : "°C";

    auto object = root["dev"].to<JsonObject>();

    object["name"] = NetworkSettings.getHostname();
    object["ids"] = _macAddr;
    object["cu"] = String("http://") + NetworkSettings.localIP().toString();
    object["mf"] = "TemperatureLogger";
    object["mdl"] = "";
    object["sw"] = TEMP_LOGGER_VERSION;

    if (config.Mqtt.Hass.Expire) {
        root["exp_aft"] = 1.5 * config.DS18B20.PollInterval;
    }
    root["dev_cla"] = "temperature";
    root["stat_cla"] = "measurement";

    String buffer;
    serializeJson(root, buffer);

    publish(configTopic, buffer);
}

void MqttHandleHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic, payload, Configuration.get().Mqtt.Hass.Retain);
    yield();
}