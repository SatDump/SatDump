#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class AGCBlock : public Block
        {
        public:
            float p_rate = 1e-4;
            float p_reference = 1.0;
            float p_gain = 1.0;
            float p_max_gain = 65536;

        private:
            float rate;      // adjustment rate
            float reference; // reference value
            float gain;      // current gain
            float max_gain;  // max allowable gain

            bool work();

        public:
            AGCBlock();
            ~AGCBlock();

            void init()
            {
                rate = p_rate;
                reference = p_reference;
                gain = p_gain;
                max_gain = p_max_gain;
            }

            nlohmann::json getParams()
            {
                nlohmann::json v;
                v["rate"] = p_rate;
                v["reference"] = p_reference;
                v["gain"] = p_gain;
                v["max_gain"] = p_max_gain;
                return v;
            }

            void setParams(nlohmann::json v)
            {
                setValFromJSONIfExists(p_rate, v["rate"]);
                setValFromJSONIfExists(p_reference, v["reference"]);
                setValFromJSONIfExists(p_gain, v["gain"]);
                setValFromJSONIfExists(p_max_gain, v["max_gain"]);
            }
        };
    }
}