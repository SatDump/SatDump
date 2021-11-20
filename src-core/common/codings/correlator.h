#pragma once

#include <cstdint>
#include "types.h"

/*
A generic correlator, for (mostly) CCSDS uses
Based on : 
https://github.com/opensatelliteproject/libsathelper
https://github.com/dbdexter-dev/meteor_decode
*/
class Correlator
{
private:
    const constellation_t d_modulation;
    uint64_t syncwords[8];
    uint8_t *hard_buf;

public:
    Correlator(constellation_t mod, uint64_t syncword);
    ~Correlator();
    int correlate(int8_t *soft_input, phase_t &phase, bool &swap, int &cor, int length);
};