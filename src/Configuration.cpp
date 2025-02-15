// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "Configuration.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "defaults.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <nvs_flash.h>

CONFIG_T config;

static std::condition_variable sWriterCv;
static std::mutex sWriterMutex;
static unsigned sWriterCount = 0;

void ConfigurationClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&ConfigurationClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    memset(&config, 0x0, sizeof(config));
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg.SaveCount++;

    JsonDocument doc;

    JsonObject cfg = doc["cfg"].to<JsonObject>();
    cfg["version"] = config.Cfg.Version;
    cfg["save_count"] = config.Cfg.SaveCount;

    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["ssid"] = config.WiFi.Ssid;
    wifi["password"] = config.WiFi.Password;
    wifi["ip"] = IPAddress(config.WiFi.Ip).toString();
    wifi["netmask"] = IPAddress(config.WiFi.Netmask).toString();
    wifi["gateway"] = IPAddress(config.WiFi.Gateway).toString();
    wifi["dns1"] = IPAddress(config.WiFi.Dns1).toString();
    wifi["dns2"] = IPAddress(config.WiFi.Dns2).toString();
    wifi["dhcp"] = config.WiFi.Dhcp;
    wifi["hostname"] = config.WiFi.Hostname;
    wifi["aptimeout"] = config.WiFi.ApTimeout;

    JsonObject mdns = doc["mdns"].to<JsonObject>();
    mdns["enabled"] = config.Mdns.Enabled;

    JsonObject ntp = doc["ntp"].to<JsonObject>();
    ntp["server"] = config.Ntp.Server;
    ntp["timezone"] = config.Ntp.Timezone;
    ntp["timezone_descr"] = config.Ntp.TimezoneDescr;
    ntp["latitude"] = config.Ntp.Latitude;
    ntp["longitude"] = config.Ntp.Longitude;
    ntp["sunsettype"] = config.Ntp.SunsetType;

    JsonObject mqtt = doc["mqtt"].to<JsonObject>();
    mqtt["enabled"] = config.Mqtt.Enabled;
    mqtt["hostname"] = config.Mqtt.Hostname;
    mqtt["port"] = config.Mqtt.Port;
    mqtt["clientid"] = config.Mqtt.ClientId;
    mqtt["username"] = config.Mqtt.Username;
    mqtt["password"] = config.Mqtt.Password;
    mqtt["topic"] = config.Mqtt.Topic;
    mqtt["retain"] = config.Mqtt.Retain;
    mqtt["publish_interval"] = config.Mqtt.PublishInterval;
    mqtt["clean_session"] = config.Mqtt.CleanSession;

    JsonObject templogger = doc["templogger"].to<JsonObject>();
    templogger["pollinterval"] = config.DS18B20.PollInterval;
    templogger["fahrenheit"] = config.DS18B20.Fahrenheit;

    JsonArray sensors = templogger["sensors"].to<JsonArray>();
    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        JsonObject sensor = sensors[i].to<JsonObject>();
        sensor["serial"] = config.DS18B20.Sensors[i].Serial;
        sensor["name"] = config.DS18B20.Sensors[i].Name;
    }

    JsonObject mqtt_lwt = mqtt["lwt"].to<JsonObject>();
    mqtt_lwt["topic"] = config.Mqtt.Lwt.Topic;
    mqtt_lwt["value_online"] = config.Mqtt.Lwt.Value_Online;
    mqtt_lwt["value_offline"] = config.Mqtt.Lwt.Value_Offline;
    mqtt_lwt["qos"] = config.Mqtt.Lwt.Qos;

    JsonObject mqtt_tls = mqtt["tls"].to<JsonObject>();
    mqtt_tls["enabled"] = config.Mqtt.Tls.Enabled;
    mqtt_tls["root_ca_cert"] = config.Mqtt.Tls.RootCaCert;
    mqtt_tls["certlogin"] = config.Mqtt.Tls.CertLogin;
    mqtt_tls["client_cert"] = config.Mqtt.Tls.ClientCert;
    mqtt_tls["client_key"] = config.Mqtt.Tls.ClientKey;

    JsonObject mqtt_hass = mqtt["hass"].to<JsonObject>();
    mqtt_hass["enabled"] = config.Mqtt.Hass.Enabled;
    mqtt_hass["retain"] = config.Mqtt.Hass.Retain;
    mqtt_hass["topic"] = config.Mqtt.Hass.Topic;
    mqtt_hass["device_name"] = config.Mqtt.Hass.DeviceName;
    mqtt_hass["individual_panels"] = config.Mqtt.Hass.IndividualPanels;
    mqtt_hass["expire"] = config.Mqtt.Hass.Expire;

    JsonObject security = doc["security"].to<JsonObject>();
    security["password"] = config.Security.Password;
    security["allow_readonly"] = config.Security.AllowReadonly;

    JsonObject device = doc["device"].to<JsonObject>();
    device["pinmapping"] = config.Dev_PinMapping;

    JsonObject display = device["display"].to<JsonObject>();
    display["powersafe"] = config.Display.PowerSafe;
    display["screensaver"] = config.Display.ScreenSaver;
    display["rotation"] = config.Display.Rotation;
    display["contrast"] = config.Display.Contrast;
    display["locale"] = config.Display.Locale;
    display["diagram_duration"] = config.Display.Diagram.Duration;
    display["diagram_mode"] = config.Display.Diagram.Mode;

    JsonArray leds = device["led"].to<JsonArray>();
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds.add<JsonObject>();
        led["brightness"] = config.Led_Single[i].Brightness;
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return false;
    }

    // Serialize JSON to file
    if (serializeJson(doc, f) == 0) {
        MessageOutput.println("Failed to write file");
        return false;
    }

    f.close();
    return true;
}

