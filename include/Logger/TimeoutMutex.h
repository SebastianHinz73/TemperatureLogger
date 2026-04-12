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
        if(_lockEnd > 0 && millis() > _lockEnd)
        {
            MessageOutput.println("TimeoutMutex: Force unlock");
            std::mutex::unlock(); // force unlock
            // Try once more without recursion
            if(try_lock())
            {
                _lockEnd = millis() + maxUse;
                return true;
            }
        }
        return false;
    }

    void unlock()
    {
        _lockEnd = 0;
        std::mutex::unlock();
    }

private:
    unsigned long _lockEnd = 0;
};

