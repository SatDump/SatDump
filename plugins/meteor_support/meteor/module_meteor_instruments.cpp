#include "module_meteor_instruments.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "meteor.h"
#include "products/image_products.h"
#include "common/simple_deframer.h"
#include "common/tracking/tle.h"
#include "products/dataset.h"
#include "resources.h"

namespace meteor
{
    namespace instruments
    {
        MeteorInstrumentsDecoderModule::MeteorInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MeteorInstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            mtvza_reader2.endian_mode = true;

            // Deframers
            def::SimpleDeframer msumr_deframer(0x0218a7a392dd9abf, 64, 11850 * 8, 10, true);
            def::SimpleDeframer mtvza_deframer(0xFB386A45, 32, 248 * 8, 0, true);
            def::SimpleDeframer mtvza_deframer2(0x38fb456a, 32, 248 * 8, 0, true);
            def::SimpleDeframer bism_deframer(0x71DE2CD8, 32, 88 * 8, 0, true);

            time_t current_time = d_parameters.contains("start_timestamp")
                                      ? (d_parameters["start_timestamp"].get<double>() != -1
                                             ? d_parameters["start_timestamp"].get<double>()
                                             : time(0))
                                      : time(0);

            time_t currentDay = current_time + 3 * 3600.0;       // Moscow Time
            time_t dayValue = currentDay - (currentDay % 86400); // Requires the day to be known from another source

            std::vector<double> msumr_timestamps;
            std::vector<uint8_t> msumr_ids;

            // std::ofstream file_out("idk_bism.bin");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                std::vector<std::vector<uint8_t>> msumr_frames;
                std::vector<std::vector<uint8_t>> mtvza_frames, mtvza_frames2;
                std::vector<std::vector<uint8_t>> bism_frames;

                // MSU-MR Deframing
                {
                    std::vector<uint8_t> msu_mr_data;
                    msu_mr_data.insert(msu_mr_data.end(), &cadu[23 - 1], &cadu[23 - 1] + 238);
                    msu_mr_data.insert(msu_mr_data.end(), &cadu[279 - 1], &cadu[279 - 1] + 238);
                    msu_mr_data.insert(msu_mr_data.end(), &cadu[535 - 1], &cadu[535 - 1] + 238);
                    msu_mr_data.insert(msu_mr_data.end(), &cadu[791 - 1], &cadu[791 - 1] + 234);
                    msumr_frames = msumr_deframer.work(msu_mr_data.data(), msu_mr_data.size());
                }

                // MSU-MR Processing
                for (std::vector<uint8_t> msumr_frame : msumr_frames)
                {
                    msumr_reader.work(msumr_frame.data());
                    double timestamp = dayValue + (msumr_frame[8]) * 3600.0 + (msumr_frame[9]) * 60.0 + (msumr_frame[10] + 0.0) + double(msumr_frame[11] / 255.0);
                    timestamp -= 3 * 3600.0;
                    msumr_timestamps.push_back(timestamp);
                    mtvza_reader.latest_msumr_timestamp = mtvza_reader2.latest_msumr_timestamp = timestamp; // MTVZA doesn't have timestamps of its own, so use MSU-MR's
                    msumr_ids.push_back(msumr_frame[12] >> 4);
                }

                // MTVZA Deframing
                {
                    std::vector<uint8_t> mtvza_data;
                    mtvza_data.insert(mtvza_data.end(), &cadu[15 - 1], &cadu[15 - 1] + 8);
                    mtvza_data.insert(mtvza_data.end(), &cadu[271 - 1], &cadu[271 - 1] + 8);
                    mtvza_data.insert(mtvza_data.end(), &cadu[527 - 1], &cadu[527 - 1] + 8);
                    mtvza_data.insert(mtvza_data.end(), &cadu[783 - 1], &cadu[783 - 1] + 8);
                    mtvza_frames = mtvza_deframer.work(mtvza_data.data(), mtvza_data.size());
                    mtvza_frames2 = mtvza_deframer2.work(mtvza_data.data(), mtvza_data.size());
                }

                // MTVZA Processing
                for (std::vector<uint8_t> &frame : mtvza_frames)
                    mtvza_reader.work(frame.data());
                for (std::vector<uint8_t> &frame : mtvza_frames2)
                    mtvza_reader2.work(frame.data());

                // BIS-M Deframing
                {
                    std::vector<uint8_t> bism_data;
                    bism_data.insert(bism_data.end(), &cadu[7 - 1], &cadu[7 - 1] + 4);
                    bism_data.insert(bism_data.end(), &cadu[263 - 1], &cadu[263 - 1] + 4);
                    bism_data.insert(bism_data.end(), &cadu[519 - 1], &cadu[519 - 1] + 4);
                    bism_data.insert(bism_data.end(), &cadu[775 - 1], &cadu[775 - 1] + 4);
                    bism_frames = bism_deframer.work(bism_data.data(), bism_data.size());
                }

                // BIS-M Processing
                // for (std::vector<uint8_t> &frame : bism_frames)
                {
                    // TODO
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            // Identify satellite, and apply per-sat settings...
            int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end());
            msumr_ids.clear();

            std::string sat_name = "Unknown Meteor";
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

            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = get_median(msumr_timestamps);

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

            // MTVZA
            {
                auto &mreader = mtvza_reader2.lines > mtvza_reader.lines
                                    ? mtvza_reader2
                                    : mtvza_reader;

                mtvza_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MTVZA";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MTVZA");
                logger->info("Lines : " + std::to_string(mreader.lines));

                satdump::ImageProducts mtvza_products;
                mtvza_products.instrument_name = "mtvza";
                mtvza_products.has_timestamps = true;
                mtvza_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mtvza_products.set_tle(satdump::general_tle_registry.get_from_norad(norad));
                mtvza_products.set_timestamps(mreader.timestamps);

                if (msumr_serial_number == 2)
                    mtvza_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-2_mtvza.json")));
                else if (msumr_serial_number == 3)
                    mtvza_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_mtvza.json")));

                for (int i = 0; i < 30; i++)
                    mtvza_products.images.push_back({"MTVZA-" + std::to_string(i + 1), std::to_string(i + 1), mreader.getChannel(i)});

                mtvza_products.save(directory);
                dataset.products_list.push_back("MTVZA");

                mtvza_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void MeteorInstrumentsDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##meteorinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
                ImGui::Text("MSU-MR");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d", msumr_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(msumr_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MTVZA");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImColor(0, 255, 0), "%d",
                                   mtvza_reader2.lines > mtvza_reader.lines
                                       ? mtvza_reader2.lines
                                       : mtvza_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mtvza_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MeteorInstrumentsDecoderModule::getID()
        {
            return "meteor_instruments";
        }

        std::vector<std::string> MeteorInstrumentsDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MeteorInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MeteorInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop
