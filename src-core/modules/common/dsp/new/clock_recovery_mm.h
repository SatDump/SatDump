#pragma once

#include "block.h"
#include "modules/common/dsp/clock_recovery_mm.h"

namespace dsp
{
    class CCMMClockRecoveryBlock : public Block<std::complex<float>, std::complex<float>>
    {
    private:
        dsp::ClockRecoveryMMCC d_rec;
        void work();

    public:
        CCMMClockRecoveryBlock(dsp::stream<std::complex<float>> &input, float omega, float omegaGain, float mu, float muGain, float omegaLimit);
    };

    class FFMMClockRecoveryBlock : public Block<float, float>
    {
    private:
        dsp::ClockRecoveryMMFF d_rec;
        void work();

    public:
        FFMMClockRecoveryBlock(dsp::stream<float> &input, float omega, float omegaGain, float mu, float muGain, float omegaLimit);
    };
}