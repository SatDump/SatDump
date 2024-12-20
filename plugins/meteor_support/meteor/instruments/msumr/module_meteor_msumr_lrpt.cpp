#include "module_meteor_msumr_lrpt.h"
#include <fstream>
#include "common/ccsds/ccsds_time.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "logger.h"
#include <filesystem>
#include <cstring>
#include "lrpt_msumr_reader.h"
#include "imgui/imgui.h"
#include "../../meteor.h"
#include "products/image_products.h"
#include <ctime>
#include "products/dataset.h"
#include "resources.h"
#include "common/utils.h"
#include "nlohmann/json_utils.h"
#include "msumr_tlm.h"

#define BUFFER_SIZE 8192

namespace meteor
{
    namespace msumr
    {
        void createMSUMRProduct(satdump::ImageProducts &product, double timestamp, int norad, int msumr_serial_number,
                                nlohmann::json &calib_cfg, uint8_t lrpt_channels)
        {
            product.bit_depth = 8;
            product.instrument_name = "msu_mr";
            product.has_timestamps = true;
            product.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
            product.set_tle(satdump::general_tle_registry.get_from_norad_time(norad, timestamp));
            if (msumr_serial_number == 0) // M2
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr_lrpt.json")));
            else if (msumr_serial_number == 3) // M2-3
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_msumr_lrpt.json")));
            else if (msumr_serial_number == 4) // M2-4
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-4_msumr_lrpt.json")));
            else // Default to M2
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr_lrpt.json")));

            bool has_calib = false;
            for (auto &this_temp : calib_cfg["vars"]["temps"])
            {
                if (!this_temp.is_null())
                {
                    has_calib = true;
                    break;
                }
            }

            if (has_calib)
            {
                product.set_calibration(calib_cfg);
                int next_channel = 0;
                if ((1 << 0) & lrpt_channels)
                {
                    product.set_calibration_type(next_channel, satdump::ImageProducts::CALIB_REFLECTANCE);
                    product.set_wavenumber(next_channel, 0);
                    product.set_calibration_default_radiance_range(next_channel, 0, 1);
                    next_channel++;
                }
                if ((1 << 1) & lrpt_channels)
                {
                    product.set_calibration_type(next_channel, satdump::ImageProducts::CALIB_REFLECTANCE);
                    product.set_wavenumber(next_channel, 0);
                    product.set_calibration_default_radiance_range(next_channel, 0, 1);
                    next_channel++;
                }
                if ((1 << 2) & lrpt_channels)
                {
                    product.set_calibration_type(next_channel, satdump::ImageProducts::CALIB_REFLECTANCE);
                    product.set_wavenumber(next_channel, 0);
                    product.set_calibration_default_radiance_range(next_channel, 0, 1);
                    next_channel++;
                }
                if ((1 << 3) & lrpt_channels)
                {
                    product.set_calibration_type(next_channel, satdump::ImageProducts::CALIB_RADIANCE);
                    product.set_wavenumber(next_channel, 2695.9743);
                    product.set_calibration_default_radiance_range(next_channel, 0.05, 1);
                    next_channel++;
                }
                if ((1 << 4) & lrpt_channels)
                {
                    product.set_calibration_type(next_channel, satdump::ImageProducts::CALIB_RADIANCE);
                    product.set_wavenumber(next_channel, 925.4075);
                    product.set_calibration_default_radiance_range(next_channel, 30, 120);
                    next_channel++;
                }
                if ((1 << 5) & lrpt_channels)
                {
                    product.set_calibration_type(next_channel, satdump::ImageProducts::CALIB_RADIANCE);
                    product.set_wavenumber(next_channel, 839.8979);
                    product.set_calibration_default_radiance_range(next_channel, 30, 120);
                    next_channel++;
                }
            }
            else
                logger->warn("Analog telemetry missing from transmission. Calibration is disabled!");
        }

        METEORMSUMRLRPTDecoderModule::METEORMSUMRLRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void METEORMSUMRLRPTDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-MR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            bool meteorm2x_mode = d_parameters["m2x_mode"].get<bool>();

            time_t lastTime = 0;

            uint8_t cadu[1024];

            ccsds::ccsds_aos::Demuxer ccsds_demuxer(882, true);

            lrpt::MSUMRReader msureader(meteorm2x_mode);

            logger->info("Demultiplexing and deframing...");

            std::map<double, size_t> telemetry_timestamps;
            std::vector<uint8_t> msumr_ids;
            nlohmann::json msu_mr_telemetry;
            nlohmann::json msu_mr_telemetry_calib;
            std::vector<std::vector<std::pair<uint16_t, uint16_t>>> calibration_info(6);

