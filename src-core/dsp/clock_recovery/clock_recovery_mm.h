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

            bool needs_reinit = false;

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

            nlohmann::json get_cfg_list()
            {
                nlohmann::json p;
                add_param(p, "omega", "float");
                add_param(p, "omegaGain", "float", pow(8.7e-3, 2) / 4.0);
                add_param(p, "mu", "float", 0.5f);
                add_param(p, "muGain", "float", 8.7e-3);
                add_param(p, "omegaLimit", "float", 0.005);
                add_param(p, "nfilt", "int", 128);
                add_param(p, "ntaps", "int", 8);
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "omega")
                    return p_omega;
                else if (key == "omegaGain")
                    return p_omegaGain;
                else if (key == "mu")
                    return p_mu;
                else if (key == "muGain")
                    return p_muGain;
                else if (key == "omegaLimit")
                    return p_omegaLimit;
                else if (key == "nfilt")
                    return p_nfilt;
                else if (key == "ntaps")
                    return p_ntaps;
                else
                    throw satdump_exception(key);
            }

            void set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "omega")
                {
                    p_omega = v;
                    needs_reinit = true;
                }
                else if (key == "omegaGain")
                {
                    p_omegaGain = v;
                    needs_reinit = true;
                }
                else if (key == "mu")
                {
                    p_mu = v;
                    needs_reinit = true;
                }
                else if (key == "muGain")
                {
                    p_muGain = v;
                    needs_reinit = true;
                }
                else if (key == "omegaLimit")
                {
                    p_omegaLimit = v;
                    needs_reinit = true;
                }
                else if (key == "nfilt")
                {
                    p_nfilt = v;
                    needs_reinit = true;
                }
                else if (key == "ntaps")
                {
                    p_ntaps = v;
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
            }
        };
    }
}