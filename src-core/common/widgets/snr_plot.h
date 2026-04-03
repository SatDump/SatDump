#pragma once

#include "complex"
#include "imgui/imgui.h"

namespace satdump
{
    namespace widgets
    {
        class SNRPlotViewer
        {
        private:
            float snr_history[200];

        public:
            void draw(float snr = 1, float peak_snr = 1);
        };
    } // namespace widgets
} // namespace satdump