// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "PinMapping.h"
#include "MessageOutput.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <string.h>

#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE 0U
#endif

#ifndef DISPLAY_DATA
#define DISPLAY_DATA 255U
#endif

#ifndef DISPLAY_CLK
#define DISPLAY_CLK 255U
#endif

#ifndef DISPLAY_CS
#define DISPLAY_CS 255U
#endif

#ifndef DISPLAY_RESET
#define DISPLAY_RESET 255U
#endif

#ifndef LED0
#define LED0 -1
#endif

#ifndef LED1
#define LED1 -1
#endif

#ifndef SENSOR_DS18B20
#define SENSOR_DS18B20 -1
#endif

#ifndef SD_ENABLED
#define SD_ENABLED false
#endif

#ifndef SD_SCK
#define SD_SCK -1
#endif

#ifndef SD_MISO
#define SD_MISO -1
#endif

#ifndef SD_MOSI
#define SD_MOSI -1
#endif

#ifndef SD_CS
#define SD_CS -1
#endif


#ifndef W5500_MOSI
#define W5500_MOSI -1
#endif

#ifndef W5500_MISO
#define W5500_MISO -1
#endif

#ifndef W5500_SCLK
#define W5500_SCLK -1
#endif

#ifndef W5500_CS
#define W5500_CS -1
#endif

#ifndef W5500_INT
#define W5500_INT -1
#endif

#ifndef W5500_RST
#define W5500_RST -1
#endif

#if CONFIG_ETH_USE_ESP32_EMAC

#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR -1
#endif

#ifndef ETH_PHY_POWER
#define ETH_PHY_POWER -1
#endif

#ifndef ETH_PHY_MDC
#define ETH_PHY_MDC -1
#endif

#ifndef ETH_PHY_MDIO
#define ETH_PHY_MDIO -1
#endif

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#endif

#ifndef ETH_CLK_MODE
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#endif

#endif

PinMappingClass PinMapping;

PinMappingClass::PinMappingClass()
{
    memset(&_pinMapping, 0x0, sizeof(_pinMapping));

    _pinMapping.w5500_mosi = W5500_MOSI;
    _pinMapping.w5500_miso = W5500_MISO;
    _pinMapping.w5500_sclk = W5500_SCLK;
    _pinMapping.w5500_cs = W5500_CS;
    _pinMapping.w5500_int = W5500_INT;
    _pinMapping.w5500_rst = W5500_RST;

#if CONFIG_ETH_USE_ESP32_EMAC
#ifdef OPENDTU_ETHERNET
    _pinMapping.eth_enabled = true;
#else
    _pinMapping.eth_enabled = false;
#endif
    _pinMapping.eth_phy_addr = ETH_PHY_ADDR;
    _pinMapping.eth_power = ETH_PHY_POWER;
    _pinMapping.eth_mdc = ETH_PHY_MDC;
    _pinMapping.eth_mdio = ETH_PHY_MDIO;
    _pinMapping.eth_type = ETH_PHY_TYPE;
    _pinMapping.eth_clk_mode = ETH_CLK_MODE;
#endif

    _pinMapping.display_type = DISPLAY_TYPE;
    _pinMapping.display_data = DISPLAY_DATA;
    _pinMapping.display_clk = DISPLAY_CLK;
    _pinMapping.display_cs = DISPLAY_CS;
    _pinMapping.display_reset = DISPLAY_RESET;

    _pinMapping.led[0] = LED0;
    _pinMapping.led[1] = LED1;

    _pinMapping.sensor_ds18b20 = SENSOR_DS18B20;

    _pinMapping.sd_enabled = SD_ENABLED;
    _pinMapping.sd_sck = SD_SCK;
    _pinMapping.sd_miso = SD_MISO;
    _pinMapping.sd_mosi = SD_MOSI;
    _pinMapping.sd_cs = SD_CS;
}

PinMapping_t& PinMappingClass::get()
{
    return _pinMapping;
}

