// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
//#include <Arduino.h>
#include "MessageOutput.h"
#include <mutex>


class TimeoutMutex : public std::mutex{
public:
    bool TryLock(int maxWait, int maxUse)
    {
        for(int i = 0; i <= maxWait; i+=10)
        {
            if(try_lock())
            {
                _lockEnd = millis() + maxUse;
                return true;
            }
            delay(10);
        }
        if(millis() > _lockEnd)
        {
            unlock(); // force unlock
            return TryLock(0, maxUse);
        }
        return false;
    }

    void unlock()
    {
        std::mutex::unlock();
    }

private:
    time_t _lockEnd;
};

