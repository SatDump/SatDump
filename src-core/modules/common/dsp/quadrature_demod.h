#pragma once

#include "block.h"
#include "lib/quadrature_demod.h"

namespace dsp
{
    class QuadratureDemodBlock : public Block<std::complex<float>, float>
    {
    private:
        dsp::QuadratureDemod d_quad;
        void work();

    public:
        QuadratureDemodBlock(std::shared_ptr<dsp::stream<std::complex<float>>> input, float gain);
    };
}