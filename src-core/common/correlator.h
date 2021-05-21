#pragma once

#include <cstdint>

enum modulation_t
{
    BPSK, // TODO
    QPSK
};

enum phase_t
{
    PHASE_0,
    PHASE_90,
    PHASE_180,
    PHASE_270,
};

/*
A generic correlator, for (mostly) CCSDS uses
Based on : 
https://github.com/opensatelliteproject/libsathelper
https://github.com/dbdexter-dev/meteor_decode
*/
class Correlator
{
private:
    const modulation_t d_modulation;
    uint64_t syncwords[8];
    uint8_t *hard_buf;

public:
    Correlator(modulation_t mod, uint64_t syncword);
    ~Correlator();
    int correlate(int8_t *soft_input, phase_t &phase, bool &swap, int &cor, int length);
};