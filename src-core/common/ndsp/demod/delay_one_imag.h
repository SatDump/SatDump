#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    class DelayOneImag : public ndsp::Block
    {
    private:
        void work();
        float lastSamp;

    public:
        // Nothing

    public:
        DelayOneImag();

        void set_params(nlohmann::json p = {});
        void start();
    };
}