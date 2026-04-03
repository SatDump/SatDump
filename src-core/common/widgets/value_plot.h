#pragma once

#include "complex"
#include "imgui/imgui.h"

namespace satdump
{
    namespace widgets
    {
        class ValuePlotViewer
        {
        private:
            float history[200];

        public:
            void draw(float value, float max, float min, std::string name);
        };
    } // namespace widgets
} // namespace satdump