#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class SimpleGardnerRecoveryBlock : public BlockSimple<complex_t, complex_t>
        {
        private:
            float period = 4;
            float alpha = 0.02f;

            complex_t last_sample = 0.0f;

            // Gardner stuff, since we need a midpoint
            complex_t prev_sym = 0.0f;
            complex_t mid = 0.0f;
            bool is_mid = true;

            // Samples until next half-symbol
            float phase = 0.0f;

        private:
            bool clock_recovery(complex_t sample, complex_t *sym);

        public:
            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

        public:
            SimpleGardnerRecoveryBlock();
            ~SimpleGardnerRecoveryBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "sps", "float");
                add_param_simple(p, "alpha", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "sps")
                    return period;
                else if (key == "alpha")
                    return alpha;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "sps")
                    period = v;
                else if (key == "alpha")
                    alpha = v;
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump