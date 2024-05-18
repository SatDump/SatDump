#include "bitview.h"
#include "imgui/imgui_image.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/implot/implot.h"
#include <fstream>

#include <fcntl.h>
#include <sys/mman.h>
#include "common/utils.h"
#include <filesystem>

#include "logger.h"

//////
#include "common/simple_deframer.h"

namespace satdump
{
    BitViewApplication::BitViewApplication()
        : Application("bitview")
    {
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

            ImGui::TreeNodeEx(cont->getName().c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (cont == current_container ? ImGuiTreeNodeFlags_Selected : 0));
            if (ImGui::IsItemClicked())
            {
                current_container = cont;
            }
            ImGui::TreePush(std::string("##BitViewerTree" + cont->getName()).c_str());
            renderBitContainers(cont->all_bit_containers, current_container);
            ImGui::TreePop();
        }
    }

    void BitViewApplication::drawPanel()
    {
        if (ImGui::CollapsingHeader("Files##bitview"))
        {
            ImGui::Text("Load File :");
            if (select_bitfile_dialog.draw() && select_bitfile_dialog.isValid())
                all_bit_containers.push_back(std::make_shared<BitContainer>(std::filesystem::path(select_bitfile_dialog.getPath()).stem().string(), select_bitfile_dialog.getPath()));

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
            }
        }
        if (ImGui::CollapsingHeader("Deframer"))
        {
            if (current_bit_container)
            {
                ImGui::InputText("Syncword", &deframer_syncword);
                ImGui::InputInt("Syncword Size", &deframer_syncword_size);
                ImGui::InputInt("Frame Size (Bits)", &deframer_syncword_framesize);

                if (ImGui::Button("DeframeTest"))
                {
                    if (process_thread.joinable())
                        process_thread.join();

                    auto func = [this]()
                    {
                        uint8_t *ptr = current_bit_container->get_ptr();
                        size_t size = current_bit_container->get_ptr_size();

                        std::ofstream file_out("/tmp/testdef.bin", std::ios::binary);
                        def::SimpleDeframer def_test(0x1acffc1d, deframer_syncword_size, deframer_syncword_framesize, 0);

                        deframer_current_frames = 0;

                        size_t current_ptr = 0;
                        while (current_ptr < size)
                        {
                            size_t csize = std::min<size_t>(8192, size - current_ptr);
                            auto vf = def_test.work(ptr + current_ptr, csize);
                            current_ptr += csize;

                            for (auto &f : vf)
                                file_out.write((char *)f.data(), f.size());
                            deframer_current_frames += vf.size();

                            process_progress = double(current_ptr) / double(size);
                        }

                        file_out.close();

                        std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>(current_bit_container->getName() + " Deframed", "/tmp/testdef.bin");
                        newbitc->d_bitperiod = deframer_syncword_framesize;
                        newbitc->init_bitperiod();

                        current_bit_container->all_bit_containers.push_back(newbitc);
                    };
                    process_thread = std::thread(func);
                }

                ImGui::Text("Frames : %d", deframer_current_frames);
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
