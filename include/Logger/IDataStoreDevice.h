// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

typedef std::function<size_t(uint8_t* buffer, size_t maxLen, size_t alreadySent)> ResponseFiller;

////////////////////////

class IDataStoreDevice {
public:
    virtual ~IDataStoreDevice() { }
    virtual void writeValue(uint16_t serial, time_t time, float value) = 0;
    virtual bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller) = 0;
    virtual bool getFile(uint16_t serial, time_t start, uint32_t length, ResponseFiller& responseFiller) = 0;
};
