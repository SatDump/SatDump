#include "module_msg_instruments_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "imgui/imgui_image.h"

namespace meteosat
{
    MSGInstrumentsDecoderModule::MSGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    void MSGInstrumentsDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        logger->info("Using input frames " + d_input_file);

        time_t lastTime = 0;
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

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)cadu, 1279);
            else
                input_fifo->read((uint8_t *)cadu, 1279);

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

            progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        data_in.close();

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
                    seviri_reader->textureID = makeImageTexture(1000, 1000);
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

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string MSGInstrumentsDecoderModule::getID()
    {
        return "msg_instruments";
    }

    std::vector<std::string> MSGInstrumentsDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> MSGInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MSGInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
}