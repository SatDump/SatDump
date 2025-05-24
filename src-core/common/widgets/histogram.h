#pragma once

#include "common/dsp/complex.h"
#include "common/dsp/utils/random.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/implot/implot.h"
#include <mutex>

namespace widgets
{
#define HIST_SIZE 2048

    class HistoViewer
    {
    private:
        complex_t sample_buffer_complex_float[HIST_SIZE];
        const int d_histo_size;
        dsp::Random rng;
        std::mutex mtx;

    public:
        float d_hscale, d_vscale;

    public:
        HistoViewer(float hscale = 1, float vscale = 1, int histo_size = 200);
        ~HistoViewer();
        void pushComplex(complex_t *buffer, int size);
        void pushFloatAndGaussian(float *buffer, int size);
        void draw();
    };
} // namespace widgets
