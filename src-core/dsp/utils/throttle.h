#pragma once

#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {

        template <typename T>
        class ThrottleBlock : public BlockSimple<T, T>
        {
        private:
            bool needs_reinit = true;
            double samplerate = 1e6;

            std::chrono::steady_clock::time_point start_time_point;
            std::chrono::duration<double> sample_time_period;
            unsigned long long total_samples = 0;

        public:
            ThrottleBlock();
            ~ThrottleBlock();

            uint32_t process(T *input, uint32_t nsamples, T *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samplerate", "float", "Samplerate");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate")
                {
                    samplerate = v;
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
