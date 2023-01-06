#include "module_gcom1_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "products/products.h"
#include "products/dataset.h"
#include "products/image_products.h"

namespace gcom1
{
    namespace instruments
    {
        GCOM1InstrumentsDecoderModule::GCOM1InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void GCOM1InstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1264];

            // Demuxers
            ccsds::ccsds_weather::Demuxer demuxer_vcid17(1092, false);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1264);

                // Parse this transport frame
                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

                // logger->info(pkt.header.apid);
                // logger->info(vcdu.vcid);

                if (vcdu.vcid == 17) // AMSR-2 VCID
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid17.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 1576)
                            amsr2_reader.work(pkt);
                    }
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            std::string sat_name = "GCOM-W1";
            int norad = 38337;

            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            // AMSR-2
            {
                amsr2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSR-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AMSR-2");
                logger->info("Lines : " + std::to_string(amsr2_reader.lines));

                satdump::ImageProducts amsr2_products;
                amsr2_products.instrument_name = "amsr2";
                // amsr2_products.has_timestamps = true;
                // amsr2_products.set_tle(satellite_tle);
                amsr2_products.bit_depth = 12;
                // amsr2_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                // amsr2_products.set_timestamps(mhs_reader.timestamps);
                // amsr2_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_mhs.json")));

                for (int i = 0; i < 20; i++)
                    amsr2_products.images.push_back({"AMSR2-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), amsr2_reader.getChannel(i)});

                amsr2_products.save(directory);
                dataset.products_list.push_back("AMSR-2");

                amsr2_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void GCOM1InstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GCOM-1 Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##gcominstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AMSR-2");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", (int)amsr2_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(amsr2_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GCOM1InstrumentsDecoderModule::getID()
        {
            return "gcom1_instruments";
        }

        std::vector<std::string> GCOM1InstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GCOM1InstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GCOM1InstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop