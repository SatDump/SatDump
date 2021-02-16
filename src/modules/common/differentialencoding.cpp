/*
 * DifferentialDecoder.cpp
 *
 *  Created on: 25/01/2017
 *      Author: Lucas Teske
 *      Modified by Aang23
 */

#include "differentialencoding.h"

void NRZMDiff::decode(uint8_t *data, int length)
{
    uint8_t mask;
    for (int i = 0; i < length; i++)
    {
        mask = ((data[i] >> 1) & 0x7F) | (lastBit << 7);
        lastBit = data[i] & 1;
        data[i] ^= mask;
    }
}