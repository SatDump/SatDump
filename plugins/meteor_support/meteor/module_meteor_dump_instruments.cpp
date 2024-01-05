#include "module_meteor_dump_instruments.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "meteor.h"
#include "products/image_products.h"
#include "common/tracking/tle.h"
#include "products/dataset.h"
#include "resources.h"

namespace meteor
{
    namespace instruments
    {
        MeteorDumpInstrumentsDecoderModule::MeteorDumpInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MeteorDumpInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t frame[380];

            std::vector<uint16_t> mtvza_channels[46];

            for (int i = 0; i < 46; i++)
                mtvza_channels[i].resize(200);

            double last_timestamp = 0;
            std::vector<double> timestamps;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&frame, 380);

                {
                    int counter = frame[4];

                    int pos = 10;
                    for (int c = 0; c < 46; c++)
                    {
                        for (int p = 0; p < 4; p++)
                        {
                            uint16_t val =
                                (frame[pos + p * 92 + c * 2 + 0] << 8 | frame[pos + p * 92 + c * 2 + 1]); //+ 32768;

                            // int16_t val2 = *((int16_t *)&val);

                            if (counter < 51 && counter > 0)
                                mtvza_channels[c][mtvza_lines * 200 + (counter - 1) * 4 + p] = val; // (int)val2; // + 32768;
                        }
                    }

                    if (counter == 51)
                    {
                        mtvza_lines++;

                        for (int i = 0; i < 46; i++)
                            mtvza_channels[i].resize((mtvza_lines + 1) * 200);
                        timestamps.push_back(last_timestamp);

                        logger->info("Lines %d CNT %d", mtvza_lines, counter);
                    }
                    else if (counter == 0)
                    {
                        uint16_t vidk1 = frame[12] << 8 | frame[13];
                        //  uint16_t vidk2 = frame[36] << 8 | frame[37];
                        uint32_t vidk2 = frame[14] << 8 | frame[15] << 0; //| frame[16] << 8 | frame[17];

                        // uint16_t counter = frame[6] << 8 | frame[7];

                        double vtidk = 16708 * 24 * 3600 + (vidk2) * 65536 + vidk1 + 189 * 60 - 101.1 * 10 * 60; // (double)vidk1 + (double(vidk2) / 65535.0);

                        last_timestamp = vtidk; // dayValue + double(counter) * (60.0 / 24.85) - 4 * 3600; //+ 5 * 60; //- 3 * 3600.0; //- 14 * 3600 + 3 * 60; //+ 12 * 3600.0;

                        // logger->trace("%f ---- %f               %f", last_vidk, vtidk - last_vidk, last_timestamp);

                        // printf("TIMET %s \n", timestamp_to_string(last_timestamp).c_str());
                        // last_vidk = vtidk;
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

#if 0
            // Identify satellite, and apply per-sat settings...
            int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end());
            msumr_ids.clear();
#endif

            std::string sat_name = "Unknown Meteor";
#if 0
            if (msumr_serial_number == 0)
                sat_name = "METEOR-M2";
            else if (msumr_serial_number == 1)
                sat_name = "METEOR-M2-1";
            else if (msumr_serial_number == 2)
                sat_name = "METEOR-M2-2";
            else if (msumr_serial_number == 3)
                sat_name = "METEOR-M2-3";

            int norad = 0;
            if (msumr_serial_number == 0)
                norad = 40069; // M2
            else if (msumr_serial_number == 1)
                norad = 0; // M2-1, failed launch
            else if (msumr_serial_number == 2)
                norad = 44387; // M2-2
            else if (msumr_serial_number == 3)
                norad = 57166; // M2-3
#endif

            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = time(0); // avg_overflowless(msumr_timestamps);

#if 0
            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            // MSU-MR
            {
                msumr_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-MR";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MSU-MR");
                logger->info("Lines : " + std::to_string(msumr_reader.lines));

                satdump::ImageProducts msumr_products;
                msumr_products.instrument_name = "msu_mr";
                msumr_products.has_timestamps = true;
                msumr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                msumr_products.set_tle(satdump::general_tle_registry.get_from_norad(norad));

                std::vector<double> filter_timestamps = msumr_timestamps;
                double avg = avg_overflowless(filter_timestamps);
                double last = 0;
                for (double &v : filter_timestamps)
                {
                    if (abs(avg - v) > 10000)
                    {
                        last = v;
                        v = -1;
                        continue;
                    }

                    if (last >= v || abs(last - v) > 1.0)
                    {
                        last = v;
                        v = -1;
                        continue;
                    }
                    last = v;

                    // logger->info(v);
                }

                // for (double &v : filter_timestamps)
                //     logger->info(v);

                msumr_products.set_timestamps(filter_timestamps);
                if (msumr_serial_number == 0)
                    msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr.json")));
                else if (msumr_serial_number == 2)
                    msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-2_msumr.json")));
                else if (msumr_serial_number == 3)
                    msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_msumr.json")));

                for (int i = 0; i < 6; i++)
                    msumr_products.images.push_back({"MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), msumr_reader.getChannel(i)});

                msumr_products.save(directory);
                dataset.products_list.push_back("MSU-MR");

                msumr_status = DONE;
            }
#endif

            // MTVZA
            {
                mtvza_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MTVZA";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MTVZA");
                logger->info("Lines : " + std::to_string(mtvza_lines));

                satdump::ImageProducts mtvza_products;
                mtvza_products.instrument_name = "mtvza";
                mtvza_products.has_timestamps = true;
                mtvza_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                int norad = 57166;
                mtvza_products.set_tle(satdump::general_tle_registry.get_from_norad(norad));
                mtvza_products.set_timestamps(timestamps);
                mtvza_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_mtvza_dump.json")));

                for (int i = 0; i < 46; i++)
                    mtvza_products.images.push_back({"MTVZA-" + std::to_string(i + 1), std::to_string(i + 1), image::Image<uint16_t>(mtvza_channels[i].data(), 200, mtvza_lines, 1)});

                mtvza_products.save(directory);
                dataset.products_list.push_back("MTVZA");

                mtvza_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void MeteorDumpInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR Dump Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##meteordumpinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MTVZA");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", mtvza_lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mtvza_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MeteorDumpInstrumentsDecoderModule::getID()
        {
            return "meteor_dump_instruments";
        }

        std::vector<std::string> MeteorDumpInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MeteorDumpInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MeteorDumpInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop
