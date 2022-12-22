#pragma once

#include <cstdint>
#include "types.h"
#include <volk/volk_alloc.hh>
#include "common/dsp/constellation.h"

/*
A generic correlator, for (mostly) CCSDS uses
*/
class CorrelatorGeneric
{
private:
    const dsp::constellation_type_t d_modulation;

    int syncword_length;

    float *converted_buffer;
    std::vector<volk::vector<float>> syncwords;

    void rotate_float_buf(float *buf, int size, float rot_deg);
    void modulate_soft(float *buf, uint8_t *bit, int size);

public:
    CorrelatorGeneric(dsp::constellation_type_t mod, std::vector<uint8_t> syncword, int max_frm_size);
    ~CorrelatorGeneric();
    int correlate(int8_t *soft_input, phase_t &phase, bool &swap, float &cor, int length);
};