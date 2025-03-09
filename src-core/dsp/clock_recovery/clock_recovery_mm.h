#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"
#include "common/dsp/resamp/polyphase_bank.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class MMClockRecoveryBlock : public Block
        {
        public:
            float p_omega = 0;
            float p_omegaGain = pow(8.7e-3, 2) / 4.0;
            float p_mu = 0.5f;
            float p_muGain = 8.7e-3;
            float p_omegaLimit = 0.005;
            int p_nfilt = 128;
            int p_ntaps = 8;

        private:
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

            bool work();

        public:
            MMClockRecoveryBlock();
            ~MMClockRecoveryBlock();

            void init();

            nlohmann::json getParams()
            {
                nlohmann::json v;
                v["omega"] = p_omega;
                v["omegaGain"] = p_omegaGain;
                v["mu"] = p_mu;
                v["muGain"] = p_muGain;
                v["omegaLimit"] = p_omegaLimit;
                v["nfilt"] = p_nfilt;
                v["ntaps"] = p_ntaps;
                return v;
            }

            void setParams(nlohmann::json v)
            {
                setValFromJSONIfExists(p_omega, v["omega"]);
                setValFromJSONIfExists(p_omegaGain, v["omegaGain"]);
                setValFromJSONIfExists(p_mu, v["p_mu"]);
                setValFromJSONIfExists(p_muGain, v["muGain"]);
                setValFromJSONIfExists(p_omegaLimit, v["omegaLimit"]);
                setValFromJSONIfExists(p_nfilt, v["nfilt"]);
                setValFromJSONIfExists(p_ntaps, v["ntaps"]);
            }
        };
    }
}