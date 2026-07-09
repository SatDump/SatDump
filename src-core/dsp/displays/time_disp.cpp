#include "time_disp.h"
#include "imgui/implot/implot.h"

namespace satdump
{
    namespace ndsp
    {
        TimeDisplayBlock::TimeDisplayBlock() : Block("time_disp_f", {{"in", DSP_SAMPLE_TYPE_F32}}, {}) {}

        TimeDisplayBlock::~TimeDisplayBlock() {}

        bool TimeDisplayBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            float *samples = iblk.getSamples<float>();

            if (iblk.size > 0)
            {
                std::lock_guard<std::mutex> lock(time_history_mtx);
                int history_size = time_history.size();
                if (history_size > 0)
                {
                    if (iblk.size >= history_size)
                    {
                        std::copy(samples + iblk.size - history_size, samples + iblk.size, time_history.begin());
                    }
                    else
                    {
                        std::copy(time_history.begin() + iblk.size, time_history.end(), time_history.begin());
                        std::copy(samples, samples + iblk.size, time_history.end() - iblk.size);
                    }
                }
            }

            inputs[0].fifo->free(iblk);
            return false;
        }

        void TimeDisplayBlock::draw(ImVec2 size)
        {
            std::lock_guard<std::mutex> lock(time_history_mtx);
            if (ImPlot::BeginPlot("TimeSeries", size))
            {
                ImPlot::PlotScatter("Real", time_history.data(), time_history.size());
                ImPlot::EndPlot();
            }
        }
    } // namespace ndsp
} // namespace satdump
