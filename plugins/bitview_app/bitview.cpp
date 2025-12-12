#include "bitview.h"
#include "bit_container.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/implot/implot.h"
#include "imgui/implot/implot_internal.h"
#include <exception>
#include <fcntl.h>
#include <memory>
#include <string>

#include "libs/muparser/muParser.h"
#include "libs/muparser/muParserError.h"
#include "logger.h"
#include "tools/ccsds_apid_demux/ccsds_apid_demux.h"
#include "tools/ccsds_vcid_splitter/ccsds_vcid_splitter.h"
#include "tools/deframer/deframer.h"
#include "tools/diff_decode/diff_decode.h"
#include "tools/soft2hard/soft2hard.h"
#include "tools/take_skip/take_skip.h"

namespace satdump
{
    BitViewHandler::BitViewHandler(std::shared_ptr<BitContainer> c) : bc(c)
    {
        c->bitview = this;
        handler_tree_icon = u8"\uf471";

        all_tools.push_back(std::make_shared<TakeSkipTool>());
        all_tools.push_back(std::make_shared<DeframerTool>());
        all_tools.push_back(std::make_shared<DifferentialTool>());
        all_tools.push_back(std::make_shared<Soft2HardTool>());
        all_tools.push_back(std::make_shared<CCSDSVcidSplitterTool>());
        all_tools.push_back(std::make_shared<CCSDSAPIDDemuxTool>());

        frame_width_exp = std::to_string(bc->d_bitperiod);
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

                if (ImGui::InputText("Period (Bits)", &frame_width_exp))
                {
                    try
                    {
                        mu::Parser equParser;
                        equParser.SetExpr(frame_width_exp);
                        int nout = 0;
                        double *out = equParser.Eval(nout);

                        if (nout == 1)
                        {
                            bc->d_bitperiod = *out;
                            bc->init_display();
                        }
                        else
                        {
                            logger->error("Error parsing bit period! Expression must have 1 output");
                        }
                    }
                    catch (mu::ParserError &)
                    {
                    }
                }
            }

            if (ImGui::RadioButton("Bits", !custom_bit_depth && bc->d_display_bits == 1))
            {
                custom_bit_depth = false;
                bc->d_display_bits = 1;
                bc->init_display();
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Bytes", !custom_bit_depth && bc->d_display_bits == 8))
            {
                custom_bit_depth = false;
                bc->d_display_bits = 8;
                bc->init_display();
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Custom", custom_bit_depth))
            {
                custom_bit_depth = true;
                bc->d_display_bits = 10;
                bc->init_display();
            }

            if (custom_bit_depth)
            {
                if (ImGui::InputInt("Bit Depth", &bc->d_display_bits))
                {
                    if (bc->d_display_bits > 32)
                        bc->d_display_bits = 32;
                    if (bc->d_display_bits < 1)
                        bc->d_display_bits = 1;
                    bc->init_display();
                }
            }

            if (!bc->d_is_temporary)
            {
                if (ImGui::Button("Reload"))
                {
                    auto bbc = bc;
                    bc = std::make_shared<BitContainer>(bbc->getName(), bbc->getFilePath(), bbc->frames);
                    bc->d_bitperiod = bbc->d_bitperiod;
                    bc->init_display();
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
                ImGui::PushID((tool->getName()).c_str());
                tool->renderMenu(bc, is_busy);
                ImGui::PopID();

                if (tool->needToProcess())
                {
                    tool->setProcessed();
                    auto func = [this, tool]()
                    {
                        try
                        {
                            tool->process(bc, process_progress);
                        }
                        catch (std::exception &e)
                        {
                            logger->error("Error running tool! %s", e.what());
                        }
                        is_busy = false;
                    };
                    is_busy = true;
                    process_task.push(func);
                }
            }
        }

        ImGui::Spacing();

        ImGui::ProgressBar(process_progress);

        ImGui::Separator();

        if (ImGui::Button("Find Sync"))
        {
            auto func = [this]()
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
            process_task.push(func);
        }
    }

    void BitViewHandler::drawMenuBar() {}

    void BitViewHandler::drawContextMenu()
    {
        // Delete all subhandlers quickly
        if (subhandlers.size() && ImGui::MenuItem("Clear Subhandlers"))
        {
            for (auto &h : subhandlers)
                delSubHandler(h);
        }
    }

    void BitViewHandler::drawContents(ImVec2 win_size)
    {
        ImVec2 window_size = win_size;

        bc->doUpdateTextures();

        ImPlot::BeginPlot("MainPlot", window_size, ImPlotFlags_Equal | ImPlotFlags_NoLegend);
        ImPlot::SetupAxes(nullptr, nullptr, 0, ImPlotAxisFlags_Invert);

        ImPlotRect c = ImPlot::GetPlotLimits();
        bc->doDrawPlotTextures(c);

        // ImPlot::GetPlotDrawList()->AddRectFilled(ImPlot::PlotToPixels({0, 0}), ImPlot::PlotToPixels({1, 1}), ImColor(255, 0, 0, 255 * 0.5));

        if (reset_view)
        {
            ImPlot::GetCurrentPlot()->XAxis(0).SetRange(ImPlotRange(0, 512));
            ImPlot::GetCurrentPlot()->YAxis(0).SetRange(ImPlotRange(0, 512));
            reset_view = false;
        }

        ImPlot::EndPlot();

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            reset_view = true;
    }
} // namespace satdump
