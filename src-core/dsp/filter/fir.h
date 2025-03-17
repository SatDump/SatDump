#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class FIRBlock : public Block
        {
        public:
            std::vector<float> p_taps;

        private:
            T *buffer = nullptr;
            float **taps = nullptr;
            int ntaps;
            int align;
            int aligned_tap_count;

            bool work();

        public:
            FIRBlock();
            ~FIRBlock();

            void init()
            {
                // Free if needed
                if (taps != nullptr)
                {
                    for (int i = 0; i < aligned_tap_count; i++)
                        volk_free(taps[i]);
                    volk_free(taps);
                }

                // Get alignement parameters
                align = volk_get_alignment();
                aligned_tap_count = std::max<int>(1, align / sizeof(T));

                // Set taps
                ntaps = p_taps.size();

                // Init taps
                this->taps = (float **)volk_malloc(aligned_tap_count * sizeof(float *), align);
                for (int i = 0; i < aligned_tap_count; i++)
                {
                    this->taps[i] = (float *)volk_malloc((ntaps + aligned_tap_count - 1) * sizeof(float), align);
                    for (int y = 0; y < ntaps + aligned_tap_count - 1; y++)
                        this->taps[i][y] = 0;
                    for (int j = 0; j < ntaps; j++)
                        this->taps[i][i + j] = p_taps[(ntaps - 1) - j]; // Reverse taps
                }
            }

            nlohmann::json get_cfg()
            {
                nlohmann::json v;
                v["taps"] = p_taps;
                return v;
            }

            void set_cfg(nlohmann::json v)
            {
                p_taps = v["taps"].get<std::vector<float>>();
            }
        };
    }
}