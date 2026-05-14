#pragma once

#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include "dsp/resampling/rational_resampler.h"
#include "dsp/utils/vco.h"
#include "nlohmann/json.hpp"
#include <string>

namespace satdump
{
    namespace ndsp
    {
        class GFSKModHierBlock : public Block
        {
        private:
            RationalResamplerBlock<float> fir_blk;
            VCOBlock vco_blk;

        private:
            float sensitivity = M_PI / 4.0;
            float alpha = 0.35;
            int ntaps = 31;
            int interpolation = 2;

        public:
            GFSKModHierBlock();
            ~GFSKModHierBlock();

            std::vector<BlockIO> get_inputs() { return fir_blk.get_inputs(); }
            std::vector<BlockIO> get_outputs() { return vco_blk.get_outputs(); }
            void set_input(BlockIO f, int i) { fir_blk.set_input(f, i); }
            BlockIO get_output(int i, int nbuf) { return vco_blk.get_output(i, nbuf); }

            void init()
            {
                auto taps = dsp::firdes::convolve(dsp::firdes::gaussian(1, interpolation, alpha, ntaps), {1, 1});
                fir_blk.set_cfg("taps", taps);
                fir_blk.set_cfg("interpolation", interpolation);
                fir_blk.set_cfg("decimation", 1);

                vco_blk.set_cfg("k", sensitivity);
                vco_blk.set_cfg("amp", 1.0);
            }

            void start()
            {
                init();

                vco_blk.link(&fir_blk, 0, 0, 4);

                fir_blk.start();
                vco_blk.start();
            }

            void stop(bool stop_snow, bool force)
            {
                fir_blk.stop(stop_snow);
                vco_blk.stop(stop_snow);
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json v;

                add_param_simple(v, "sensitivity", "float");
                add_param_simple(v, "alpha", "float");
                add_param_simple(v, "ntaps", "int");
                add_param_simple(v, "interpolation", "int");

                return v;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "sensitivity")
                    return sensitivity;
                else if (key == "alpha")
                    return alpha;
                else if (key == "ntaps")
                    return ntaps;
                else if (key == "interpolation")
                    return interpolation;
                else
                    return nlohmann::ordered_json();
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "sensitivity")
                {
                    sensitivity = v;
                    init();
                    return RES_OK;
                }
                else if (key == "alpha" || key == "ntaps" || key == "interpolation")
                {
                    if (key == "alpha")
                        alpha = v;
                    if (key == "ntaps")
                        ntaps = v;
                    if (key == "interpolation")
                        interpolation = v;
                    init();
                    return RES_OK;
                }
                else
                {
                    return RES_ERR;
                }
            }

            bool work() { return false; }
        };
    } // namespace ndsp
} // namespace satdump