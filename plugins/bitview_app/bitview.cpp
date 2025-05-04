#include "bitview.h"
#include "imgui/imgui_image.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/implot/implot.h"
#include <fstream>
#include "core/style.h"

#include <fcntl.h>
#include "common/utils.h"
#include <filesystem>

#include "logger.h"

//////
#include "tools/deframer.h"
#include "tools/diff_decode.h"
#include "tools/soft2hard.h"
#include "tools/ccsds_vcid_splitter.h"

namespace satdump
{
    // TODOREWORK
    BitViewHandler::BitViewHandler(std::shared_ptr<BitContainer> c)
        : current_bit_container(c)
    {
        c->bitview = this;
        handler_tree_icon = u8"\uf471";

        all_tools.push_back(std::make_shared<DeframerTool>());
        all_tools.push_back(std::make_shared<DifferentialTool>());
        all_tools.push_back(std::make_shared<Soft2HardTool>());
        all_tools.push_back(std::make_shared<CCSDSVcidSplitterTool>());
    }

    BitViewHandler::BitViewHandler()
    {
        handler_tree_icon = u8"\uf471";

        all_tools.push_back(std::make_shared<DeframerTool>());
        all_tools.push_back(std::make_shared<DifferentialTool>());
        all_tools.push_back(std::make_shared<Soft2HardTool>());
        all_tools.push_back(std::make_shared<CCSDSVcidSplitterTool>());
    }

    BitViewHandler::~BitViewHandler()
    {
    }

    void BitViewHandler::drawMenu()
    {
        if (is_busy)
            style::beginDisabled();
        if (ImGui::CollapsingHeader("Files##bitview", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Load File :");
            if (select_bitfile_dialog.draw() && select_bitfile_dialog.isValid())
            {
                try
                {
                    current_bit_container = std::make_shared<BitContainer>(std::filesystem::path(select_bitfile_dialog.getPath()).stem().string() +
                                                                               std::filesystem::path(select_bitfile_dialog.getPath()).stem().extension().string(),
                                                                           select_bitfile_dialog.getPath());
                    current_bit_container->bitview = this;
                }
                catch (std::exception &e)
                {
                    logger->error("Could not load file: %s", e.what());
                }
            }

            ImGui::Separator();
        }
        if (ImGui::CollapsingHeader("Control"))
        {
            if (current_bit_container)
            {
                double vv = current_bit_container->d_bitperiod;
                if (ImGui::InputDouble("Period", &vv))
                {
                    current_bit_container->d_bitperiod = vv;
                    current_bit_container->init_bitperiod();
                    current_bit_container->forceUpdateAll();
                }

                if (ImGui::RadioButton("Bits", current_bit_container->d_display_mode == 0))
                {
                    current_bit_container->d_display_mode = 0;
                    current_bit_container->forceUpdateAll();
                }
                if (ImGui::RadioButton("Bytes", current_bit_container->d_display_mode == 1))
                {
                    current_bit_container->d_display_mode = 1;
                    current_bit_container->forceUpdateAll();
                }
            }
        }
        if (is_busy)
            style::endDisabled();
        if (ImGui::CollapsingHeader("Tools"))
        {
            if (current_bit_container)
            {
                for (auto &tool : all_tools)
                {
                    ImGui::Separator();
                    ImGui::Text("%s", tool->getName().c_str());
                    ImGui::Separator();

                    tool->renderMenu(current_bit_container, is_busy);

                    if (tool->needToProcess())
                    {
                        tool->setProcessed();
                        auto func = [this, tool](int)
                        {
                            tool->process(current_bit_container, process_progress);
                            is_busy = false;
                        };
                        is_busy = true;
                        process_threadp.push(func);
                    }
                }

                ImGui::Spacing();

                ImGui::ProgressBar(process_progress);
            }
        }
    }

    void BitViewHandler::drawMenuBar()
    {
    }

    void BitViewHandler::drawContents(ImVec2 win_size)
    {
        ImVec2 window_size = win_size;

        if (current_bit_container)
            current_bit_container->doUpdateTextures();

        ImPlot::BeginPlot("MainPlot", window_size, ImPlotFlags_Equal | ImPlotFlags_NoLegend);
        ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_Invert);

        ImPlotRect c = ImPlot::GetPlotLimits();
        if (current_bit_container)
            current_bit_container->doDrawPlotTextures(c);

        // ImPlot::GetPlotDrawList()->AddRectFilled(ImPlot::PlotToPixels({0, 0}), ImPlot::PlotToPixels({1, 1}), ImColor(255, 0, 0, 255 * 0.5));

        ImPlot::EndPlot();
    }
}