            while (!data_in.eof())
            {
                data_in.read((char *)&cadu, 1024);

                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                if (vcdu.vcid == 5)
                {
                    std::vector<ccsds::CCSDSPacket> frames = ccsds_demuxer.work(cadu);
                    for (ccsds::CCSDSPacket pkt : frames)
                    {
                        msureader.work(pkt);

                        if (pkt.header.apid == 70 && pkt.payload.size() >= 62)
                        {
                            // Telemetry Timestamp
                            if (meteorm2x_mode)
                                telemetry_timestamps[ccsds::parseCCSDSTimeFullRaw(&pkt.payload.data()[0], 11322)] = msumr_ids.size();
                            else
                                telemetry_timestamps[ccsds::parseCCSDSTimeFullRaw(&pkt.payload.data()[0], 0)] = msumr_ids.size();

                            // ID parsing
                            uint8_t msumr_id = pkt.payload[8 + 12] >> 4;
                            msumr_ids.push_back(msumr_id);
                            parseMSUMRTelemetry(msu_mr_telemetry, msu_mr_telemetry_calib, msumr_ids.size() - 1, &pkt.payload[8]);

                            // Convert calibration data
                            uint16_t words10_bits[12];
                            for (int n = 0; n < 3; n++)
                            {
                                int bitpos = 43 + n * 5;
                                // Convert 5 bytes to 4 10-bits values
                                words10_bits[n * 4 + 0] = ((pkt.payload[bitpos] << 2) | (pkt.payload[bitpos + 1] >> 6));
                                words10_bits[n * 4 + 1] = (((pkt.payload[bitpos + 1] % 64) << 4) | (pkt.payload[bitpos + 2] >> 4));
                                words10_bits[n * 4 + 2] = (((pkt.payload[bitpos + 2] % 16) << 6) | (pkt.payload[bitpos + 3] >> 2));
                                words10_bits[n * 4 + 3] = (((pkt.payload[bitpos + 3] % 4) << 8) | pkt.payload[bitpos + 4]);
                            }

                            for (int channel = 0; channel < 6; channel++)
                                calibration_info[channel].push_back({words10_bits[channel * 2], words10_bits[channel * 2 + 1]});
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

            data_in.close();
            logger->info("Writing images.... (Can take a while)");

            // Identify satellite, and apply per-sat settings...
            int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end(), -1);
            msumr_ids.clear();
            logger->trace("MSU-MR ID %d", msumr_serial_number);

            // Override, if a valid override is provided
            if (d_parameters.contains("satellite_number") && d_parameters["satellite_number"].is_string())
            {
                std::string override_sat = d_parameters["satellite_number"];
                std::transform(override_sat.begin(), override_sat.end(), override_sat.begin(),
                               [](unsigned char c)
                               { return std::toupper(c); });

                if (override_sat == "M2")
                    msumr_serial_number = 0;
                else if (override_sat == "M2-1")
                    msumr_serial_number = 1;
                else if (override_sat == "M2-2")
                    msumr_serial_number = 2;
                else if (override_sat == "M2-3")
                    msumr_serial_number = 3;
                else if (override_sat == "M2-4")
                    msumr_serial_number = 4;
                else if (override_sat != "AUTO")
                    logger->warn("Invalid METEOR satellite \"%s\" provided. Using transmitted ID!",
                                 d_parameters["satellite_number"].get<std::string>().c_str());
            }

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
                norad = 59051; // M2-4

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            nlohmann::json calib_cfg;
            uint8_t lrpt_channels = 0;
            calib_cfg["calibrator"] = "meteor_msumr";
            calib_cfg["vars"]["lrpt"] = true;
            calib_cfg["vars"]["views"] = nlohmann::json::array();
            calib_cfg["vars"]["temps"] = nlohmann::json::array();

            satdump::ImageProducts msumr_products;
            std::vector<satdump::ImageProducts::ImageHolder> msumr_images;
            for (int i = 0; i < 6; i++)
            {
                image::Image img = msureader.getChannel(i);
                logger->info("MSU-MR Channel %d Lines  : %zu", i + 1, img.height());
                if (img.size() > 0)
                {
                    msumr_images.push_back({ "MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), img, {}, 8 });
                    lrpt_channels |= 1 << i;
                    nlohmann::json channel_views = nlohmann::json::array();
                    channel_views[0] = nlohmann::json::array();
                    channel_views[1] = nlohmann::json::array();
                    for (double &this_timestamp : msureader.timestamps)
                    {
                        if (telemetry_timestamps.count(this_timestamp) == 0)
                        {
                            channel_views[0].push_back(0);
                            channel_views[1].push_back(0);
                        }
                        else
                        {
                            channel_views[0].push_back(calibration_info[i][telemetry_timestamps[this_timestamp]].first);
                            channel_views[1].push_back(calibration_info[i][telemetry_timestamps[this_timestamp]].second);
                        }
                    }
                    calib_cfg["vars"]["views"].push_back(channel_views);
                }
            }

            for (double &this_timestamp : msureader.timestamps)
            {
                if (telemetry_timestamps.count(this_timestamp) == 0)
                    calib_cfg["vars"]["temps"].push_back(nullptr);
                else
                    calib_cfg["vars"]["temps"].push_back(msu_mr_telemetry_calib[telemetry_timestamps[this_timestamp]]);
            }

            createMSUMRProduct(msumr_products, get_median(msureader.timestamps), norad, msumr_serial_number, calib_cfg, lrpt_channels);
            msumr_products.images.swap(msumr_images);
            msumr_products.set_timestamps(msureader.timestamps);
            msumr_products.save(directory);
            msumr_products.images.clear(); // Free up memory

            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = get_median(msureader.timestamps);
            dataset.products_list.push_back("MSU-MR");

            if (d_parameters.contains("fill_missing") && d_parameters["fill_missing"])
            {
                std::string fill_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-MR (Filled)";
                if (!std::filesystem::exists(fill_directory))
                    std::filesystem::create_directory(fill_directory);

                size_t max_fill_lines = 50;
                if (d_parameters.contains("max_fill_lines"))
                    max_fill_lines = d_parameters["max_fill_lines"];

                // Fill missing calibration data
                for (size_t channel = 0; channel < calib_cfg["vars"]["views"].size(); channel++)
                {
                    int last_good_view = -1;
                    bool found_bad_view = false;
                    for (size_t i = 0; i < calib_cfg["vars"]["views"][channel][0].size(); i++)
                    {
                        if (found_bad_view)
                        {
                            if (calib_cfg["vars"]["views"][channel][0][i].get<int>() != 0)
                            {
                                if (last_good_view != -1)
                                {
                                    for (size_t j = 1; j < i - last_good_view; j++)
                                    {
                                        calib_cfg["vars"]["views"][channel][0][last_good_view + j] = (int)((int)calib_cfg["vars"]["views"][channel][0][last_good_view] +
                                                                                                           ((int)calib_cfg["vars"]["views"][channel][0][i] -
                                                                                                            (int)calib_cfg["vars"]["views"][channel][0][last_good_view]) *
                                                                                                               ((double)j / double(i - last_good_view)));
                                        calib_cfg["vars"]["views"][channel][1][last_good_view + j] = (int)((int)calib_cfg["vars"]["views"][channel][1][last_good_view] +
                                                                                                           ((int)calib_cfg["vars"]["views"][channel][1][i] -
                                                                                                            (int)calib_cfg["vars"]["views"][channel][1][last_good_view]) *
                                                                                                               ((double)j / double(i - last_good_view)));
                                    }
                                }
                                last_good_view = i;
                                found_bad_view = false;
                            }
                        }
                        else if (calib_cfg["vars"]["views"][channel][0][i].get<int>() == 0)
                            found_bad_view = true;
                        else
                            last_good_view = i;
                    }
                }

                satdump::ImageProducts filled_products;
                createMSUMRProduct(filled_products, get_median(msureader.timestamps), norad, msumr_serial_number, calib_cfg, lrpt_channels);
                for (int i = 0; i < 6; i++)
                {
                    image::Image img = msureader.getChannel(i, max_fill_lines);
                    if (img.size() > 0)
                        filled_products.images.push_back({"MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), img, {}, 8});
                }
                filled_products.set_timestamps(msureader.timestamps);
                filled_products.save(fill_directory);
                dataset.products_list.push_back("MSU-MR (Filled)");
            }

            saveJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/telemetry.json", msu_mr_telemetry);
            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void METEORMSUMRLRPTDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR MSU-MR LRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

            ImGui::End();
        }

        std::string METEORMSUMRLRPTDecoderModule::getID()
        {
            return "meteor_msumr_lrpt";
        }

        std::vector<std::string> METEORMSUMRLRPTDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> METEORMSUMRLRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<METEORMSUMRLRPTDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msumr
} // namespace meteor
