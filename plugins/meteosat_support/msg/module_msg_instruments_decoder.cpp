#include "module_msg_instruments_decoder.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace meteosat
{
    MSGInstrumentsDecoderModule::MSGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    void MSGInstrumentsDecoderModule::process()
    {
        uint8_t cadu[1279];

        // Demuxers
        ccsds::ccsds_tm::Demuxer demuxer_vcid0(1109, false, 0, 0);
        ccsds::ccsds_tm::Demuxer demuxer_vcid1(1109, false, 0, 0);

        // Setup readers
        seviri_reader = std::make_shared<msg::SEVIRIReader>(d_parameters["seviri_rss"].get<bool>());
        std::string seviri_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SEVIRI";
        if (!std::filesystem::exists(seviri_directory))
            std::filesystem::create_directory(seviri_directory);
        seviri_reader->d_directory = seviri_directory;

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)cadu, 1279);

            // Parse this transport frame
            ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

            if (vcdu.vcid == 0)
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid0.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 2046)
                        seviri_reader->work(vcdu.spacecraft_id, pkt);
            }
            else if (vcdu.vcid == 1)
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid1.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    ; // WIP
            }

            // TODOREWORK maybe add Progress as stats?
        }

        cleanup();

        seviri_reader->saveImages();
    }

    void MSGInstrumentsDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("MSG Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTabBar("Images TabBar", ImGuiTabBarFlags_None))
        {
            if (seviri_reader)
            {
                if (seviri_reader->textureID == 0)
                {
                    seviri_reader->textureID = makeImageTexture();
                    seviri_reader->textureBuffer = new uint32_t[1000 * 1000];
                    memset(seviri_reader->textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                    seviri_reader->hasToUpdate = true;
                }

                if (seviri_reader->hasToUpdate)
                {
                    seviri_reader->hasToUpdate = false;
                    updateImageTexture(seviri_reader->textureID, seviri_reader->textureBuffer, 1000, 1000);
                }

                if (ImGui::BeginTabItem("Ch 4"))
                {
                    ImGui::Image((void *)(intptr_t)seviri_reader->textureID, {200 * ui_scale, 200 * ui_scale});
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                    if (seviri_reader->is_saving)
                        ImGui::TextColored(style::theme.green, "Saving...");
                    else
                        ImGui::TextColored(style::theme.orange, "Receiving...");
                    // else
                    //     ImGui::TextColored(style::theme.red, "Idle (Image)...");
                    ImGui::EndGroup();
                    ImGui::EndTabItem();
                }
            }
        }
        ImGui::EndTabBar();

        drawProgressBar();

        ImGui::End();
    }

    std::string MSGInstrumentsDecoderModule::getID() { return "msg_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> MSGInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MSGInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteosat