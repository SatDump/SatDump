#pragma once

#include "imgui/imgui.h"
#include "common/dsp/complex.h"
#include "common/dsp/utils/random.h"

namespace widgets
{
#define CONST_SIZE 2048

    class ConstellationViewer
    {
    private:
        complex_t sample_buffer_complex_float[CONST_SIZE];
        const int d_constellation_size;
        dsp::Random rng;

    public:
        float d_hscale, d_vscale;

    public:
        ConstellationViewer(float hscale = 1, float vscale = 1, int constellation_size = 200);
        ~ConstellationViewer();
        void pushComplexScaled(complex_t *buffer, int size, float scale);
        void pushComplex(complex_t *buffer, int size);
        void pushFloatAndGaussian(float *buffer, int size);
        void pushSofttAndGaussian(int8_t *buffer, float scale, int size);
        void draw();
    };
}