// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "Display_Graphic.h"
#include "Configuration.h"
#include "Datastore.h"
#include "I18n.h"
#include "MessageOutput.h"
#include <NetworkSettings.h>
#include <map>
#include <time.h>

std::map<DisplayType_t, std::function<U8G2*(uint8_t, uint8_t, uint8_t, uint8_t)>> display_types = {
    { DisplayType_t::PCD8544, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_PCD8544_84X48_F_4W_HW_SPI(U8G2_R0, cs, data, reset); } },
    { DisplayType_t::SSD1306, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SSD1309, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SSD1309_128X64_NONAME0_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::ST7567_GM12864I_59N, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_ST7567_ENH_DG128064I_F_HW_I2C(U8G2_R0, reset, clock, data); } },
};

// Language defintion, respect order in translation lists
#define I18N_LOCALE_EN 0
#define I18N_LOCALE_DE 1
#define I18N_LOCALE_FR 2

static const char* const i18n_date_format[] = { "%m/%d/%Y %H:%M", "%d.%m.%Y %H:%M", "%d/%m/%Y %H:%M" };

DisplayGraphicClass::DisplayGraphicClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&DisplayGraphicClass::loop, this))
{
    _actSensorIndex = 0;
    _actPageTime = 0;
}

DisplayGraphicClass::~DisplayGraphicClass()
{
    delete _display;
}

void DisplayGraphicClass::init(Scheduler& scheduler, const DisplayType_t type, const uint8_t data, const uint8_t clk, const uint8_t cs, const uint8_t reset)
{
    _display_type = type;
    if (isValidDisplay()) {
        auto constructor = display_types[_display_type];
        _display = constructor(reset, clk, data, cs);
        if (_display_type == DisplayType_t::ST7567_GM12864I_59N) {
            _display->setI2CAddress(0x3F << 1);
        }
        _display->begin();
        setContrast(DISPLAY_CONTRAST);
        setStatus(true);

        scheduler.addTask(_loopTask);
        _loopTask.setInterval(_period);
        _loopTask.enable();
    }
}

void DisplayGraphicClass::calcLineHeights()
{
    uint8_t yOff = 0;
    for (uint8_t i = 0; i < 4; i++) {
        setFont(i);
        yOff += (_display->getMaxCharHeight());
        _lineOffsets[i] = yOff;
    }
}

void DisplayGraphicClass::setFont(const uint8_t line)
{
    switch (line) {
    // case 0:
    //     _display->setFont((_isLarge) ? u8g2_font_ncenB14_tr : u8g2_font_logisoso16_tr);
    //     break;
    case 3:
        _display->setFont(u8g2_font_5x8_tr);
        break;
    default:
        _display->setFont((_isLarge) ? u8g2_font_ncenB10_tr : u8g2_font_5x8_tr);
        break;
    }
}

bool DisplayGraphicClass::isValidDisplay()
{
    return _display_type > DisplayType_t::None && _display_type < DisplayType_Max;
}

void DisplayGraphicClass::printText(const char* text, const uint8_t line)
{
    uint8_t dispX;
    if (!_isLarge) {
        dispX = 0;
    } else {
        dispX = 5;
    }
    setFont(line);

    dispX += enableScreensaver ? (_mExtra % 7) : 0;
    _display->drawStr(dispX, _lineOffsets[line], text);
}

void DisplayGraphicClass::setOrientation(const uint8_t rotation)
{
    if (!isValidDisplay()) {
        return;
    }

    switch (rotation) {
    case 0:
        _display->setDisplayRotation(U8G2_R0);
        break;
    case 1:
        _display->setDisplayRotation(U8G2_R1);
        break;
    case 2:
        _display->setDisplayRotation(U8G2_R2);
        break;
    case 3:
        _display->setDisplayRotation(U8G2_R3);
        break;
    }

    _isLarge = (_display->getWidth() > 100);
    calcLineHeights();
}

