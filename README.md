# Temperature Logger

This project is based on OpenDTU. Thank you for that!

There is a project page with more information. <https://www.diy-temperature-logger.com/>

The project is licensed under an Open Source License ( GNU General Public License version 2).

## Overview

- Project for reading out DS18B20 sensors with an ESP32 board
- Up to 5 DS18B20 pins with a total of 30 sensors can be configured
- Support for OLED displays
- MQTT support with Home Assistant Auto Discovery
- Support for ESP32 or ESP32-S3 (an ESP32-S3 chip with PSRAM is recommended)
- Data can be stored on SD cards. Thats recommended for ESP32 boards.
- Data can be stored in PSRam (6MByte). Thats recommended for ESP32-S3 N16N8. The data there will also survive a software board reset. This works very well up to 30 days. Error detection and correction is used.
- Data can be stored in RAM (4KBytes). That's not really recommended, since the memory can only hold about 240 entries.
- Export and import of data in PSRam
- Pins for sensors, display etc. are configurable
- Online Debug Console
- Compatible with the Android IoT Sensor app with which the temperature history can be displayed.

## Some Picture

![Overview Main](docs/pics/overview1.png)

The DS18B20 sensors are recognized automatically. It is possible to change the names. Sensors that are not connected can be deleted.  It is possible to change the order of the sensors.

![Overview Settings](docs/pics/overview2.png)

## Installation

Please have a look at: [Installation](install/README.md)
