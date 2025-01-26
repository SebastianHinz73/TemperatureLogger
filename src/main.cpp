// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "Configuration.h"
#include "Datastore.h"
#include "Display_Graphic.h"
#include "I18n.h"
#include "Led_Single.h"
#include "Logger/DS18B20List.h"
#include "Logger/RamDrive.h"
#include "Logger/SDCard.h"
#include "MessageOutput.h"
#include "MqttHandleDS18B20.h"
#include "MqttHandleDtu.h"
#include "MqttHandleHass.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "RestartHelper.h"
#include "Scheduler.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <SpiManager.h>
#include <TaskScheduler.h>
#include <esp_heap_caps.h>

void setup()
{
    // Move all dynamic allocations >512byte to psram (if available)
    heap_caps_malloc_extmem_enable(512);

    // Initialize SpiManager
    SpiManagerInst.register_bus(SPI2_HOST);
#if SOC_SPI_PERIPH_NUM > 2
    SpiManagerInst.register_bus(SPI3_HOST);
#endif

    RamDriveClass::AllocateRamDrive();

    // Initialize serial output
    Serial.begin(SERIAL_BAUDRATE);
#if !ARDUINO_USB_CDC_ON_BOOT
    // Only wait for serial interface to be set up when not using CDC
    while (!Serial)
        yield();
#endif
    MessageOutput.init(scheduler);
    MessageOutput.println();
    MessageOutput.println("Starting Logger");

    // Initialize file system
    MessageOutput.print("Initialize FS... ");
    if (!LittleFS.begin(false)) { // Do not format if mount failed
        MessageOutput.print("failed... trying to format...");
        if (!LittleFS.begin(true)) {
            MessageOutput.print("success");
        } else {
            MessageOutput.print("failed");
        }
    } else {
        MessageOutput.println("done");
    }

    // Read configuration values
    Configuration.init(scheduler);
    MessageOutput.print("Reading configuration... ");
    if (!Configuration.read()) {
        if (Configuration.write()) {
            MessageOutput.print("written... ");
        } else {
            MessageOutput.print("failed... ");
        }
    }
    if (Configuration.get().Cfg.Version != CONFIG_VERSION) {
        MessageOutput.print("migrated... ");
        Configuration.migrate();
    }
    auto& config = Configuration.get();
    MessageOutput.println("done");

    // Read languate pack
    MessageOutput.print("Reading language pack... ");
    I18n.init(scheduler);
    MessageOutput.println("done");

    // Load PinMapping
    MessageOutput.print("Reading PinMapping... ");
    if (PinMapping.init(Configuration.get().Dev_PinMapping)) {
        MessageOutput.print("found valid mapping ");
    } else {
        MessageOutput.print("using default config ");
    }
    const auto& pin = PinMapping.get();
    MessageOutput.println("done");

    // Initialize Network
    MessageOutput.print("Initialize Network... ");
    NetworkSettings.init(scheduler);
    MessageOutput.println("done");
    NetworkSettings.applyConfig();

    // Initialize NTP
    MessageOutput.print("Initialize NTP... ");
    NtpSettings.init();
    MessageOutput.println("done");

    // Initialize MqTT
    MessageOutput.print("Initialize MqTT... ");
    MqttSettings.init();
    MqttHandleDtu.init(scheduler);
    MqttHandleDS18B20.init(scheduler);
    MqttHandleHass.init(scheduler);
    MessageOutput.println("done");

    // Initialize WebApi
    MessageOutput.print("Initialize WebApi... ");
    WebApi.init(scheduler);
    MessageOutput.println("done");

    if (static_cast<DisplayType_t>(pin.display_type) != DisplayType_t::None) {
        // Initialize Display
        MessageOutput.print("Initialize Display... ");

        Display.init(scheduler,
            static_cast<DisplayType_t>(pin.display_type),
            pin.display_data,
            pin.display_clk,
            pin.display_cs,
            pin.display_reset);
        Display.setDiagramMode(static_cast<DiagramMode_t>(config.Display.Diagram.Mode));
        Display.setOrientation(config.Display.Rotation);
        Display.enablePowerSafe = config.Display.PowerSafe;
        Display.enableScreensaver = config.Display.ScreenSaver;
        Display.setContrast(config.Display.Contrast);
        Display.setLocale(config.Display.Locale);
        Display.setStartupDisplay();
        MessageOutput.println("done");
    }

    MessageOutput.print("Initialize DS18B20 ... ");
    DS18B20List.init(scheduler);
    MessageOutput.println("done");

    if (pin.sd_enabled) {
        MessageOutput.print("Initialize SD card ... ");
        pSDCard = new SDCardClass();
        pSDCard->init(scheduler);
        Datastore.init(static_cast<IDataStoreDevice*>(pSDCard));
        MessageOutput.println("done");
        RamDriveClass::FreeRamDrive();
    } else {
        // https://esp32.com/viewtopic.php?t=11767
        // PSRAM contains data also after reset
        MessageOutput.print("Initialize Ram drive ... ");

        pRamDrive = new RamDriveClass();
        Datastore.init(static_cast<IDataStoreDevice*>(pRamDrive));
        MessageOutput.println("done");
    }

    RestartHelper.init(scheduler);
}

void loop()
{
    scheduler.execute();
}