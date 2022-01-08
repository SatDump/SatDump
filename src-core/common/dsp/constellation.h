#pragma once

#include "complex.h"

namespace dsp
{
    enum constellation_type_t
    {
        BPSK,
        QPSK,
        PSK8,
        APSK16,
        APSK32
    };

    /*
    This class was made using GNU Radio and LeanDVB's implementation
    as an example, mostly because it was written while experimenting 
    with DVB-S2.

    TODO : Faster LUT Lookup for soft bits, to make it anywhere near
    usable. This has not been yet as BPSK and QPSK can simply be 
    soft-demodulated using the I and Q branches directly.

    TODO : Optimize the soft calc function... We don't need to use a
    std::vector and such.

    This constellation parser expects symbols to be around 1 and -1.
    */
    class constellation_t
    {
    protected:
        const constellation_type_t const_type; // Constellation type
        int const_bits;                        // Number of bits transmitted per sample
        int const_states;                      // Number of possible states
        complex_t *constellation;              // LUT used for modulation
        float const_amp = 1.0f;                // Required for APSK
        float const_sca = 50.0f;               // Const scale for soft symbols

        complex_t polar(float r, int n, float i);
        int8_t clamp(float x);

    public:
        constellation_t(constellation_type_t type, float g1 = 0, float g2 = 0);
        ~constellation_t();

        complex_t mod(uint8_t symbol);                                                                        // Modulate a raw symbol. Proper resampling and RRC filtering is still required!
        uint8_t demod(complex_t sample);                                                                      // Demodulate a complex sample to hard bits
        uint8_t soft_demod(int8_t *sample);                                                                   // Demodulate a complex sample stored as soft (I/Q) bits to hard bits
        void soft_demod(int8_t *samples, int size, uint8_t *bits);                                            // Demodulate a full buffer of softs
        void demod_soft_calc(complex_t sample, int8_t *bits, float *phase_error = nullptr, float npwr = 1.0); // Demodulate to soft symbols
    };
};