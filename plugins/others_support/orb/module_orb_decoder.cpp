#include "module_orb_decoder.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include <filesystem>
#include <fstream>

namespace orb
{
    ORBDecoderModule::ORBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    void ORBDecoderModule::process()
    {
        uint8_t cadu[1024];

        // Demuxers
        ccsds::ccsds_aos::Demuxer demuxer_vcid4(882, true, 2);

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
            l3_parser.directory = l3_dir;
        }

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)cadu, 1024);

            // Parse this transport frame
            ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

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
        }

        cleanup();

        l2_parser.saveAll();
        l3_parser.saveAll();
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
                        //     ImGui::TextColored(style::theme.green, "Writing image...");
                        /*else*/
                        if (dec.is_dling)
                            ImGui::TextColored(style::theme.orange, "Receiving...");
                        else
                            ImGui::TextColored(style::theme.red, "Idle (Image)...");
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
                        //     ImGui::TextColored(style::theme.green, "Writing image...");
                        /*else*/
                        if (dec.is_dling)
                            ImGui::TextColored(style::theme.orange, "Receiving...");
                        else
                            ImGui::TextColored(style::theme.red, "Idle (Image)...");
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
                    ImGui::TextColored(style::theme.red, "Idle (Image)...");
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
            }
        }
        ImGui::EndTabBar();

        drawProgressBar();

        ImGui::End();
    }

    std::string ORBDecoderModule::getID() { return "orb_decoder_test"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> ORBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<ORBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace orb