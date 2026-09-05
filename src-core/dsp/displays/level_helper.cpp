#include "dsp/displays/level_helper.h"
#include "imgui/imgui.h"
#include "imgui/implot/implot.h"
#include <algorithm>
#include <cmath>
#include <ctime>

namespace satdump
{
    namespace ndsp
    {
        LevelHelperDisplayBlock::LevelHelperDisplayBlock() : Block("level_helper_disp_f", {{"in", DSP_SAMPLE_TYPE_CF32}}, {}) {}

        LevelHelperDisplayBlock::~LevelHelperDisplayBlock() {}

        bool LevelHelperDisplayBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                inputs[0].fifo->free(iblk);
                return true;
            }

            complex_t *samples = iblk.getSamples<complex_t>();

            for (int i = 0; i < iblk.size; i++)
            {
                level_history_mtx.lock();
                time_history.erase(time_history.begin(), time_history.begin() + 1);
                time_history.push_back(20 * log10(1 + samples[i].norm()));
                level_history_mtx.unlock();
            }

            inputs[0].fifo->free(iblk);
            return false;
        }

        void LevelHelperDisplayBlock::draw(ImVec2 size)
        {
            level_history_mtx.lock();
            double max_level = *std::max_element(time_history.begin(), time_history.end());
            float barval = max_level / 6.020599913279624;
            ImGui::ProgressBar(barval, ImVec2(200, 0));
            level_history_mtx.unlock();
        }
    } // namespace ndsp
} // namespace satdump
