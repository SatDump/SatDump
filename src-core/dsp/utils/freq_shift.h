#pragma once

/**
 * @file freq_shift.h
 */

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
            float p_freq_shift = 0;

            bool needs_reinit = false;

        private:
            float freq_shift;

            complex_t phase_delta;
            complex_t phase;

        public:
            FreqShiftBlock();
            ~FreqShiftBlock();

            void set_freq(uint64_t samplerate, float freq_shift);
            void set_freq_raw(float freq_shift);


            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "freq_shift", "float", "Frequency Shift");
                // add_param_simple(p, "raw_shift", "bool", "Raw Shift");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "freq_shift")
                    return p_freq_shift;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "freq_shift")
                {
                    p_freq_shift = v;
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
