// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "MqttHandleHass.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "defaults.h"

MqttHandleHassClass MqttHandleHass;

void MqttHandleHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleHassClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
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
#if 0
    if (!Configuration.get().Mqtt.Hass.Enabled) {
        return;
    }

    if (!MqttSettings.getConnected()) {
        return;
    }
    if (_macAddr.isEmpty()) {
        uint8_t* bssid = WiFi.BSSID();
        char mac[13];
        sprintf(mac, "%02X%02X%02X%02X%02X%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        _macAddr = mac;
    }

    const CONFIG_T& config = Configuration.get();
    for (uint8_t i = 0; i < Datastore.getSensorCnt(); i++) {
        if (!config.DS18B20.Sensors[i].Connected) {
            continue;
        }
        publishSensor(config.DS18B20.Sensors[i]);
    }
#endif
}

#if 0
void MqttHandleHassClass::publishSensor(const DS18B20SENSOR_CONFIG_T& sensorConfig)
{
    const CONFIG_T& config = Configuration.get();

    String configTopic = "sensor/logger_" + _macAddr + "/" + String(sensorConfig.Serial, 16) + "_Temperature/config";

    DynamicJsonDocument root(1024);
    root["name"] = sensorConfig.Name;
    root["stat_t"] = MqttSettings.getPrefix() + _macAddr + "/" + String(sensorConfig.Serial, 16);
    root["uniq_id"] = _macAddr + "_" + String(sensorConfig.Serial, 16) + "_temperature";
    root["unit_of_meas"] = config.DS18B20.Fahrenheit ? "�F" : "�C";

    if (config.Mqtt_Hass_Expire) {
        root["exp_aft"] = 1.5 * config.DS18B20.PollInterval;
    }
    root["dev_cla"] = "temperature";
    root["stat_cla"] = "measurement";

    String buffer;
    serializeJson(root, buffer);

    publish(configTopic, buffer);
}
#endif
void MqttHandleHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic.c_str(), payload.c_str(), Configuration.get().Mqtt.Hass.Retain);
}