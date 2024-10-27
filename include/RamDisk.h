// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "IDataStoreDevice.h"
#include <Arduino.h>


////////////////////////

class RamDiskClass : public IDataStoreDevice {
public:
    RamDiskClass() { }
    void init();

    // IDataStoreDevice
    virtual void writeValue(uint16_t serial, time_t time, float value);
    virtual bool getFileSize(uint16_t serial, const tm& timeinfo, size_t& size);
    virtual bool getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller);

private:

};
extern RamDiskClass RamDisk;
