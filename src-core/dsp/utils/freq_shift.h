#pragma once

/**
 * @file freq_shift.h
 */

// TODOREWORK check whether or not needs to have a raw frequency shift like from "common/dsp/utils/freq_shift.h"

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief Frequency shifter WIP!!
         */
        class FreqShiftBlock : public BlockSimple<complex_t, complex_t>
        {
        public:
            double samplerate = 0;
            double freq_shift = 0;
            bool raw_shift = 0;

        private:
            complex_t phase_delta;
            complex_t phase;

        public:
            FreqShiftBlock();
            ~FreqShiftBlock();

            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "freq_shift", "freq", "Frequency Shift");
                add_param_simple(p, "raw_shift", "bool", "Raw Shift");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "freq_shift")
                    return freq_shift;
                else if (key == "raw_shift")
                    return raw_shift;
                else if (key == "samplerate")
                    return samplerate;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "freq_shift")
                    freq_shift = v;
                if (key == "raw_shift")
                    raw_shift = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
