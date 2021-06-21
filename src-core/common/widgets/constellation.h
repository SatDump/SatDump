#pragma once

#include "imgui/imgui.h"
#include <complex>
#include "common/dsp/lib/random.h"

namespace widgets
{
    const int CONST_SIZE = 2048;

    class ConstellationViewer
    {
    private:
        std::complex<float> sample_buffer_complex_float[CONST_SIZE];
        const float d_hscale, d_vscale;
        const int d_constellation_size;
        dsp::Random rng;

    public:
        ConstellationViewer(float hscale = 1, float vscale = 1, int constellation_size = 200);
        ~ConstellationViewer();
        void pushComplex(std::complex<float> *buffer, int size);
        void pushFloatAndGaussian(float *buffer, int size);
        void draw();
    };
}