void DisplayGraphicClass::setLocale(const String& locale)
{
    _display_language = locale;
    uint8_t idx = I18N_LOCALE_EN;
    if (locale == "de") {
        idx = I18N_LOCALE_DE;
    } else if (locale == "fr") {
        idx = I18N_LOCALE_FR;
    }

    _i18n_date_format = i18n_date_format[idx];

    String notUsed;
    I18n.readDisplayStrings(locale,
        _i18n_date_format,
        notUsed,
        notUsed,
        notUsed,
        notUsed,
        notUsed,
        notUsed,
        notUsed);
}

void DisplayGraphicClass::setDiagramMode(DiagramMode_t mode)
{
    if (mode < DiagramMode_t::DisplayMode_Max) {
        _diagram_mode = mode;
    }
}

void DisplayGraphicClass::setStartupDisplay()
{
    if (!isValidDisplay()) {
        return;
    }

    _display->clearBuffer();
    printText("Starting ...", 0);
    _display->sendBuffer();
}

void DisplayGraphicClass::loop()
{
    _loopTask.setInterval(_period);

    _display->clearBuffer();

    if (isHallDetected()) {
        _previousMillis = millis();
        setStatus(true);
    }

    if ((millis() - _lastDisplayUpdate) > _period) {
        _display->clearBuffer();
        bool displayPowerSave = false;

        auto& config = Configuration.get();

        uint32_t u32time;
        float value = 0;

        uint8_t line = 0;
        uint8_t sensorIndex = _actSensorIndex;
        while (line < 3) {
            if (config.DS18B20.Sensors[sensorIndex].Serial != 0) {
                if (Datastore.getTemperature(config.DS18B20.Sensors[sensorIndex].Serial, u32time, value)) {
                    snprintf(_fmtText, sizeof(_fmtText), "%.2f %s", value, config.DS18B20.Sensors[sensorIndex].Name);
                } else {
                    snprintf(_fmtText, sizeof(_fmtText), "??? %s", config.DS18B20.Sensors[sensorIndex].Name);
                }
                printText(_fmtText, line);
                line++;
            }
            sensorIndex = (sensorIndex + 1) % TEMPLOGGER_MAX_COUNT;

            // restart
            if (sensorIndex == 0) {
                break;
            }
        }

        _actPageTime = (_actPageTime + 1) % 3;
        if (_actPageTime == 0) {
            _actSensorIndex = sensorIndex;
        }

        //=====> IP or Date-Time ========
        // Change every 3 seconds
        if (!(_mExtra % (3 * 2) < 3) && NetworkSettings.localIP()) {
            printText(NetworkSettings.localIP().toString().c_str(), 3);
        } else {
            // Get current time
            time_t now = time(nullptr);
            strftime(_fmtText, sizeof(_fmtText), _i18n_date_format.c_str(), localtime(&now));
            printText(_fmtText, 3);
        }
        _display->sendBuffer();

        _mExtra++;
        _lastDisplayUpdate = millis();

        // check if it's time to enter power saving mode
        if (millis() - _previousMillis >= (_interval * 2)) {
            displayPowerSave = enablePowerSafe;
        }

        if (!_displayTurnedOn) {
            displayPowerSave = true;
        }

        _display->setPowerSave(displayPowerSave);
    }
}

void DisplayGraphicClass::setContrast(const uint8_t contrast)
{
    if (!isValidDisplay()) {
        return;
    }
    _display->setContrast(contrast * 2.55f);
}

void DisplayGraphicClass::setStatus(const bool turnOn)
{
    _displayTurnedOn = turnOn;
}

//////////////////////////////////////////////////////////////////////
bool DisplayGraphicClass::isHallDetected()
{
#ifdef CONFIG_IDF_TARGET_ESP32S3
    return false;
#else
    return hallRead() < -10; // Reads Hall sensor value
#endif
}

DisplayGraphicClass Display;
