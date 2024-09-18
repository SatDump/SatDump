#pragma once

#include "tool.h"
#include "imgui/imgui_stdlib.h"
#include <fstream>
#include "common/codings/differential/nrzm.h"
#include "core/style.h"

namespace satdump
{
    class DifferentialTool : public BitViewTool
    {
    private:
        int diff_mode = 0;

        bool should_process = false;

    public:
        std::string getName() { return "Differential Decoder"; }

        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
        {
            if (is_busy)
                style::beginDisabled();

            if (ImGui::RadioButton("NRZ-M", diff_mode == 0))
                diff_mode = 0;
            ImGui::SameLine();
            if (ImGui::RadioButton("NRZ-S", diff_mode == 1))
                diff_mode = 1;

            if (ImGui::Button("Differential Decode"))
                should_process = true;

            if (is_busy)
                style::endDisabled();
        }

        bool needToProcess()
        {
            return should_process;
        }

        void setProcessed()
        {
            should_process = false;
        }

        void process(std::shared_ptr<BitContainer> &container, float &process_progress)
        {
            uint8_t *ptr = container->get_ptr();
            size_t size = container->get_ptr_size();

            char name[1000];
            tmpnam(name);
            std::string tmpfile = name;
            std::ofstream file_out(tmpfile, std::ios::binary);
            diff::NRZMDiff nrzm_decoder;

            uint8_t tmp_buf[8192];

            size_t current_ptr = 0;
            while (current_ptr < size)
            {
                size_t csize = std::min<size_t>(8192, size - current_ptr);

                memcpy(tmp_buf, ptr + current_ptr, csize);
                current_ptr += csize;

                nrzm_decoder.decode(tmp_buf, csize);

                if (diff_mode == 1)
                    for (int i = 0; i < csize; i++)
                        tmp_buf[i] ^= 0xFF;

                file_out.write((char *)tmp_buf, csize);

                process_progress = double(current_ptr) / double(size);
            }

            file_out.close();

            std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>(container->getName() + (diff_mode == 1 ? " NRZ-S" : " NRZ-M"), tmpfile);
            newbitc->d_bitperiod = container->d_bitperiod;
            newbitc->init_bitperiod();
            newbitc->d_is_temporary = true;

            container->all_bit_containers.push_back(newbitc);
        }
    };
}