bool ConfigurationClass::read()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r", false);
    Utils::skipBom(f);

    JsonDocument doc;

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return false;
    }

    JsonObject cfg = doc["cfg"];
    config.Cfg.Version = cfg["version"] | CONFIG_VERSION;
    config.Cfg.SaveCount = cfg["save_count"] | 0;

    JsonObject wifi = doc["wifi"];
    strlcpy(config.WiFi.Ssid, wifi["ssid"] | WIFI_SSID, sizeof(config.WiFi.Ssid));
    strlcpy(config.WiFi.Password, wifi["password"] | WIFI_PASSWORD, sizeof(config.WiFi.Password));
    strlcpy(config.WiFi.Hostname, wifi["hostname"] | APP_HOSTNAME, sizeof(config.WiFi.Hostname));

    IPAddress wifi_ip;
    wifi_ip.fromString(wifi["ip"] | "");
    config.WiFi.Ip[0] = wifi_ip[0];
    config.WiFi.Ip[1] = wifi_ip[1];
    config.WiFi.Ip[2] = wifi_ip[2];
    config.WiFi.Ip[3] = wifi_ip[3];

    IPAddress wifi_netmask;
    wifi_netmask.fromString(wifi["netmask"] | "");
    config.WiFi.Netmask[0] = wifi_netmask[0];
    config.WiFi.Netmask[1] = wifi_netmask[1];
    config.WiFi.Netmask[2] = wifi_netmask[2];
    config.WiFi.Netmask[3] = wifi_netmask[3];

    IPAddress wifi_gateway;
    wifi_gateway.fromString(wifi["gateway"] | "");
    config.WiFi.Gateway[0] = wifi_gateway[0];
    config.WiFi.Gateway[1] = wifi_gateway[1];
    config.WiFi.Gateway[2] = wifi_gateway[2];
    config.WiFi.Gateway[3] = wifi_gateway[3];

    IPAddress wifi_dns1;
    wifi_dns1.fromString(wifi["dns1"] | "");
    config.WiFi.Dns1[0] = wifi_dns1[0];
    config.WiFi.Dns1[1] = wifi_dns1[1];
    config.WiFi.Dns1[2] = wifi_dns1[2];
    config.WiFi.Dns1[3] = wifi_dns1[3];

    IPAddress wifi_dns2;
    wifi_dns2.fromString(wifi["dns2"] | "");
    config.WiFi.Dns2[0] = wifi_dns2[0];
    config.WiFi.Dns2[1] = wifi_dns2[1];
    config.WiFi.Dns2[2] = wifi_dns2[2];
    config.WiFi.Dns2[3] = wifi_dns2[3];

    config.WiFi.Dhcp = wifi["dhcp"] | WIFI_DHCP;
    config.WiFi.ApTimeout = wifi["aptimeout"] | ACCESS_POINT_TIMEOUT;

    JsonObject mdns = doc["mdns"];
    config.Mdns.Enabled = mdns["enabled"] | MDNS_ENABLED;

    JsonObject ntp = doc["ntp"];
    strlcpy(config.Ntp.Server, ntp["server"] | NTP_SERVER, sizeof(config.Ntp.Server));
    strlcpy(config.Ntp.Timezone, ntp["timezone"] | NTP_TIMEZONE, sizeof(config.Ntp.Timezone));
    strlcpy(config.Ntp.TimezoneDescr, ntp["timezone_descr"] | NTP_TIMEZONEDESCR, sizeof(config.Ntp.TimezoneDescr));
    config.Ntp.Latitude = ntp["latitude"] | NTP_LATITUDE;
    config.Ntp.Longitude = ntp["longitude"] | NTP_LONGITUDE;
    config.Ntp.SunsetType = ntp["sunsettype"] | NTP_SUNSETTYPE;

    JsonObject mqtt = doc["mqtt"];
    config.Mqtt.Enabled = mqtt["enabled"] | MQTT_ENABLED;
    strlcpy(config.Mqtt.Hostname, mqtt["hostname"] | MQTT_HOST, sizeof(config.Mqtt.Hostname));
    config.Mqtt.Port = mqtt["port"] | MQTT_PORT;
    strlcpy(config.Mqtt.ClientId, mqtt["clientid"] | NetworkSettings.getApName().c_str(), sizeof(config.Mqtt.ClientId));
    strlcpy(config.Mqtt.Username, mqtt["username"] | MQTT_USER, sizeof(config.Mqtt.Username));
    strlcpy(config.Mqtt.Password, mqtt["password"] | MQTT_PASSWORD, sizeof(config.Mqtt.Password));
    strlcpy(config.Mqtt.Topic, mqtt["topic"] | MQTT_TOPIC, sizeof(config.Mqtt.Topic));
    config.Mqtt.Retain = mqtt["retain"] | MQTT_RETAIN;
    config.Mqtt.PublishInterval = mqtt["publish_interval"] | MQTT_PUBLISH_INTERVAL;
    config.Mqtt.CleanSession = mqtt["clean_session"] | MQTT_CLEAN_SESSION;

    JsonObject templogger = doc["templogger"];
    config.DS18B20.PollInterval = templogger["pollinterval"] | TEMPLOGGER_DS18B20_POLL_INTERVAL;
    config.DS18B20.Fahrenheit = templogger["fahrenheit"] | TEMPLOGGER_DS18B20_FAHRENHEIT;

    JsonArray sensors = templogger["sensors"];

    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        JsonObject sensor = sensors[i].as<JsonObject>();
        config.DS18B20.Sensors[i].Serial = sensor["serial"] | 0U;
        strlcpy(config.DS18B20.Sensors[i].Name, sensor["name"] | "undefined", sizeof(config.DS18B20.Sensors[i].Name));
    }
    JsonObject mqtt_lwt = mqtt["lwt"];

    String MQTT_LWT_TOPIC = String(Utils::getChipId()) + "/status";
    strlcpy(config.Mqtt.Lwt.Topic, MQTT_LWT_TOPIC.c_str(), sizeof(config.Mqtt.Lwt.Topic));
    strlcpy(config.Mqtt.Lwt.Value_Online, mqtt_lwt["value_online"] | MQTT_LWT_ONLINE, sizeof(config.Mqtt.Lwt.Value_Online));
    strlcpy(config.Mqtt.Lwt.Value_Offline, mqtt_lwt["value_offline"] | MQTT_LWT_OFFLINE, sizeof(config.Mqtt.Lwt.Value_Offline));
    config.Mqtt.Lwt.Qos = mqtt_lwt["qos"] | MQTT_LWT_QOS;

    JsonObject mqtt_tls = mqtt["tls"];
    config.Mqtt.Tls.Enabled = mqtt_tls["enabled"] | MQTT_TLS;
    strlcpy(config.Mqtt.Tls.RootCaCert, mqtt_tls["root_ca_cert"] | MQTT_ROOT_CA_CERT, sizeof(config.Mqtt.Tls.RootCaCert));
    config.Mqtt.Tls.CertLogin = mqtt_tls["certlogin"] | MQTT_TLSCERTLOGIN;
    strlcpy(config.Mqtt.Tls.ClientCert, mqtt_tls["client_cert"] | MQTT_TLSCLIENTCERT, sizeof(config.Mqtt.Tls.ClientCert));
    strlcpy(config.Mqtt.Tls.ClientKey, mqtt_tls["client_key"] | MQTT_TLSCLIENTKEY, sizeof(config.Mqtt.Tls.ClientKey));

    JsonObject mqtt_hass = mqtt["hass"];
    config.Mqtt.Hass.Enabled = mqtt_hass["enabled"] | MQTT_HASS_ENABLED;
    config.Mqtt.Hass.Retain = mqtt_hass["retain"] | MQTT_HASS_RETAIN;
    config.Mqtt.Hass.Expire = mqtt_hass["expire"] | MQTT_HASS_EXPIRE;
    config.Mqtt.Hass.IndividualPanels = mqtt_hass["individual_panels"] | MQTT_HASS_INDIVIDUALPANELS;
    strlcpy(config.Mqtt.Hass.Topic, mqtt_hass["topic"] | MQTT_HASS_TOPIC, sizeof(config.Mqtt.Hass.Topic));
    strlcpy(config.Mqtt.Hass.DeviceName, mqtt_hass["device_name"] | MQTT_HASS_DEVICENAME, sizeof(config.Mqtt.Hass.DeviceName));

    JsonObject security = doc["security"];
    strlcpy(config.Security.Password, security["password"] | ACCESS_POINT_PASSWORD, sizeof(config.Security.Password));
    config.Security.AllowReadonly = security["allow_readonly"] | SECURITY_ALLOW_READONLY;

    JsonObject device = doc["device"];
    strlcpy(config.Dev_PinMapping, device["pinmapping"] | DEV_PINMAPPING, sizeof(config.Dev_PinMapping));

    JsonObject display = device["display"];
    config.Display.PowerSafe = display["powersafe"] | DISPLAY_POWERSAFE;
    config.Display.ScreenSaver = display["screensaver"] | DISPLAY_SCREENSAVER;
    config.Display.Rotation = display["rotation"] | DISPLAY_ROTATION;
    config.Display.Contrast = display["contrast"] | DISPLAY_CONTRAST;
    strlcpy(config.Display.Locale, display["locale"] | DISPLAY_LOCALE, sizeof(config.Display.Locale));
    config.Display.Diagram.Duration = display["diagram_duration"] | DISPLAY_DIAGRAM_DURATION;
    config.Display.Diagram.Mode = display["diagram_mode"] | DISPLAY_DIAGRAM_MODE;

    JsonArray leds = device["led"];
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds[i].as<JsonObject>();
        config.Led_Single[i].Brightness = led["brightness"] | LED_BRIGHTNESS;
    }

    f.close();
    return true;
}

