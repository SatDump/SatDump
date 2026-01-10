#include "deinterleave.h"
#include "common/codings/differential/nrzm.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include <cstdint>
#include <fstream>

namespace satdump
{
    std::string DeinterleaveTool::getName() { return "Deinterleave"; }

    void DeinterleaveTool::renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
    {
        if (is_busy)
            style::beginDisabled();

        ImGui::InputInt("Depth", &idepth);
        ImGui::InputInt("Cnt", &icnt);

        if (ImGui::Button("Deinterleave"))
            should_process = true;

        if (is_busy)
            style::endDisabled();
    }

    bool DeinterleaveTool::needToProcess() { return should_process; }

    void DeinterleaveTool::setProcessed() { should_process = false; }

    void DeinterleaveTool::process(std::shared_ptr<BitContainer> &container, float &process_progress)
    {
        uint8_t *ptr = container->get_ptr();
        size_t size = container->get_ptr_size();

        struct ChOut
        {
            ChOut(std::string f) : tmpfile(f), fileout(f, std::ios::binary) {}

            std::string tmpfile;
            std::ofstream fileout;
            std::vector<BitContainer::FrameDef> frms;
            size_t pos = 0;

            uint8_t shifter;
            int in_shifter = 0;
        };
        std::map<int, std::shared_ptr<ChOut>> chs_outs;

        for (int i = 0; i < idepth; i++)
        {
            char name[1000];
            tmpnam(name);
            std::string tmpfile = name;
            chs_outs.emplace(i, std::make_shared<ChOut>(tmpfile));
        }

        {
            for (size_t i = 0, ii = 0; i < size * 8; i++)
            {
                auto &c = chs_outs[ii];

                c->shifter = c->shifter << 1 | ((ptr[i / 8] >> (7 - (i % 8))) & 1);
                c->in_shifter++;
                if (c->in_shifter == 8)
                {
                    c->fileout.write((char *)&c->shifter, 1);
                    c->in_shifter = 0;
                }

                process_progress = (float)i / float(size * 8);

                ii++;
                if (ii >= idepth)
                    ii = 0;
            }

            for (int i = 0; i < idepth; i++)
            {
                auto &c = chs_outs[i];
                if (c->in_shifter > 0)
                    c->fileout.write((char *)&c->shifter, 1);
            }
        }

        for (int i = 0; i < idepth; i++)
        {
            auto &c = chs_outs[i];

            c->fileout.close();
            std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>("Deinterleaved " + std::to_string(i) + "", c->tmpfile, container->frames);
            newbitc->d_display_bits = container->d_display_bits;
            newbitc->d_bitperiod = container->d_bitperiod;
            newbitc->init_display();
            newbitc->d_is_temporary = true;

            if (container->bitview != nullptr)
                ((BitViewHandler *)container->bitview)->addSubHandler(std::make_shared<BitViewHandler>(newbitc));
            else
                logger->error("Can't add container!");
        }
    }
} // namespace satdump