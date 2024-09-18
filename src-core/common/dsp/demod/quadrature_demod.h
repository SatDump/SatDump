#pragma once

#include "common/dsp/block.h"

/*
Quadrature demodulator
*/
namespace dsp
{
    class QuadratureDemodBlock : public Block<complex_t, float>
    {
    private:
        float gain;
        // complex_t *buffer, *buffer_out;
        void work();

        float phase = 0;

    public:
        QuadratureDemodBlock(std::shared_ptr<dsp::stream<complex_t>> input, float gain);
        ~QuadratureDemodBlock();

        int process(complex_t *input, int nsamples, float *output);
        void set_gain(float gain);
    };
}