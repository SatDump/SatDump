#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include "dsp/complex_json.h"
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace ndsp
    {
        template <typename T, typename TT = float>
        class FIRBlock : public BlockSimple<T, T>
        {
        public:
            bool needs_reinit = false;
            std::vector<TT> p_taps;

        private:
            int buffer_size = 0;
            volk::vector<T> buffer;
            TT **taps = nullptr;
            int ntaps;
            int align;
            int aligned_tap_count;

            size_t in_buffer;

        public:
            uint32_t process(T *input, uint32_t nsamples, T *output);

        public:
            FIRBlock(std::string id = "");
            ~FIRBlock();

            void init();

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "taps")
                    return p_taps;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "taps")
                {
                    p_taps = v.get<std::vector<TT>>();
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump