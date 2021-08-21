#pragma once

#include "imgui/imgui.h"
#include "complex"

namespace widgets
{
    class SNRPlotViewer
    {
    private:
        float snr_history[200];

    public:
        void draw(float snr = 1, float peak_snr = 1);
    };
}