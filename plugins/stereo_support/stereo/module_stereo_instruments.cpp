#include "module_stereo_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/utils.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "core/resources.h"

namespace stereo
{
    StereoInstrumentsDecoderModule::StereoInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
        icer_path = parameters.contains("icer_path") ? parameters["icer_path"].get<std::string>() : "";
    }

    void StereoInstrumentsDecoderModule::process()
    {
        uint8_t cadu[1119];

        // std::ofstream output("file.ccsds");

        int s_waves_lines = 0;
        std::vector<uint8_t> s_waves_data;

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        std::filesystem::create_directories(directory + "/SECCHI/");

        secchi_reader = new secchi::SECCHIReader(icer_path, directory + "/SECCHI");
        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)&cadu, 1119);

            // Parse this transport frame
            ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

            // logger->info(pkt.header.apid);
            // logger->info(vcdu.vcid);

            // uint8_t *tttt = &cadu[4 + 7];
            // double timestamp = (tttt[0] << 24 |
            //                     tttt[1] << 16 |
            //                     tttt[2] << 8 |
            //                     tttt[3]) +
            //                    tttt[4] / 256.0 +
            //                    1161734400 + 43200;

            //  logger->trace(timestamp_to_string(timestamp));

            if (vcdu.vcid == 7) // Space Beacon VCID
            {
                for (int i = 0; i < 4; i++) // Extract all 4 packets. Hacky but hey works.
                {
                    ccsds::CCSDSPacket pkt;
                    pkt.header = ccsds::parseCCSDSHeader(&cadu[25 + i * 272]);
                    pkt.payload.insert(pkt.payload.end(), &cadu[25 + i * 272 + 6], &cadu[25 + i * 272 + 6 + 266]);

                    if (pkt.header.apid == 1393) // S/WAVES
                    {
                        s_waves_data.insert(s_waves_data.end(), (char *)&pkt.payload[20], (char *)&pkt.payload[20 + 162]);
                        s_waves_lines++;
                    }
                    else if (pkt.header.apid == 1140) // SECCHI SUV
                    {
                        secchi_reader->work(pkt);
                    }
                    else if (pkt.header.apid == 1139) // SECCHI NotSureYet?
                    {
                        secchi_reader->work(pkt);
                    }
                    else if (pkt.header.apid == 1138) // SECCHI NotSureYet?
                    {
                        secchi_reader->work(pkt);
                    }
                    else if (pkt.header.apid == 1137) // SECCHI Occulted?
                    {
                        secchi_reader->work(pkt);
                    }
                    else if (pkt.header.apid == 880) {}
                    else if (pkt.header.apid == 624) {}
                    else if (pkt.header.apid == 0) // IDK
                    {
                    }
                    else if (pkt.header.apid != 2047)
                    {
                        // printf("APID %d\n", pkt.header.apid);
                    }
                }
            }
        }

        delete secchi_reader;

        cleanup();

        {
            logger->info("----------- S/WAVES");
            logger->info("Lines : " + std::to_string(s_waves_lines));

            image::Image image_s_waves(s_waves_data.data(), 8, 162, s_waves_lines, 1);
            image::save_img(image_s_waves, directory + "/S_WAVES");
        }
    }

    void StereoInstrumentsDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Stereo Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        /*if (ImGui::BeginTable("##aiminstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Instrument");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Images / Frames");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Status");

            for (int i = 0; i < 4; i++)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("CIPS %d", i + 1);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", (int)cips_readers[i].images.size());
                ImGui::TableSetColumnIndex(2);
                drawStatus(cips_status[i]);
            }

            ImGui::EndTable();
        }*/

        drawProgressBar();

        ImGui::End();
    }

    std::string StereoInstrumentsDecoderModule::getID() { return "stereo_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> StereoInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<StereoInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace stereo