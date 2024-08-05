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
    BitViewApplication::BitViewApplication()
        : Application("bitview")
    {
        all_tools.push_back(std::make_shared<DeframerTool>());
        all_tools.push_back(std::make_shared<DifferentialTool>());
        all_tools.push_back(std::make_shared<Soft2HardTool>());
        all_tools.push_back(std::make_shared<CCSDSVcidSplitterTool>());
    }

    BitViewApplication::~BitViewApplication()
    {
    }

    void BitViewApplication::drawUI()
    {
        ImVec2 bitviewer_size = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTable("##bitviewer_table", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("##bitpanel_v", ImGuiTableColumnFlags_None, bitviewer_size.x * panel_ratio);
            ImGui::TableSetupColumn("##bitview", ImGuiTableColumnFlags_None, bitviewer_size.x * (1.0f - panel_ratio));
            ImGui::TableNextColumn();

            float left_width = ImGui::GetColumnWidth(0);
            float right_width = bitviewer_size.x - left_width;
            if (left_width != last_width && last_width != -1)
                panel_ratio = left_width / bitviewer_size.x;
            last_width = left_width;

            ImGui::BeginChild("BitViewerChildPanel", {left_width, float(bitviewer_size.y - 10)});
            drawPanel();
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            drawContents();
            ImGui::EndGroup();
            ImGui::EndTable();
        }
    }

    void renderBitContainers(std::vector<std::shared_ptr<satdump::BitContainer>> &all_bit_containers, std::shared_ptr<satdump::BitContainer> &current_container)
    {
        for (int i = 0; i < all_bit_containers.size(); i++)
        {
            auto &cont = all_bit_containers[i];

            if (!cont)
                continue;

            ImGui::TreeNodeEx(cont->getName().c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (cont == current_container ? ImGuiTreeNodeFlags_Selected : 0));
            if (ImGui::IsItemClicked())
            {
                current_container = cont;
            }
            ImGui::TreePush(std::string("##BitViewerTree" + cont->getID()).c_str());

            bool del = false;
            { // Closing button
                ImGui::SameLine();
                ImGui::Text("  ");
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Text, style::theme.red.Value);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
                if (ImGui::SmallButton(std::string(u8"\uf00d##dataset" + cont->getName()).c_str()))
                {
                    logger->info("Closing bit container " + cont->getName());
                    del = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }

            renderBitContainers(cont->all_bit_containers, current_container);
            ImGui::TreePop();

            if (del)
                cont.reset();
        }
    }

    void BitViewApplication::drawPanel()
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
                    all_bit_containers.push_back(std::make_shared<BitContainer>(std::filesystem::path(select_bitfile_dialog.getPath()).stem().string(), select_bitfile_dialog.getPath()));
                }
                catch (std::exception &e)
                {
                    logger->error("Could not load file: %s", e.what());
                }
            }

            ImGui::Separator();

            renderBitContainers(all_bit_containers, current_bit_container);
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

    void BitViewApplication::drawContents()
    {
        ImVec2 window_size = ImGui::GetContentRegionAvail();

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