bool PinMappingClass::init(const String& deviceMapping)
{
    File f = LittleFS.open(PINMAPPING_FILENAME, "r", false);

    if (!f) {
        return false;
    }

    Utils::skipBom(f);

    JsonDocument doc;
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }

    for (uint8_t i = 0; i < doc.size(); i++) {
        String devName = doc[i]["name"] | "";
        if (devName == deviceMapping) {
            strlcpy(_pinMapping.name, devName.c_str(), sizeof(_pinMapping.name));

            _pinMapping.w5500_mosi = doc[i]["w5500"]["mosi"] | W5500_MOSI;
            _pinMapping.w5500_miso = doc[i]["w5500"]["miso"] | W5500_MISO;
            _pinMapping.w5500_sclk = doc[i]["w5500"]["sclk"] | W5500_SCLK;
            _pinMapping.w5500_cs = doc[i]["w5500"]["cs"] | W5500_CS;
            _pinMapping.w5500_int = doc[i]["w5500"]["int"] | W5500_INT;
            _pinMapping.w5500_rst = doc[i]["w5500"]["rst"] | W5500_RST;

#if CONFIG_ETH_USE_ESP32_EMAC
#ifdef OPENDTU_ETHERNET
            _pinMapping.eth_enabled = doc[i]["eth"]["enabled"] | true;
#else
            _pinMapping.eth_enabled = doc[i]["eth"]["enabled"] | false;
#endif
            _pinMapping.eth_phy_addr = doc[i]["eth"]["phy_addr"] | ETH_PHY_ADDR;
            _pinMapping.eth_power = doc[i]["eth"]["power"] | ETH_PHY_POWER;
            _pinMapping.eth_mdc = doc[i]["eth"]["mdc"] | ETH_PHY_MDC;
            _pinMapping.eth_mdio = doc[i]["eth"]["mdio"] | ETH_PHY_MDIO;
            _pinMapping.eth_type = doc[i]["eth"]["type"] | ETH_PHY_TYPE;
            _pinMapping.eth_clk_mode = doc[i]["eth"]["clk_mode"] | ETH_CLK_MODE;
#endif

            _pinMapping.display_type = doc[i]["display"]["type"] | DISPLAY_TYPE;
            _pinMapping.display_data = doc[i]["display"]["data"] | DISPLAY_DATA;
            _pinMapping.display_clk = doc[i]["display"]["clk"] | DISPLAY_CLK;
            _pinMapping.display_cs = doc[i]["display"]["cs"] | DISPLAY_CS;
            _pinMapping.display_reset = doc[i]["display"]["reset"] | DISPLAY_RESET;

            _pinMapping.led[0] = doc[i]["led"]["led0"] | LED0;
            _pinMapping.led[1] = doc[i]["led"]["led1"] | LED1;

            _pinMapping.sensor_ds18b20 = doc[i]["sensor"]["ds18b20"] | SENSOR_DS18B20;

            _pinMapping.sd_enabled = doc[i]["sd"]["enabled"] | SD_ENABLED;
            _pinMapping.sd_sck = doc[i]["sd"]["sck"] | SD_SCK;
            _pinMapping.sd_miso = doc[i]["sd"]["miso"] | SD_MISO;
            _pinMapping.sd_mosi = doc[i]["sd"]["mosi"] | SD_MOSI;
            _pinMapping.sd_cs = doc[i]["sd"]["cs"] | SD_CS;

            return true;
        }
    }

    return false;
}

bool PinMappingClass::isValidW5500Config() const
{
    return _pinMapping.w5500_mosi >= 0
        && _pinMapping.w5500_miso >= 0
        && _pinMapping.w5500_sclk >= 0
        && _pinMapping.w5500_cs >= 0
        && _pinMapping.w5500_int >= 0
        && _pinMapping.w5500_rst >= 0;
}

#if CONFIG_ETH_USE_ESP32_EMAC
bool PinMappingClass::isValidEthConfig() const
{
    return _pinMapping.eth_enabled
        && _pinMapping.eth_mdc >= 0
        && _pinMapping.eth_mdio >= 0;
}
#endif
