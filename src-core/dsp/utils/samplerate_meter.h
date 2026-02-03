#pragma once

#include "dsp/block.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class SamplerateMeterBlock : public BlockSimple<T, T>
        {
        private:
            double last_timestamp = 0;
            double samplecount = 0;

        public:
            double measured_samplerate;

        public:
            SamplerateMeterBlock();
            ~SamplerateMeterBlock();

            uint32_t process(T *input, uint32_t nsamples, T *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                return p;
            }

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
