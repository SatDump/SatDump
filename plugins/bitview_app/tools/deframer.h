#pragma once

#include "common/simple_deframer.h"
#include "core/style.h"
#include "imgui/imgui_stdlib.h"
#include "tool.h"
#include <fstream>
#include <string>

namespace satdump
{
    class DeframerTool : public BitViewTool
    {
    private:
        std::string deframer_syncword = "0x1acffc1d";
        int deframer_syncword_size = 32;
        int deframer_syncword_framesize = 8192;
        int deframer_current_frames = 0;
        int deframer_threshold = 0;
        bool deframer_byte_aligned = false;
        bool deframer_soft_bits_in = false;

        bool should_process = false;

    public:
        std::string getName() { return "Deframer"; }

        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
        {
            if (is_busy)
                style::beginDisabled();

            ImGui::InputText("Syncword", &deframer_syncword);
            ImGui::InputInt("Syncword Size", &deframer_syncword_size);
            ImGui::InputInt("Frame Size (Bits)", &deframer_syncword_framesize);
            ImGui::InputInt("Threshold", &deframer_threshold);
            ImGui::Checkbox("Byte Aligned", &deframer_byte_aligned);
            ImGui::Checkbox("Soft Bits In", &deframer_soft_bits_in);

            if (ImGui::Button("DeframeTest"))
                should_process = true;

            ImGui::Text("Frames : %d", deframer_current_frames);

            if (is_busy)
                style::endDisabled();
        }

        bool needToProcess() { return should_process; }

        void setProcessed() { should_process = false; }

        void process(std::shared_ptr<BitContainer> &container, float &process_progress)
        {
            uint8_t *ptr = container->get_ptr();
            size_t size = container->get_ptr_size();

            char name[1000];
            tmpnam(name);
            std::string tmpfile = name;
            std::ofstream file_out(tmpfile, std::ios::binary);

            char *syncword_endptr;
            uint64_t syncword = strtoull(deframer_syncword.c_str(), &syncword_endptr, 16);
            if (*syncword_endptr != 0)
            {
                logger->error("Cannot parse syncword!");
                return;
            }

            def::SimpleDeframer def_test(syncword, deframer_syncword_size, deframer_syncword_framesize, deframer_threshold, deframer_byte_aligned, deframer_soft_bits_in);
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
            std::shared_ptr<satdump::BitContainer> newbitc;

            try
            {
                newbitc = std::make_shared<satdump::BitContainer>("Deframed (" + deframer_syncword + " , " + std::to_string(deframer_syncword_framesize) + ")", tmpfile);
            }
            catch (std::exception &e)
            {
                logger->error("Error loading deframed data: %s", e.what());
                return;
            }

            newbitc->d_bitperiod = deframer_syncword_framesize;
            newbitc->init_bitperiod();
            newbitc->d_is_temporary = true;

            if (container->bitview != nullptr)
                ((BitViewHandler *)container->bitview)->addSubHandler(std::make_shared<BitViewHandler>(newbitc));
            else
                logger->error("Can't add container!");
        }
    };
} // namespace satdump