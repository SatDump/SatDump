#include "take_skip.h"
#include "common/codings/differential/nrzm.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include <cstdint>
#include <fstream>

namespace satdump
{
    std::string TakeSkipTool::getName() { return "Take Skip"; }

    void TakeSkipTool::renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
    {
        if (is_busy)
            style::beginDisabled();

        ImGui::InputInt("Skip Bits", &skip_bits);

        if (ImGui::Button("Skip"))
            should_process = true;

        if (is_busy)
            style::endDisabled();
    }

    bool TakeSkipTool::needToProcess() { return should_process; }

    void TakeSkipTool::setProcessed() { should_process = false; }

    void TakeSkipTool::process(std::shared_ptr<BitContainer> &container, float &process_progress)
    {
        uint8_t *ptr = container->get_ptr();
        size_t size = container->get_ptr_size();

        char name[1000];
        tmpnam(name);
        std::string tmpfile = name;
        std::ofstream file_out(tmpfile, std::ios::binary);

        // if (container->d_frame_mode) {
        // TODOREWORK?
        // }
        // else
        {
            uint8_t shifter = 0;
            int in_shifter = 0;
            for (size_t i = skip_bits; i < size * 8; i++)
            {
                shifter = shifter << 1 | ((ptr[i / 8] >> (7 - (i % 8))) & 1);
                in_shifter++;
                if (in_shifter == 8)
                {
                    file_out.write((char *)&shifter, 1);
                    in_shifter = 0;
                }

                process_progress = (float)i / float(size * 8);
            }

            if (in_shifter > 0)
                file_out.write((char *)&shifter, 1);
        }

        file_out.close();
        std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>("Skipped " + std::to_string(skip_bits) + " Bits", tmpfile, container->frames);
        newbitc->d_display_bits = container->d_display_bits;
        newbitc->d_bitperiod = container->d_bitperiod;
        newbitc->init_display();
        newbitc->d_is_temporary = true;

        if (container->bitview != nullptr)
            ((BitViewHandler *)container->bitview)->addSubHandler(std::make_shared<BitViewHandler>(newbitc));
        else
            logger->error("Can't add container!");
    }
} // namespace satdump