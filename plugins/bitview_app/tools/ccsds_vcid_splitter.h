#pragma once

#include "tool.h"
#include "imgui/imgui_stdlib.h"
#include <fstream>
#include "core/style.h"
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "common/ccsds/ccsds_standard/vcdu.h"
#include <map>

namespace satdump
{
    class CCSDSVcidSplitterTool : public BitViewTool
    {
    private:
        int cadu_mode = 0;
        int cadu_size = 8192;

        bool should_process = false;

    public:
        std::string getName() { return "CCSDS VCID Splitter"; }

        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
        {
            if (is_busy)
                style::beginDisabled();

            if (ImGui::RadioButton("AOS", cadu_mode == 0))
                cadu_mode = 0;
            if (ImGui::RadioButton("TM", cadu_mode == 1))
                cadu_mode = 1;

            ImGui::InputInt("CADU Width (bits)", &cadu_size);

            if (ImGui::Button("Perform"))
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

            std::map<int, int> vcid_counts;
            std::map<int, std::pair<std::string, std::ofstream>> output_files;

            size_t current_ptr = 0;
            while (current_ptr < size)
            {
                size_t csize = std::min<size_t>(cadu_size / 8, size - current_ptr);
                if (cadu_size / 8 != csize)
                    break;

                uint8_t *ptr_pos = ptr + current_ptr;

                int vcid = 0;
                if (cadu_mode == 0)
                    vcid = ccsds::ccsds_weather::parseVCDU(ptr_pos).vcid;
                else if (cadu_mode == 0)
                    vcid = ccsds::ccsds_standard::parseVCDU(ptr_pos).vcid;

                if (output_files.count(vcid) == 0)
                {
                    output_files.emplace(vcid, std::pair<std::string, std::ofstream>{"", std::ofstream()});
                    vcid_counts.emplace(vcid, 0);

                    char name[1000];
                    tmpnam(name);
                    std::string tmpfile = name;
                    output_files[vcid].first = tmpfile;
                    output_files[vcid].second = std::ofstream(tmpfile, std::ios::binary);
                }

                output_files[vcid].second.write((char *)ptr_pos, cadu_size / 8);
                vcid_counts[vcid]++;

                current_ptr += csize;

                process_progress = double(current_ptr) / double(size);
            }

            for (auto &f : output_files)
            {
                f.second.second.close();
                std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>(container->getName() + " VCID " + std::to_string(f.first) + " (" + std::to_string(vcid_counts[f.first]) + ")", f.second.first);
                newbitc->d_bitperiod = cadu_size;
                newbitc->init_bitperiod();
                newbitc->d_is_temporary = true;

                container->all_bit_containers.push_back(newbitc);
            }
        }
    };
}