void ConfigurationClass::migrate()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r", false);
    if (!f) {
        MessageOutput.println("Failed to open file, cancel migration");
        return;
    }

    JsonDocument doc;

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.printf("Failed to read file, cancel migration: %s\r\n", error.c_str());
        return;
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return;
    }

    if (config.Cfg.Version < 0x00011800) {
        JsonObject mqtt = doc["mqtt"];
        config.Mqtt.PublishInterval = mqtt["publish_invterval"];
    }

    if (config.Cfg.Version < 0x00011a00) {
        // This migration fixes this issue: https://github.com/espressif/arduino-esp32/issues/8828
        // It occours when migrating from Core 2.0.9 to 2.0.14
        // which was done by updating ESP32 PlatformIO from 6.3.2 to 6.5.0
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (config.Cfg.Version < 0x00011c00) {
        if (!strcmp(config.Ntp.Server, NTP_SERVER_OLD)) {
            strlcpy(config.Ntp.Server, NTP_SERVER, sizeof(config.Ntp.Server));
        }
    }

    if (config.Cfg.Version < 0x00011d00) {
        JsonObject device = doc["device"];
        JsonObject display = device["display"];
        switch (display["language"] | 0U) {
        case 0U:
            strlcpy(config.Display.Locale, "en", sizeof(config.Display.Locale));
            break;
        case 1U:
            strlcpy(config.Display.Locale, "de", sizeof(config.Display.Locale));
            break;
        case 2U:
            strlcpy(config.Display.Locale, "fr", sizeof(config.Display.Locale));
            break;
        }
    }

    f.close();

    config.Cfg.Version = CONFIG_VERSION;
    write();
    read();
}

