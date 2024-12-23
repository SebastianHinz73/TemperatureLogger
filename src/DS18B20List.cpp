// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Sebastian Hinz
 */

#include "DS18B20List.h"
#include "Configuration.h"
#include "Crc16.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <AsyncJson.h>

#define TEMPERATURE_PRECISION 12
#define TIME_SCAN_NEW_SENSORS 60

DS18B20ListClass DS18B20List;

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++) {
        // zero pad the address if necessary
        if (deviceAddress[i] < 16)
            MessageOutput.print("0");
        MessageOutput.print(deviceAddress[i], HEX);
    }
}

bool is_eqal(const DeviceAddress& a, const DeviceAddress& b)
{
    return !(a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || a[3] != b[3] || a[4] != b[4] || a[5] != b[5] || a[6] != b[6] || a[7] != b[7]);
}

DS18B20ListClass::DS18B20ListClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&DS18B20ListClass::loop, this))
{
}

void DS18B20ListClass::init(Scheduler& scheduler)
{
    const PinMapping_t& pin = PinMapping.get();

    scheduler.addTask(_loopTask);
    _loopTask.enable();

    _ow.begin(pin.sensor_ds18b20);
    _sensors.setOneWire(&_ow);
    _sensors.begin();

    scanSensors();
    readTemperature();

    _lastScan = millis();
    _lastTemperatureUpdate = millis();
}

void DS18B20ListClass::loop()
{
    if (millis() - _lastScan > TIME_SCAN_NEW_SENSORS * 1000) {
        scanSensors();
        _lastScan = millis();
    }

    auto config = Configuration.get();
    if (millis() - _lastTemperatureUpdate > config.DS18B20.PollInterval * 1000) {
        readTemperature();
        _lastTemperatureUpdate = millis();
    }
}

void DS18B20ListClass::scanSensors()
{
    // locate devices on the bus
    MessageOutput.print("Locating devices...");
    MessageOutput.print("Found ");
    MessageOutput.print(_sensors.getDeviceCount(), DEC);
    MessageOutput.println(" devices.");

    bool writeConfig = false;

    for (int i = 0; i < _sensors.getDeviceCount(); i++) {
        DeviceAddress addr;
        if (!_sensors.getAddress(addr, i)) {
            MessageOutput.printf("Unable to find address for Device %d\r\n", i);
            continue;
        }
        if (i >= TEMPLOGGER_MAX_COUNT) {
            MessageOutput.printf("To many connected DS18B20 sensors. Max=%d, Found=%d\r\n", TEMPLOGGER_MAX_COUNT, _sensors.getDeviceCount());
            break;
        }

        auto it = std::find_if(_list.begin(), _list.end(), [&](std::unique_ptr<DS18B20> const& obj) { 
                                if (is_eqal(obj.get()->_addr, addr))
                                {
                                    return true;
                                }
                                return false; });

        // not yet found by scan
        if (it == _list.end()) {
            char crc16Buffer[5]; // build CRC16
            snprintf(crc16Buffer, sizeof(crc16Buffer), "%04X", Crc16::Calc(addr, sizeof(DeviceAddress)));
            // For compatibility with the old version. First convert to 4 byte string and then to uint16_t.
            uint16_t serial = strtol(crc16Buffer, 0, 16);
            // add to data store
            Datastore.addSensor(serial);

            _sensors.setResolution(addr, TEMPERATURE_PRECISION);
            // add to own list
            _list.push_back(std::make_unique<DS18B20>(addr, serial));

            // printAddress(addr);

            // add to configuration
            if (!Configuration.addSensor(serial)) {
                MessageOutput.printf("Can not add %04X sensor to configuration. Remove unconnected DS18B20 sensors from configuration.\r\n", serial);
            }

            writeConfig = true;
        }
    }
    if (writeConfig) {
        Configuration.write();
    }
}

void DS18B20ListClass::readTemperature()
{
    _sensors.requestTemperatures();
    auto config = Configuration.get();

    for (const auto& entry : _list) {
        float temp;
        if (config.DS18B20.Fahrenheit) {
            temp = _sensors.getTempF(entry->_addr);
            if (temp == DEVICE_DISCONNECTED_F) {
                MessageOutput.println("Error: Could not read temperature data");
                continue;
            }
        } else {
            temp = _sensors.getTempC(entry->_addr);
            if (temp == DEVICE_DISCONNECTED_C) {
                MessageOutput.println("Error: Could not read temperature data");
                continue;
            }
        }
        MessageOutput.printf("Temp %04X: %.2f\r\n", entry->_serial, temp);
        Datastore.addValue(entry->_serial, temp);
    }
}
