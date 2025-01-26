// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Configuration.h"
#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>

// mqtt discovery device classes
enum DeviceClassType {
    DEVICE_CLS_NONE = 0,
    DEVICE_CLS_CURRENT,
    DEVICE_CLS_ENERGY,
    DEVICE_CLS_PWR,
    DEVICE_CLS_VOLTAGE,
    DEVICE_CLS_FREQ,
    DEVICE_CLS_POWER_FACTOR,
    DEVICE_CLS_REACTIVE_POWER,
    DEVICE_CLS_CONNECTIVITY,
    DEVICE_CLS_DURATION,
    DEVICE_CLS_SIGNAL_STRENGTH,
    DEVICE_CLS_TEMPERATURE,
    DEVICE_CLS_RESTART
};
const char* const deviceClass_name[] = { 0, "current", "energy", "power", "voltage", "frequency", "power_factor", "reactive_power", "connectivity", "duration", "signal_strength", "temperature", "restart" };

enum StateClassType {
    STATE_CLS_NONE = 0,
    STATE_CLS_MEASUREMENT,
    STATE_CLS_TOTAL_INCREASING
};
const char* const stateClass_name[] = { 0, "measurement", "total_increasing" };

enum CategoryType {
    CATEGORY_NONE = 0,
    CATEGORY_CONFIG,
    CATEGORY_DIAGNOSTIC
};
const char* const category_name[] = { 0, "config", "diagnostic" };

class MqttHandleHassClass {
public:
    MqttHandleHassClass();
    void init(Scheduler& scheduler);
    void publishConfig();
    void forceUpdate();

private:
    void loop();
    static void publish(const String& subtopic, const String& payload);
    static void publish(const String& subtopic, const JsonDocument& doc);

    static void addCommonMetadata(JsonDocument& doc, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const StateClassType state_class, const CategoryType category);

    // Sensor
    static void publishSensor(JsonDocument& doc, const String& root_device, const String& unique_id_prefix, const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const StateClassType state_class, const CategoryType category);
    static void publishDtuSensor(const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const StateClassType state_class, const CategoryType category);
    static void publishDS18B20Sensor(const String& name, const String& serial, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const StateClassType state_class, const CategoryType category);

    static void createDtuInfo(JsonDocument& doc);

    static void createDeviceInfo(JsonDocument& doc, const String& name, const String& identifiers, const String& configuration_url, const String& manufacturer, const String& model, const String& sw_version, const String& via_device = "");

    static String getDtuUniqueId();
    static String getDtuUrl();

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
};

extern MqttHandleHassClass MqttHandleHass;