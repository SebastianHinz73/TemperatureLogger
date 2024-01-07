// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <WString.h>
#include <cstdint>
#include <TaskSchedulerDeclarations.h>

class MqttHandleDS18B20Class {
public:
    void init(Scheduler& scheduler);
    void loop();

private:
    Task _loopTask;

    uint32_t _lastPublish = 0;
    String _macAddr;
};

extern MqttHandleDS18B20Class MqttHandleDS18B20;