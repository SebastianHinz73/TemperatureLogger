// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "MqttHandleDtu.h"
#include "Configuration.h"
#include "Logger/RamDrive.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <CpuTemperature.h>
#include <Utils.h>

MqttHandleDtuClass MqttHandleDtu;

MqttHandleDtuClass::MqttHandleDtuClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleDtuClass::loop, this))
{
}

void MqttHandleDtuClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleDtuClass::loop()
{
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected()) {
        _loopTask.forceNextIteration();
        return;
    }

    char buf[20] = {0};
    time_t t = pRamDrive->getOldestTime();
    if(t != 0) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    }

    String id = String(Utils::getChipId());

    MqttSettings.publish(id + "/uptime", String(esp_timer_get_time() / 1000000));
    MqttSettings.publish(id + "/ip", NetworkSettings.localIP().toString());
    MqttSettings.publish(id + "/hostname", NetworkSettings.getHostname());
    MqttSettings.publish(id + "/heap/size", String(ESP.getHeapSize()));
    MqttSettings.publish(id + "/heap/free", String(ESP.getFreeHeap()));
    MqttSettings.publish(id + "/heap/minfree", String(ESP.getMinFreeHeap()));
    MqttSettings.publish(id + "/heap/maxalloc", String(ESP.getMaxAllocHeap()));
    MqttSettings.publish(id + "/ramdrive/oldest_entry", String(buf));
    if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
        MqttSettings.publish(id + "/rssi", String(WiFi.RSSI()));
        MqttSettings.publish(id + "/bssid", WiFi.BSSIDstr());
    }

    float temperature = CpuTemperature.read();
    if (!std::isnan(temperature)) {
        MqttSettings.publish(id + "/temperature", String(temperature));
    }
}
