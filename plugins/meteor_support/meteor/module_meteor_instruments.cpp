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
#include "nlohmann/json_utils.h"
#include "instruments/msumr/msumr_tlm.h"

namespace meteor
{
    namespace instruments
    {
        MeteorInstrumentsDecoderModule::MeteorInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters),
            bism_reader(getValueOrDefault<int>(parameters["year_override"], -1))
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
            def::SimpleDeframer msumr_deframer(0x0218a7a392dd9abf, 64, 11850 * 8, 10, false);
            def::SimpleDeframer mtvza_deframer(0xFB386A45, 32, 248 * 8, 0, false);
            def::SimpleDeframer mtvza_deframer2(0x38fb456a, 32, 248 * 8, 0, false);
            def::SimpleDeframer bism_deframer(0x71DE2CD8, 32, 88 * 8, 0, false);

            std::vector<double> msumr_timestamps;
            std::vector<uint8_t> msumr_ids;

            nlohmann::json msu_mr_telemetry;
            nlohmann::json msu_mr_telemetry_calib;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                std::vector<std::vector<uint8_t>> msumr_frames;
                std::vector<std::vector<uint8_t>> mtvza_frames, mtvza_frames2;
                std::vector<std::vector<uint8_t>> bism_frames;

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
                for (std::vector<uint8_t> &frame : bism_frames)
                    bism_reader.work(frame.data());

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

                    time_t bism_day = bism_reader.get_last_day_moscow();
                    if (bism_day != 0)
                    {
                        double timestamp = bism_day + (msumr_frame[8]) * 3600.0 + (msumr_frame[9]) * 60.0 + (msumr_frame[10] + 0.0) + double(msumr_frame[11] / 255.0);
                        timestamp -= 3 * 3600.0;
                        msumr_timestamps.push_back(timestamp);
                        mtvza_reader.latest_msumr_timestamp = mtvza_reader2.latest_msumr_timestamp = timestamp; // MTVZA doesn't have timestamps of its own, so use MSU-MR's
                    }
                    else
                    {
                        msumr_timestamps.push_back(-1);
                        mtvza_reader.latest_msumr_timestamp = mtvza_reader2.latest_msumr_timestamp = -1;
                    }

                    msumr_ids.push_back(msumr_frame[12] >> 4);
                    parseMSUMRTelemetry(msu_mr_telemetry, msu_mr_telemetry_calib, msumr_timestamps.size() - 1, msumr_frame.data());
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

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            // Identify satellite, and apply per-sat settings...
            int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end(), -1);
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
            else if (msumr_serial_number == 4)
                sat_name = "METEOR-M2-4";

            int norad = 0;
            if (msumr_serial_number == 0)
                norad = 40069; // M2
            else if (msumr_serial_number == 1)
                norad = 0; // M2-1, failed launch
            else if (msumr_serial_number == 2)
                norad = 44387; // M2-2
            else if (msumr_serial_number == 3)
                norad = 57166; // M2-3
            else if (msumr_serial_number == 4)
                norad = 59051; // M2-4, WAITING FOR NORAD

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
                msumr_products.bit_depth = 10;
                msumr_products.has_timestamps = true;
                msumr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                msumr_products.set_tle(satdump::general_tle_registry.get_from_norad_time(norad, dataset.timestamp));

                std::vector<double> filter_timestamps = msumr_timestamps;
                double last = 0;
                for (double &v : filter_timestamps)
                {
                    if (abs(dataset.timestamp - v) > 10000)
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
                else if (msumr_serial_number == 4)
                    msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-4_msumr.json")));

                for (int i = 0; i < 6; i++)
                    msumr_products.images.push_back({"MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), msumr_reader.getChannel(i)});

                nlohmann::json calib_cfg;
                calib_cfg["calibrator"] = "meteor_msumr";
                calib_cfg["vars"]["lrpt"] = false;
                calib_cfg["vars"]["views"] = msumr_reader.calibration_info;
                calib_cfg["vars"]["temps"] = msu_mr_telemetry_calib;

                msumr_products.set_calibration(calib_cfg);
                msumr_products.set_calibration_type(0, satdump::ImageProducts::CALIB_REFLECTANCE);
                msumr_products.set_calibration_type(1, satdump::ImageProducts::CALIB_REFLECTANCE);
                msumr_products.set_calibration_type(2, satdump::ImageProducts::CALIB_REFLECTANCE);
                msumr_products.set_calibration_type(3, satdump::ImageProducts::CALIB_RADIANCE);
                msumr_products.set_calibration_type(4, satdump::ImageProducts::CALIB_RADIANCE);
                msumr_products.set_calibration_type(5, satdump::ImageProducts::CALIB_RADIANCE);

                msumr_products.set_wavenumber(0, 0);
                msumr_products.set_wavenumber(1, 0);
                msumr_products.set_wavenumber(2, 0);
                msumr_products.set_wavenumber(3, 2695.9743);
                msumr_products.set_wavenumber(4, 925.4075);
                msumr_products.set_wavenumber(5, 839.8979);

                msumr_products.set_calibration_default_radiance_range(0, 0, 1);
                msumr_products.set_calibration_default_radiance_range(1, 0, 1);
                msumr_products.set_calibration_default_radiance_range(2, 0, 1);
                msumr_products.set_calibration_default_radiance_range(3, 0.05, 1);
                msumr_products.set_calibration_default_radiance_range(4, 30, 120);
                msumr_products.set_calibration_default_radiance_range(5, 30, 120);

                saveJsonFile(directory + "/telemetry.json", msu_mr_telemetry);
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
                mtvza_products.set_tle(satdump::general_tle_registry.get_from_norad_time(norad, dataset.timestamp));
                mtvza_products.set_timestamps(mreader.timestamps);

                if (msumr_serial_number == 2)
                    mtvza_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-2_mtvza.json")));
                else if (msumr_serial_number == 3)
                    mtvza_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_mtvza.json")));
                else if (msumr_serial_number == 4)
                    mtvza_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-4_mtvza.json")));

                for (int i = 0; i < 30; i++)
                    mtvza_products.images.push_back({"MTVZA-" + std::to_string(i + 1), std::to_string(i + 1), mreader.getChannel(i)});

                mtvza_products.save(directory);
                dataset.products_list.push_back("MTVZA");

                mtvza_status = DONE;
            }

            // BIS-M
            {
                logger->info("----------- BIS-M");
                logger->info("Frames : %d", bism_reader.get_lines());

#ifdef BISM_FULL_DUMP
                bism_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/BIS-M";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                bism_reader.save(directory);
#endif
                bism_status = DONE;
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
                ImGui::TextColored(style::theme.green, "%d", msumr_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(msumr_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MTVZA");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d",
                                   mtvza_reader2.lines > mtvza_reader.lines
                                       ? mtvza_reader2.lines
                                       : mtvza_reader.lines);
                ImGui::TableSetColumnIndex(2);
                drawStatus(mtvza_status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("BIS-M");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(style::theme.green, "%d", bism_reader.get_lines());
                ImGui::TableSetColumnIndex(2);
                drawStatus(bism_status);

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

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
