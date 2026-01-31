#pragma once

#include "block.h"
#include "common/dsp/complex.h"
#include <memory>

namespace satdump
{
    namespace ndsp // TODOREWORK back to normal DSP!
    {
        struct RequestBlockEvent
        {
            std::string id;
            std::vector<std::shared_ptr<Block>> &blk;
        };

        // TODOREWORK cleanup!!!!
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
    } // namespace ndsp
} // namespace satdump