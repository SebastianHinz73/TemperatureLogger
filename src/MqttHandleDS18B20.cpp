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
    _loopTask.enable();
}

void MqttHandleDS18B20Class::loop()
{
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

    if (millis() - _lastPublish > (config.Mqtt.PublishInterval * 1000)) {
        MqttSettings.publish("logger/uptime", String(millis() / 1000));
        MqttSettings.publish("logger/ip", NetworkSettings.localIP().toString());
        MqttSettings.publish("logger/hostname", NetworkSettings.getHostname());
        if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
            MqttSettings.publish("logger/rssi", String(WiFi.RSSI()));
            MqttSettings.publish("logger/bssid", String(WiFi.BSSIDstr()));
        }

        for (uint8_t i = 0; i < Configuration.getConfiguredSensorCnt(); i++) {
            if (!config.DS18B20.Sensors[i].Connected) {
                continue;
            }
            uint32_t time = 0;
            float value = 0;
            Datastore.getTemperature(config.DS18B20.Sensors[i].Serial, time, value);

            MqttSettings.publish(_macAddr + "/" + String(config.DS18B20.Sensors[i].Serial, 16), String(value));
            MessageOutput.printf("MQTT %s -> %s\r\n", String(config.DS18B20.Sensors[i].Serial, 16).c_str(), String(value).c_str());
        }

        _lastPublish = millis();
    }
}
