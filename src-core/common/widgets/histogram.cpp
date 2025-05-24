#include "histogram.h"
#include "imgui/implot/implot.h"
#include <cstring>

namespace widgets
{
    HistoViewer::HistoViewer(float hscale, float vscale, int histo_size) : d_histo_size(histo_size), d_hscale(hscale), d_vscale(vscale) {}

    HistoViewer::~HistoViewer() {}

    void HistoViewer::pushComplex(complex_t *buffer, int size)
    {
        mtx.lock();
        int to_copy = std::min<int>(HIST_SIZE, size);
        int to_move = size >= HIST_SIZE ? 0 : HIST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(complex_t));

        std::memcpy(sample_buffer_complex_float, buffer, to_copy * sizeof(complex_t));
        mtx.unlock();
    }

    void HistoViewer::pushFloatAndGaussian(float *buffer, int size)
    {
        mtx.lock();
        int to_copy = std::min<int>(HIST_SIZE, size);
        int to_move = size >= HIST_SIZE ? 0 : HIST_SIZE - size;

        if (to_move > 0)
            std::memmove(&sample_buffer_complex_float[size], sample_buffer_complex_float, to_move * sizeof(complex_t));

        for (int i = 0; i < to_copy; i++)
            sample_buffer_complex_float[i] = complex_t(buffer[i], rng.gasdev());
        mtx.unlock();
    }

    void HistoViewer::draw()
    {
        mtx.lock();

        ImPlotHistogramFlags hist_flags = ImPlotHistogramFlags_Density;
        int bins = 200;

        double realVal[HIST_SIZE];
        double imagVal[HIST_SIZE];

        for (int i = 0; i < HIST_SIZE; i++)
        {
            realVal[i] = sample_buffer_complex_float[i].real;
            imagVal[i] = sample_buffer_complex_float[i].imag;
        }
        // ImPlotLocation loc = ImPlotLocation_South;
        // ImPlotLegendFlags leg = ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal;
        ImPlot::BeginPlot("##Histogram", ImVec2(d_histo_size, d_histo_size), ImPlotFlags_NoTitle | ImPlotFlags_NoFrame | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoInputs);
        // ImPlot::SetupLegend(loc, leg);
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(-2.0, 2.0, 0, 2.0);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        ImPlot::PlotHistogram("Real", realVal, HIST_SIZE, bins, 1.0, ImPlotRange(-2.0f, 2.0f), hist_flags);
        ImPlot::PlotHistogram("Imag", imagVal, HIST_SIZE, bins, 1.0, ImPlotRange(-2.0f, 2.0f), hist_flags);
        ImPlot::EndPlot();
        mtx.unlock();
    }

} // namespace widgets
