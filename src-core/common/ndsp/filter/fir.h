#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    template <typename T>
    class FIRFilter : public ndsp::Block
    {
    private:
        void work();

        T *buffer = nullptr;
        float **taps = nullptr;
        int ntaps;
        int align;
        int aligned_tap_count;

    public:
        std::vector<float> d_taps;

    public:
        FIRFilter();
        ~FIRFilter();

        void set_params(nlohmann::json p = {});
        void start();
    };
}