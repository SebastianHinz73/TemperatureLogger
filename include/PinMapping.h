// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <ETH.h>
#include <stdint.h>

#define PINMAPPING_FILENAME "/pin_mapping.json"
#define PINMAPPING_LED_COUNT 2

#define MAPPING_NAME_STRLEN 31

struct PinMapping_t {
    char name[MAPPING_NAME_STRLEN + 1];

    int8_t w5500_mosi;
    int8_t w5500_miso;
    int8_t w5500_sclk;
    int8_t w5500_cs;
    int8_t w5500_int;
    int8_t w5500_rst;

#if CONFIG_ETH_USE_ESP32_EMAC
    int8_t eth_phy_addr;
    bool eth_enabled;
    int eth_power;
    int eth_mdc;
    int eth_mdio;
    eth_phy_type_t eth_type;
    eth_clock_mode_t eth_clk_mode;
#endif

    uint8_t display_type;
    uint8_t display_data;
    uint8_t display_clk;
    uint8_t display_cs;
    uint8_t display_reset;

    int8_t led[PINMAPPING_LED_COUNT];

    int sensor_ds18b20;

    bool sd_enabled;
    int sd_sck;
    int sd_miso;
    int sd_mosi;
    int sd_cs;
};

class PinMappingClass {
public:
    PinMappingClass();
    bool init(const String& deviceMapping);
    PinMapping_t& get();

    bool isValidW5500Config() const;
#if CONFIG_ETH_USE_ESP32_EMAC
    bool isValidEthConfig() const;
#endif

private:
    PinMapping_t _pinMapping;
};

extern PinMappingClass PinMapping;