// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

typedef std::function<size_t(uint8_t* buffer, size_t maxLen, size_t alreadySent, size_t fileSize)> ResponseFiller;

////////////////////////

class IDataStoreDevice {
public:
    virtual ~IDataStoreDevice() { }
    virtual void writeValue(uint16_t serial, time_t time, float value) = 0;
    virtual bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size) = 0;
    virtual bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller) = 0;
};
