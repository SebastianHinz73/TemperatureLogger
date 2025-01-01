//-------------------------------------------------------------------------------------
// CRC16 support class

#pragma once
#include "Arduino.h"

class Crc16 {
private:
    //-------------------------------------------------------
    // Reflects bit in a uint8_t
    //-------------------------------------------------------
    static uint8_t reflect(uint8_t data, uint8_t bits = 32)
    {
        unsigned long reflection = 0x00000000;
        // Reflect the data about the center bit.
        for (uint8_t bit = 0; bit < bits; bit++) {
            // If the LSB bit is set, set the reflection of it.
            if ((data & 0x01) != 0) {
                reflection |= (unsigned long)(1 << ((bits - 1) - bit));
            }

            data = (uint8_t)(data >> 1);
        }

        return reflection;
    }

public:
    // The Calculation results differs from other 16 bit crc implementations. I dont want to have other results (historical reasons)
    static unsigned int fastCrc(uint8_t data[], uint8_t start, uint16_t length, uint16_t polynomial, uint16_t xorIn, uint16_t xorOut, uint16_t msbMask, uint16_t mask)
    {
        unsigned int crc = xorIn;

        int j;
        uint8_t c;
        unsigned int bit;

        if (length == 0)
            return crc;

        for (int i = start; i < (start + length); i++) {
            c = data[i];

            j = 0x80;

            while (j > 0) {
                bit = (unsigned int)(crc & msbMask);
                crc <<= 1;

                if ((c & j) != 0) {
                    bit = (unsigned int)(bit ^ msbMask);
                }

                if (bit != 0) {
                    crc ^= polynomial;
                }
                j >>= 1;
            }
        }

        return crc;
    }

    static unsigned int Calc(uint8_t data[], uint16_t length)
    {
        //  XModem parameters: poly=0x1021 init=0x0000 xorout=0x0000
        return fastCrc(data, 0, length, 0x1021, 0x0000, 0x0000, 0x8000, 0xffff);
    }
};
