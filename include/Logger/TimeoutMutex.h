// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <Arduino.h>
#include <mutex>


class TimeoutMutex : public std::mutex{
public:
    bool TryLock(unsigned long maxWait, unsigned long maxUse)
    {
        if(try_lock())
        {
            _lockEnd = millis() + maxUse;
            return true;
        }
        if(millis() > _lockEnd)
        {
            unlock(); // force unlock
            return TryLock(maxWait, maxUse);
        }
        return false;
    }

private:
    time_t _lockEnd;
};

