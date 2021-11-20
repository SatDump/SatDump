#pragma once

#include <cstdint>
#include "types.h"

/*
A generic correlator, for (mostly) CCSDS uses
Based on : 
https://github.com/opensatelliteproject/libsathelper
https://github.com/dbdexter-dev/meteor_decode
*/
class Correlator32
{
private:
    const constellation_t d_modulation;
    uint32_t syncwords[8];
    uint8_t *hard_buf;

public:
    Correlator32(constellation_t mod, uint32_t syncword);
    ~Correlator32();
    int correlate(int8_t *soft_input, phase_t &phase, bool &swap, int &cor, int length);
};