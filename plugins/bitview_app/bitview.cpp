#include "bitview.h"
#include "bit_container.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/implot/implot.h"
#include <fcntl.h>
#include <memory>

#include "tools/ccsds_apid_demux.h"
#include "tools/ccsds_vcid_splitter.h"
#include "tools/deframer.h"
#include "tools/diff_decode.h"
#include "tools/soft2hard.h"

namespace satdump
{
    BitViewHandler::BitViewHandler(std::shared_ptr<BitContainer> c) : bc(c)
    {
        c->bitview = this;
        handler_tree_icon = u8"\uf471";

        all_tools.push_back(std::make_shared<DeframerTool>());
        all_tools.push_back(std::make_shared<DifferentialTool>());
        all_tools.push_back(std::make_shared<Soft2HardTool>());
        all_tools.push_back(std::make_shared<CCSDSVcidSplitterTool>());
        all_tools.push_back(std::make_shared<CCSDSAPIDDemuxTool>());
    }

    BitViewHandler::~BitViewHandler() {}

    void BitViewHandler::drawMenu()
    {
        if (is_busy)
            style::beginDisabled();

        if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (!bc->d_frame_mode)
            {
                double vv = bc->d_bitperiod;
                if (ImGui::InputDouble("Period (Bits)", &vv))
                {
                    bc->d_bitperiod = vv;
                    bc->init_bitperiod();
                    bc->forceUpdateAll();
                }
            }

            if (ImGui::RadioButton("Bits", bc->d_display_mode == 0))
            {
                bc->d_display_mode = 0;
                bc->forceUpdateAll();
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Bytes", bc->d_display_mode == 1))
            {
                bc->d_display_mode = 1;
                bc->forceUpdateAll();
            }

            if (!bc->d_is_temporary)
            {
                if (ImGui::Button("Reload"))
                {
                    auto bbc = bc;
                    bc = std::make_shared<BitContainer>(bbc->getName(), bbc->getFilePath(), bbc->frames);
                    bc->d_bitperiod = bbc->d_bitperiod;
                    bc->init_bitperiod();
                    bc->bitview = this;
                }
            }
        }

        if (is_busy)
            style::endDisabled();

        for (auto &tool : all_tools)
        {
            if (ImGui::CollapsingHeader((tool->getName()).c_str()))
            {

                tool->renderMenu(bc, is_busy);

                if (tool->needToProcess())
                {
                    tool->setProcessed();
                    auto func = [this, tool](int)
                    {
                        tool->process(bc, process_progress);
                        is_busy = false;
                    };
                    is_busy = true;
                    process_threadp.push(func);
                }
            }
        }

        ImGui::Spacing();

        ImGui::ProgressBar(process_progress);

        ImGui::Separator();

        if (ImGui::Button("Find Sync"))
        {
            auto func = [this](int)
            {
                auto ptr = bc->get_ptr();
                auto sz = bc->get_ptr_size();

                bc->highlights.clear();

                for (int i = 0; i < (sz / 8) - 4; i++)
                {
                    if (ptr[i + 0] == 0x1a && ptr[i + 1] == 0xcf && ptr[i + 2] == 0xfc && ptr[i + 3] == 0x1d)
                        bc->highlights.push_back({(size_t)i * 8, 64, 0, 0, 255});
                }

                is_busy = false;
            };
            is_busy = true;
            process_threadp.push(func);
        }
    }

    void BitViewHandler::drawMenuBar() {}

    void BitViewHandler::drawContents(ImVec2 win_size)
    {
        ImVec2 window_size = win_size;

        bc->doUpdateTextures();

        ImPlot::BeginPlot("MainPlot", window_size, ImPlotFlags_Equal | ImPlotFlags_NoLegend);
        ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_Invert);

        ImPlotRect c = ImPlot::GetPlotLimits();
        bc->doDrawPlotTextures(c);

        // ImPlot::GetPlotDrawList()->AddRectFilled(ImPlot::PlotToPixels({0, 0}), ImPlot::PlotToPixels({1, 1}), ImColor(255, 0, 0, 255 * 0.5));

        ImPlot::EndPlot();
    }
} // namespace satdump