CONFIG_T const& ConfigurationClass::get()
{
    return config;
}

ConfigurationClass::WriteGuard ConfigurationClass::getWriteGuard()
{
    return WriteGuard();
}

DS18B20SENSOR_CONFIG_T* ConfigurationClass::getFirstDS18B20Config()
{
    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial != 0) {
            return &config.DS18B20.Sensors[i];
        }
    }
    return nullptr;
}

uint32_t ConfigurationClass::getConfiguredSensorCnt()
{
    uint32_t cnt = 0;
    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial != 0) {
            cnt++;
        }
    }
    return cnt;
}

bool ConfigurationClass::addSensor(uint16_t serial)
{
    DS18B20SENSOR_CONFIG_T* actSensor = nullptr;

    // if sensor in config -> OK
    for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
        if (config.DS18B20.Sensors[i].Serial == serial) {
            actSensor = &config.DS18B20.Sensors[i];
            break;
        }
    }

    // new sensor found
    if (actSensor == nullptr) {
        for (uint8_t i = 0; i < TEMPLOGGER_MAX_COUNT; i++) {
            // find free slot with Serial 0
            if (config.DS18B20.Sensors[i].Serial == 0) {
                actSensor = &config.DS18B20.Sensors[i];
                if (!strcmp(actSensor->Name, "undefined")) {
                    snprintf(actSensor->Name, sizeof(actSensor->Name), "Temp_%04X", serial);
                }
                break;
            }
        }
    }

    if (actSensor != nullptr) {
        actSensor->Serial = serial;
    }

    return actSensor != nullptr;
}

void ConfigurationClass::loop()
{
    std::unique_lock<std::mutex> lock(sWriterMutex);
    if (sWriterCount == 0) {
        return;
    }

    sWriterCv.notify_all();
    sWriterCv.wait(lock, [] { return sWriterCount == 0; });
}

CONFIG_T& ConfigurationClass::WriteGuard::getConfig()
{
    return config;
}

ConfigurationClass::WriteGuard::WriteGuard()
    : _lock(sWriterMutex)
{
    sWriterCount++;
    sWriterCv.wait(_lock);
}

ConfigurationClass::WriteGuard::~WriteGuard()
{
    sWriterCount--;
    if (sWriterCount == 0) {
        sWriterCv.notify_all();
    }
}

ConfigurationClass Configuration;