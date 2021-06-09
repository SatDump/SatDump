#pragma once

#include "imgui/imgui.h"
#include <complex>

namespace widgets
{
    const int CONST_SIZE = 2048;

    class ConstellationViewer
    {
    private:
        std::complex<float> sample_buffer_complex_float[CONST_SIZE];
        const float d_hscale, d_vscale;

    public:
        ConstellationViewer(float hscale = 1, float vscale = 1);
        ~ConstellationViewer();
        void pushComplex(std::complex<float> *buffer, int size);
        void draw();
    };
}