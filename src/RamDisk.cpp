// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "RamDisk.h"
#include "MessageOutput.h"
#include <PinMapping.h>

RamDiskClass RamDisk;

void RamDiskClass::init()
{
}

void RamDiskClass::writeValue(uint16_t serial, time_t time, float value)
{
}

bool RamDiskClass::getFileSize(uint16_t serial, const tm& timeinfo, size_t& size)
{
    return true;
}

bool RamDiskClass::getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller)
{
    return true;
}
