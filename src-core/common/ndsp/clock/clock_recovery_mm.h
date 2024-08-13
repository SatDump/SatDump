#pragma once

#include "common/dsp/complex.h"
#include "../block.h"
#include "common/dsp/resamp/polyphase_bank.h"

namespace ndsp
{
    template <typename T>
    class ClockRecoveryMM : public ndsp::Block
    {
    private:
        void work();

        // Buffer
        T *buffer = nullptr;

        // Variables
        float mu;
        float omega;
        float omega_gain;
        float mu_gain;
        float omega_relative_limit;
        float omega_mid;
        float omega_limit;

        float sample;
        float last_sample;

        complex_t p_2T, p_1T, p_0T;
        complex_t c_2T, c_1T, c_0T;

        dsp::PolyphaseBank pfb;

        float phase_error = 0;

        int ouc = 0;
        int inc = 0;

    public:
        int d_nfilt = 128;
        int d_ntaps = 8;
        float d_mu = -1;
        float d_omega = -1;
        float d_omega_gain = -1;
        float d_mu_gain = -1;
        float d_omega_relative_limit = -1;

    public:
        ClockRecoveryMM();
        ~ClockRecoveryMM();

        void set_params(nlohmann::json p = {});
        void start();
    };
}