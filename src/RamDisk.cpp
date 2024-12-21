// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Sebastian Hinz
 */

#include "RamDisk.h"
#include "MessageOutput.h"
#include <PinMapping.h>

//////////////////////////////////////////
MyCircularBuffer::MyCircularBuffer(int elements)
{
    // time = 8 bytes, data = 8 Bytes, sensor = 1 Byte
    _start = new dataEntry_t[elements];
    _first = _start;
    _last = _start;
    _end = &_start[elements];
    // MessageOutput.printf("MyCircularBuffer::MyCircularBuffer elements %d index %d, dataEntry-Size:%d, _end - _start:%d\r\n", elements, toIndex(_end), sizeof(dataEntry_t), (char*)_end-(char*)_start);
}

MyCircularBuffer::~MyCircularBuffer()
{
    delete[] _start;
}

void MyCircularBuffer::writeValue(uint16_t serial, time_t time, float value)
{
    _last->serial = serial;
    _last->time = time;
    _last->value = value;
    MessageOutput.printf("writeValue1: ");
    debugPrint(_last);
    _last++;
    // MessageOutput.printf("writeValue1: _start(%d), _first(%d), _last(%d), _end(%d)\r\n", toIndex(_start), toIndex(_first), toIndex(_last), toIndex(_end));

    // last on end -> begin with start
    if (_last == _end) {
        _last = _start;
    }

    // last overwrites first -> increase first
    if (_last == _first) {
        _first++;
        if (_first == _end) {
            _first = _start;
        }
    }
    // MessageOutput.printf("writeValue2: _start(%d), _first(%d), _last(%d), _end(%d)\r\n", toIndex(_start), toIndex(_first), toIndex(_last), toIndex(_end));
}

int MyCircularBuffer::getEntryCnt(uint16_t serial, time_t time)
{
    int entryCnt = 0;
    dataEntry_t* act_array[2] = { _first, _start };

    for (int i = 0; i < 2; i++) {
        dataEntry_t* act = act_array[i];

        while (act < _end) {

            // end check
            if (act->time >= time + 24 * 60 * 60 || act == _last) {
                return entryCnt;
            }

            // serial && time check
            if (serial != act->serial || act->time < time) {
                act++;
                continue;
            }

            entryCnt++;
            act++;
        }
    }

    return entryCnt;
}

int MyCircularBuffer::toIndex(const dataEntry_t* entry)
{
    return (entry - _start);
}

void MyCircularBuffer::debugPrint(const dataEntry_t* entry)
{
    MessageOutput.printf("## %d: 0x%x, (%d, %05.2f)\r\n", toIndex(entry), entry->serial, entry->time, entry->value);
}

bool MyCircularBuffer::getEntry(uint16_t serial, time_t time, dataEntry_t*& act)
{
    // start with _first, then increment
    if (act == nullptr) {
        act = _first;
    } else if (act == _last) {
        return false;
    } else {
        act++;
    }

    for (int i = 0; i < 2; i++) {
        while (act < _end) {

            // end check
            if (act->time >= time + 24 * 60 * 60 || act == _last) {
                return false;
            }

            // serial && time check
            if (serial != act->serial || act->time < time) {
                act++;
                continue;
            }

            return true;
        }
        act = _start; // start again with _start
    }
    return false;
}

//////////////////////////////////////////

RamDiskClass* pRamDisk = nullptr;

void RamDiskClass::init()
{
}

void RamDiskClass::writeValue(uint16_t serial, time_t time, float value)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _myBuffer->writeValue(serial, time, value);
/*
    static bool b = false;
    if (!b) {
        b = true;
        int cnt = 0;
        for (time_t t = time - 600*500; t < time; t += 600) {
            _myBuffer->writeValue(serial, t, cnt++ % 30);
        }
    }
    */
}

bool RamDiskClass::getFileSize(uint16_t serial, const tm& timeinfo, size_t& size)
{
    std::lock_guard<std::mutex> lock(_mutex);
    size = 0;

    /*int entryCnt = _myBuffer->getEntryCnt(serial, getStartOfDay(timeinfo));
    size = entryCnt * ENTRY_TO_STRING_SIZE;
*/
    return true;
}

bool RamDiskClass::getFile(uint16_t serial, const tm& timeinfo, ResponseFiller& responseFiller)
{
    _mutex.lock();

    static dataEntry_t* act;
    act = nullptr;
    time_t start_of_day = getStartOfDay(timeinfo);

    responseFiller = [&, serial, start_of_day](uint8_t* buffer, size_t maxLen, size_t alreadySent, size_t fileSize) -> size_t {
        size_t ret = 0;
        size_t maxCnt = maxLen / ENTRY_TO_STRING_SIZE;

        // MessageOutput.printf("RamDiskClass::getFile responseFiller maxLen:%d, alreadySent:%d, fileSize:%d, maxCnt:%d\r\n", maxLen, alreadySent, fileSize, maxCnt);
        for (size_t cnt = 0; cnt < maxCnt; cnt++) {
            if (!_myBuffer->getEntry(serial, start_of_day, act)) {
                break;
            }
            int h = (act->time - start_of_day) / 3600;
            int min = ((act->time - start_of_day) - h * 3600) / 60;
            int sec = (act->time - start_of_day) - h * 3600 - min * 60;
            snprintf((char*)&buffer[cnt * ENTRY_TO_STRING_SIZE], ENTRY_TO_STRING_SIZE, "%02d:%02d:%02d;%05.2f\n", h, min, sec, act->value);

            buffer[(cnt + 1) * ENTRY_TO_STRING_SIZE - 1] = '\n';
            ret += ENTRY_TO_STRING_SIZE;
        }

        if (ret == 0) {
            _mutex.unlock();
        }
        return ret;
    };

    return true;
}

time_t RamDiskClass::getStartOfDay(const tm& timeinfo)
{
    tm info;
    memcpy(&info, &timeinfo, sizeof(tm));
    info.tm_hour = 0;
    info.tm_min = 0;
    info.tm_sec = 0;
    return mktime(&info);
}
