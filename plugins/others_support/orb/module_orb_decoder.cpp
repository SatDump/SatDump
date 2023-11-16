#include "module_orb_decoder.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "imgui/imgui_image.h"

namespace orb
{
    ORBDecoderModule::ORBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    void ORBDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        logger->info("Using input frames " + d_input_file);

        time_t lastTime = 0;
        uint8_t cadu[1024];

        // Demuxers
        ccsds::ccsds_weather::Demuxer demuxer_vcid4(882, true, 2);

        // Setup readers
        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        {
            std::string l2_dir = directory + "/ELEKTRO-L2";
            if (!std::filesystem::exists(l2_dir))
                std::filesystem::create_directories(l2_dir);
            l2_parser.directory = l2_dir;
        }

        {
            std::string l3_dir = directory + "/ELEKTRO-L3";
            if (!std::filesystem::exists(l3_dir))
                std::filesystem::create_directories(l3_dir);
            l2_parser.directory = l3_dir;
        }

        while (!data_in.eof())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)cadu, 1024);
            else
                input_fifo->read((uint8_t *)cadu, 1024);

            // Parse this transport frame
            ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

            if (vcdu.vcid == 4)
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                {
                    if (pkt.header.apid == 2)
                        l2_parser.work(pkt);
                    if (pkt.header.apid == 3)
                        l3_parser.work(pkt);
                }
            }

            progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        data_in.close();

        l2_parser.saveAll();
    }

    void ORBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("ORB Decoder Test", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTabBar("Images TabBar", ImGuiTabBarFlags_None))
        {
            bool hasImage = false;

            // ELEKTRO-L 2
            for (auto &imgMap : l2_parser.decoded_imgs)
            {
                auto &dec = imgMap.second;

                if (dec.textureID == 0)
                {
                    dec.textureID = makeImageTexture();
                    dec.textureBuffer = new uint32_t[1000 * 1000];
                    memset(dec.textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                    dec.hasToUpdate = true;
                }

                if (dec.is_dling)
                {
                    if (dec.hasToUpdate)
                    {
                        dec.hasToUpdate = false;
                        updateImageTexture(dec.textureID, dec.textureBuffer, 1000, 1000);
                    }

                    hasImage = true;

                    if (ImGui::BeginTabItem(std::string("L2 Ch " + std::to_string(imgMap.first)).c_str()))
                    {
                        ImGui::Image((void *)(intptr_t)dec.textureID, {200 * ui_scale, 200 * ui_scale});
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                        // if (dec.imageStatus == SAVING)
                        //     ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                        /*else*/
                        if (dec.is_dling)
                            ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                        else
                            ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                }
            }

            // ELEKTRO-L 3
            for (auto &imgMap : l3_parser.decoded_imgs)
            {
                auto &dec = imgMap.second;

                if (dec.textureID == 0)
                {
                    dec.textureID = makeImageTexture();
                    dec.textureBuffer = new uint32_t[1000 * 1000];
                    memset(dec.textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                    dec.hasToUpdate = true;
                }

                if (dec.is_dling)
                {
                    if (dec.hasToUpdate)
                    {
                        dec.hasToUpdate = false;
                        updateImageTexture(dec.textureID, dec.textureBuffer, 1000, 1000);
                    }

                    hasImage = true;

                    if (ImGui::BeginTabItem(std::string("L3 Ch " + std::to_string(imgMap.first)).c_str()))
                    {
                        ImGui::Image((void *)(intptr_t)dec.textureID, {200 * ui_scale, 200 * ui_scale});
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                        // if (dec.imageStatus == SAVING)
                        //     ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                        /*else*/
                        if (dec.is_dling)
                            ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                        else
                            ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                }
            }

            if (!hasImage) // Add empty tab if there is no image yet
            {
                if (ImGui::BeginTabItem("No image yet"))
                {
                    ImGui::Dummy({200 * ui_scale, 200 * ui_scale});
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                    ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
            }
        }
        ImGui::EndTabBar();

        ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string ORBDecoderModule::getID()
    {
        return "orb_decoder_test";
    }

    std::vector<std::string> ORBDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> ORBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<ORBDecoderModule>(input_file, output_file_hint, parameters);
    }
}