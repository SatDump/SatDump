#pragma once

/**
 * @file freq_shift.h
 */

#include "common/dsp/complex.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief Frequency Shift class
         *
         * This is a implementation of the old block
         * using the new NDSP system.
         *
         * @param needs_reinit handles live changes
         * @param raw_shift if true, allows for manual frequency shift
         * @param freq_shift frequency shift value
         * @param samplerate stream sample rate
         */
        class FreqShiftBlock : public Block
        {
        private:
            bool needs_reinit = false;
            bool raw_shift = false;
            double freq_shift = 0;
            double samplerate = 6e6;

        private:
            complex_t phase_delta;
            complex_t phase;

            bool work();

        public:
            FreqShiftBlock();
            ~FreqShiftBlock();

            void init()
            {
                if (raw_shift == false)
                {
                    phase = complex_t(1, 0);
                    phase_delta = complex_t(cos(2.0 * M_PI * (freq_shift / samplerate)), sin(2.0 * M_PI * (freq_shift / samplerate)));
                }
                else if (raw_shift == true)
                {
                    phase_delta = complex_t(cosf(freq_shift), sinf(freq_shift));
                }
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "freq_shift", "float", "Frequency Shift");
                add_param_simple(p, "samplerate", "float", "Samplerate");
                add_param_simple(p, "raw_shift", "bool", "Raw Shift");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "raw_shift")
                    return raw_shift;
                else if (key == "freq_shift")
                    return freq_shift;
                else if (key == "samplerate")
                    return samplerate;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "raw_shift")
                {
                    raw_shift = v;
                    needs_reinit = true;
                }
                else if (key == "freq_shift")
                {
                    freq_shift = v;
                    needs_reinit = true;
                }
                else if (key == "samplerate")
                {
                    samplerate = v;
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
