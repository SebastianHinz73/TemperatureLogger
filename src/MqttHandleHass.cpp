// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "MqttHandleHass.h"
#include "Datastore.h"
#include "Logger/DS18B20List.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "__compiled_constants.h"
#include "defaults.h"

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

    // publish DTU sensors
    publishDtuSensor("IP", "dtu/ip", "", "mdi:network-outline", DEVICE_CLS_NONE, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("WiFi Signal", "dtu/rssi", "dBm", "", DEVICE_CLS_SIGNAL_STRENGTH, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Uptime", "dtu/uptime", "s", "", DEVICE_CLS_DURATION, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Temperature", "dtu/temperature", "°C", "", DEVICE_CLS_TEMPERATURE, STATE_CLS_MEASUREMENT, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Heap Size", "dtu/heap/size", "Bytes", "mdi:memory", DEVICE_CLS_NONE, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Heap Free", "dtu/heap/free", "Bytes", "mdi:memory", DEVICE_CLS_NONE, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Largest Free Heap Block", "dtu/heap/maxalloc", "Bytes", "mdi:memory", DEVICE_CLS_NONE, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Lifetime Minimum Free Heap", "dtu/heap/minfree", "Bytes", "mdi:memory", DEVICE_CLS_NONE, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Ramdrive oldest entry", "dtu/ramdrive/oldest_entry", "", "mdi:calendar-clock", DEVICE_CLS_NONE, STATE_CLS_NONE, CATEGORY_DIAGNOSTIC);

    for (uint8_t i = 0; i < Configuration.getConfiguredSensorCnt(); i++) {
        publishDS18B20Sensor(config.DS18B20.Sensors[i].Name, String(config.DS18B20.Sensors[i].Serial, 16), config.DS18B20.Fahrenheit ? "°F" : "°C", "", DEVICE_CLS_TEMPERATURE, STATE_CLS_MEASUREMENT, CATEGORY_NONE);
    }
}

void MqttHandleHassClass::createDtuInfo(JsonDocument& root)
{
    createDeviceInfo(
        root,
        NetworkSettings.getHostname(),
        getDtuUniqueId(),
        getDtuUrl(),
        "Logger",
        "Logger",
        __COMPILED_GIT_HASH__);
}

void MqttHandleHassClass::createDeviceInfo(
    JsonDocument& root,
    const String& name, const String& identifiers, const String& configuration_url,
    const String& manufacturer, const String& model, const String& sw_version,
    const String& via_device)
{
    auto object = root["dev"].to<JsonObject>();

    object["name"] = name;
    object["ids"] = identifiers;
    object["cu"] = configuration_url;
    object["mf"] = manufacturer;
    // object["mdl"] = model;
    object["sw"] = sw_version;

    if (via_device != "") {
        object["via_device"] = via_device;
    }
}

String MqttHandleHassClass::getDtuUniqueId()
{
    return NetworkSettings.getHostname() + "_" + Utils::getChipId();
}

String MqttHandleHassClass::getDtuUrl()
{
    return String("http://") + NetworkSettings.localIP().toString();
}

void MqttHandleHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic, payload, Configuration.get().Mqtt.Hass.Retain);
    yield();
}

void MqttHandleHassClass::publish(const String& subtopic, const JsonDocument& doc)
{
    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return;
    }
    String buffer;
    serializeJson(doc, buffer);
    publish(subtopic, buffer);
}

void MqttHandleHassClass::addCommonMetadata(
    JsonDocument& doc,
    const String& unit_of_measure, const String& icon,
    const DeviceClassType device_class, const StateClassType state_class, const CategoryType category)
{
    if (unit_of_measure != "") {
        doc["unit_of_meas"] = unit_of_measure;
    }
    if (icon != "") {
        doc["ic"] = icon;
    }
    if (device_class != DEVICE_CLS_NONE) {
        doc["dev_cla"] = deviceClass_name[device_class];
    }
    if (state_class != STATE_CLS_NONE) {
        doc["stat_cla"] = stateClass_name[state_class];
        ;
    }
    if (category != CATEGORY_NONE) {
        doc["ent_cat"] = category_name[category];
    }
}

void MqttHandleHassClass::publishSensor(
    JsonDocument& doc,
    const String& root_device, const String& unique_id_prefix, const String& name, const String& state_topic,
    const String& unit_of_measure, const String& icon,
    const DeviceClassType device_class, const StateClassType state_class, const CategoryType category)
{
    String sensor_id = name;
    sensor_id.toLowerCase();
    sensor_id.replace(" ", "_");

    if (!doc["name"].is<String>()) {
        doc["name"] = name;
    }
    doc["uniq_id"] = unique_id_prefix + "_" + sensor_id;
    doc["stat_t"] = MqttSettings.getPrefix() + state_topic;

    addCommonMetadata(doc, unit_of_measure, icon, device_class, state_class, category);

    const CONFIG_T& config = Configuration.get();
    doc["avty_t"] = MqttSettings.getPrefix() + config.Mqtt.Lwt.Topic;
    doc["pl_avail"] = config.Mqtt.Lwt.Value_Online;
    doc["pl_not_avail"] = config.Mqtt.Lwt.Value_Offline;

    const String configTopic = "sensor/" + root_device + "/" + sensor_id + "/config";
    publish(configTopic, doc);
}

void MqttHandleHassClass::publishDtuSensor(
    const String& name, const String& state_topic,
    const String& unit_of_measure, const String& icon,
    const DeviceClassType device_class, const StateClassType state_class, const CategoryType category)
{
    const String dtuId = getDtuUniqueId();

    JsonDocument root;
    createDtuInfo(root);
    publishSensor(root, dtuId, dtuId, name, state_topic, unit_of_measure, icon, device_class, state_class, category);
}

void MqttHandleHassClass::publishDS18B20Sensor(
    const String& name, const String& serial,
    const String& unit_of_measure, const String& icon,
    const DeviceClassType device_class, const StateClassType state_class, const CategoryType category)
{
    const String dtuId = getDtuUniqueId();

    JsonDocument root;
    createDtuInfo(root);

    if (Configuration.get().Mqtt.Hass.Expire) {
        root["exp_aft"] = Configuration.get().Mqtt.PublishInterval * 2;
    }
    root["name"] = name;
    publishSensor(root, dtuId, dtuId, "ds18b20_" + serial, "ds18b20/" + serial, unit_of_measure, icon, device_class, state_class, category);
}
