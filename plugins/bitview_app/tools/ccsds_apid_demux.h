#pragma once

#include "bit_container.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"
#include "tool.h"
#include <fstream>
#include <map>
#include <string>

namespace satdump
{
    class CCSDSAPIDDemuxTool : public BitViewTool
    {
    private:
        bool should_process = false;

        int mpdu_data_size = 884;
        int insert_zone_size = 0;
        int apid = 1;
        bool filter_apid = false;

    public:
        std::string getName() { return "CCSDS APID Demux"; }

        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
        {
            if (is_busy)
                style::beginDisabled();

            ImGui::InputInt("MPDU Data Size", &mpdu_data_size);
            ImGui::InputInt("MPDU Insert Zone Size", &insert_zone_size);
            ImGui::Checkbox("Filter APID", &filter_apid);
            if (filter_apid)
                ImGui::InputInt("APID", &apid);

            if (ImGui::Button("Perform###2"))
                should_process = true;

            if (is_busy)
                style::endDisabled();
        }

        bool needToProcess() { return should_process; }

        void setProcessed() { should_process = false; }

        void process(std::shared_ptr<BitContainer> &container, float &process_progress)
        {
            logger->critical("Demux!!!!");

            uint8_t *ptr = container->get_ptr();
            size_t size = container->get_ptr_size();

            char name[1000];
            tmpnam(name);
            std::string tmpfile = name;
            std::ofstream fileout = std::ofstream(tmpfile, std::ios::binary);
            std::vector<BitContainer::FrameDef> frms;

            ccsds::ccsds_aos::Demuxer demuxer_vcid(mpdu_data_size, insert_zone_size, insert_zone_size);

            size_t pos = 0;
            for (int i = 0; i < size; i += 1024)
            {
                auto frm = demuxer_vcid.work(ptr + i);

                for (auto &f : frm)
                {
                    if (!filter_apid || f.header.apid == apid)
                    {
                        fileout.write((char *)f.header.raw, 6);
                        fileout.write((char *)f.payload.data(), f.payload.size());

                        frms.push_back({pos * 8, (f.payload.size() + 6) * 8});
                        pos += f.payload.size() + 6;
                    }
                }

                process_progress = double(i) / double(size);
            }

            if (frms.size())
            {
                std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>("APID " + std::to_string(apid), tmpfile, frms);
                newbitc->d_bitperiod = 8192;
                newbitc->init_bitperiod();
                newbitc->d_is_temporary = true;

                if (container->bitview != nullptr)
                    ((BitViewHandler *)container->bitview)->addSubHandler(std::make_shared<BitViewHandler>(newbitc));
                else
                    logger->error("Can't add container!");
            }

#if 0
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
                    vcid = ccsds::ccsds_aos::parseVCDU(ptr_pos).vcid;
                else if (cadu_mode == 1)
                    vcid = ccsds::ccsds_tm::parseVCDU(ptr_pos).vcid;

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

                if (container->bitview != nullptr)
                    ((BitViewHandler *)container->bitview)->addSubHandler(std::make_shared<BitViewHandler>(newbitc));
                else
                    logger->error("Can't add container!");
            }
#endif
        }
    };
} // namespace satdump