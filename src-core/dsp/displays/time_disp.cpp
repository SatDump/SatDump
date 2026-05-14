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

            for (int i = 0; i < iblk.size; i++)
            {
                time_history_mtx.lock();
                time_history.erase(time_history.begin(), time_history.begin() + 1);
                time_history.push_back(samples[i]);
                time_history_mtx.unlock();
            }

            inputs[0].fifo->free(iblk);
            return false;
        }

        void TimeDisplayBlock::draw(ImVec2 size)
        {
            time_history_mtx.lock();
            if (ImPlot::BeginPlot("TimeSeries", size))
            {
                ImPlot::PlotScatter("Real", time_history.data(), time_history.size());
                ImPlot::EndPlot();
            }
            time_history_mtx.unlock();
        }
    } // namespace ndsp
} // namespace satdump
