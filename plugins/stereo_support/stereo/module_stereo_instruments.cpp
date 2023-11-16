#include "module_stereo_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_standard/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_standard/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"

#include "resources.h"

namespace stereo
{
    StereoInstrumentsDecoderModule::StereoInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    image::Image<uint16_t> StereoInstrumentsDecoderModule::decompress_icer_tool(uint8_t *data, int dsize, int size)
    {
        std::ofstream("./stereo_secchi_raw.tmp").write((char *)data, dsize);

        if (std::filesystem::exists("./stereo_secchi_out.tmp"))
            std::filesystem::remove("./stereo_secchi_out.tmp");

        std::string icer_path = d_parameters["icer_path"];
        std::string cmd = icer_path + " -vv -i ./stereo_secchi_raw.tmp -o ./stereo_secchi_out.tmp";

        if (!std::filesystem::exists(icer_path))
        {
            logger->error("No ICER Decompressor provided. Can't decompress SECCHI!");
            return image::Image<uint16_t>();
        }

        int ret = system(cmd.data());

        if (ret == 0 && std::filesystem::exists("./stereo_secchi_out.tmp"))
        {
            logger->trace("SECCHI Decompression OK!");

            std::ifstream data_in("./stereo_secchi_out.tmp", std::ios::binary);
            uint16_t *buffer = new uint16_t[size * size];
            data_in.read((char *)buffer, sizeof(uint16_t) * size * size);
            image::Image<uint16_t> img(buffer, size, size, 1);
            delete[] buffer;

            if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                std::filesystem::remove("./stereo_secchi_out.tmp");

            return img;
        }
        else
        {
            logger->error("Failed decompressing SECCHI!");

            if (std::filesystem::exists("./stereo_secchi_out.tmp"))
                std::filesystem::remove("./stereo_secchi_out.tmp");

            return image::Image<uint16_t>();
        }
    }

    void StereoInstrumentsDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        logger->info("Using input frames " + d_input_file);

        time_t lastTime = 0;
        uint8_t cadu[1119];

        // std::ofstream output("file.ccsds");

        int s_waves_lines = 0;
        std::vector<uint8_t> s_waves_data;

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

        std::filesystem::create_directories(directory + "/SECCHI/");

        secchi_reader = new secchi::SECCHIReader(d_parameters["icer_path"], directory + "/SECCHI");

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)&cadu, 1119);

            // Parse this transport frame
            ccsds::ccsds_standard::VCDU vcdu = ccsds::ccsds_standard::parseVCDU(cadu);

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
                    else if (pkt.header.apid == 880)
                    {
                    }
                    else if (pkt.header.apid == 624)
                    {
                    }
                    else if (pkt.header.apid == 0) // IDK
                    {
                    }
                    else if (pkt.header.apid != 2047)
                    {
                        // printf("APID %d\n", pkt.header.apid);
                    }
                }
            }

            progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        delete secchi_reader;

        data_in.close();

        {
            logger->info("----------- S/WAVES");
            logger->info("Lines : " + std::to_string(s_waves_lines));

            image::Image<uint8_t> image_s_waves(s_waves_data.data(), 162, s_waves_lines, 1);
            image_s_waves.save_img(directory + "/S_WAVES");
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
                ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)cips_readers[i].images.size());
                ImGui::TableSetColumnIndex(2);
                drawStatus(cips_status[i]);
            }

            ImGui::EndTable();
        }*/

        ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string StereoInstrumentsDecoderModule::getID()
    {
        return "stereo_instruments";
    }

    std::vector<std::string> StereoInstrumentsDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> StereoInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<StereoInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
}