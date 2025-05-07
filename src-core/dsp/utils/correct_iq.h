#pragma once

/**
 * @file correct_iq.h
 */

#include "dsp/block_simple.h"
#include <cstdint>

/**
 * @brief Simple Correct IQ block (removes a DC spike).
 *
 * This is the simplest implementation I thought of.
 * A DC component will be a constant offset in the raw samples,
 * so correcting for it is as simple as getting the average value
 * (which in a normal situation should be close to 0), and substract it.
 */

namespace satdump
{
    namespace ndsp
    {
        /** @brief Just remove a DC spike with a moving average
         * respecting each stream type
         * @param alpha
         * @param beta
         * @param acc
         */
        template <typename T>
        class CorrectIQBlock : public BlockSimple<T, T>
        {
        private:
            float alpha = 0.0001;
            float beta = 0;
            T acc = 0;

        public:
            CorrectIQBlock();
            ~CorrectIQBlock();

            uint32_t process(T *input, uint32_t nsamples, T *output);

            void init() {}

            nlohmann::json get_cfg(std::string key)
            {
                throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
