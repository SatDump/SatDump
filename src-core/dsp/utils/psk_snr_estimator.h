#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "common/dsp/utils/snr_estimator.h"
#include "dsp/block.h"
#include <cstdint>
#include <cstdio>

namespace satdump
{
    namespace ndsp
    {
        class PSKSnrEstimatorBlock : public Block
        {

        private:
            M2M4SNREstimator estimator;

            bool work();

        public:
            PSKSnrEstimatorBlock();
            ~PSKSnrEstimatorBlock();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "snr", "stat");
                add_param_simple(p, "noise", "stat");
                add_param_simple(p, "signal", "stat");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "snr")
                    return estimator.snr();
                else if (key == "noise")
                    return estimator.noise();
                else if (key == "signal")
                    return estimator.signal();
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {

                throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump