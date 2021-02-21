/*
 * DeRandomizer.h
 *
 *  Created on: 07/11/2016
 *      Author: Lucas Teske
 */

#pragma once

#include <cstdint>

namespace sathelper
{
    class Derandomizer
    {
    private:
        static uint8_t pn[255];

    public:
        static void work(uint8_t *data, int length);
    };
}
