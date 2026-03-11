#pragma once

/**
 * @file block_helpers.h
 * @brief Block helpers (mostly IO)
 */

#include "block.h"
#include "common/dsp/complex.h"
#include "imgui/imgui.h"
#include <memory>

namespace satdump
{
    namespace ndsp
    {
        /**
         * @brief Event to request a block that's not accessible
         * via headers.
         * @param id block string ID
         * @param blk returned blocks
         */
        struct RequestBlockEvent
        {
            std::string id;
            std::vector<std::shared_ptr<Block>> &blk;
        };

        /**
         * @brief Get Block IO short name from given C type
         * @param T C type
         * @return Block IO short name
         */
        template <typename T>
        std::string getShortTypeName()
        {
            if (std::is_same_v<T, complex_t>)
                return "c";
            else if (std::is_same_v<T, float>)
                return "f";
            else if (std::is_same_v<T, int16_t>)
                return "s";
            else if (std::is_same_v<T, int8_t>)
                return "h";
            else if (std::is_same_v<T, uint8_t>)
                return "b";
            else
                throw satdump_exception("Invalid type for DSP blocks!");
        }

        /**
         * @brief Get Block IO type from given C type
         * @param T C type
         * @return Block IO type
         */
        template <typename T>
        BlockIOType getTypeSampleType()
        {
            if (std::is_same_v<T, complex_t>)
                return DSP_SAMPLE_TYPE_CF32;
            else if (std::is_same_v<T, float>)
                return DSP_SAMPLE_TYPE_F32;
            else if (std::is_same_v<T, int16_t>)
                return DSP_SAMPLE_TYPE_S16;
            else if (std::is_same_v<T, int8_t>)
                return DSP_SAMPLE_TYPE_S8;
            else if (std::is_same_v<T, uint8_t>)
                return DSP_SAMPLE_TYPE_U8;
            else
                throw satdump_exception("Invalid type for DSP blocks!");
        }

        /**
         * @brief Get color for a given DSP type
         * @param t IO type
         * @return matching color
         */
        inline ImColor getColorFromDSPType(BlockIOType t)
        {
            std::map<BlockIOType, ImColor> iotypes_colors = {
                {DSP_SAMPLE_TYPE_CF32, ImColor(5, 150, 255)}, //
                {DSP_SAMPLE_TYPE_F32, ImColor(252, 116, 0)},  //
                {DSP_SAMPLE_TYPE_S16, ImColor(244, 240, 5)},  //
                {DSP_SAMPLE_TYPE_S8, ImColor(200, 5, 252)},   //
                {DSP_SAMPLE_TYPE_U8, ImColor(5, 252, 87)},    //
            };

            if (iotypes_colors.count(t))
                return iotypes_colors[t];
            else
                return ImColor(255, 0, 0);
        }
    } // namespace ndsp
} // namespace satdump