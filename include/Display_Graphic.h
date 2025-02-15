// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "defaults.h"
#include <TaskSchedulerDeclarations.h>
#include <U8g2lib.h>

enum DisplayType_t {
    None,
    PCD8544,
    SSD1306,
    SH1106,
    SSD1309,
    ST7567_GM12864I_59N,
    DisplayType_Max,
};

enum DiagramMode_t {
    Off,
    Small,
    Fullscreen,
    DisplayMode_Max,
};

class DisplayGraphicClass {
public:
    DisplayGraphicClass();
    ~DisplayGraphicClass();

    void init(Scheduler& scheduler, const DisplayType_t type, const uint8_t data, const uint8_t clk, const uint8_t cs, const uint8_t reset);
    void setContrast(const uint8_t contrast);
    void setStatus(const bool turnOn);
    void setOrientation(const uint8_t rotation = DISPLAY_ROTATION);
    void setLocale(const String& locale);
    void setDiagramMode(DiagramMode_t mode);
    void setStartupDisplay();

    bool enablePowerSafe = true;
    bool enableScreensaver = true;

private:
    void loop();
    void printText(const char* text, const uint8_t line);
    void calcLineHeights();
    void setFont(const uint8_t line);
    bool isValidDisplay();
    bool isHallDetected();

    Task _loopTask;

    U8G2* _display;

    bool _displayTurnedOn;

    DisplayType_t _display_type = DisplayType_t::None;
    DiagramMode_t _diagram_mode = DiagramMode_t::Off;
    String _display_language = DISPLAY_LOCALE;
    uint8_t _mExtra;
    uint8_t _actSensorIndex;
    uint8_t _actPageTime;
    uint16_t _period = 1000;
    uint16_t _interval = 60000; // interval at which to power save (milliseconds)
    uint32_t _lastDisplayUpdate = 0;
    uint32_t _previousMillis = 0;
    char _fmtText[50];
    bool _isLarge = false;
    uint8_t _lineOffsets[5];
    String _i18n_date_format;
};

extern DisplayGraphicClass Display;