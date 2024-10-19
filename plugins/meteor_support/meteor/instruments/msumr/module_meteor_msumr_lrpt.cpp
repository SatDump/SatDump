#include "module_meteor_msumr_lrpt.h"
#include <fstream>
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
        void createMSUMRProduct(satdump::ImageProducts &product, double timestamp, int norad, int msumr_serial_number)
        {
            product.instrument_name = "msu_mr";
            product.has_timestamps = true;
            product.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
            product.needs_correlation = true;
            product.set_tle(satdump::general_tle_registry.get_from_norad_time(norad, timestamp));
            if (msumr_serial_number == 0) // M2
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr_lrpt.json")));
            else if (msumr_serial_number == 3) // M2-3
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_msumr_lrpt.json")));
            else if (msumr_serial_number == 4) // M2-4
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-4_msumr_lrpt.json")));
            else // Default to M2
                product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr_lrpt.json")));
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

            std::vector<uint8_t> msumr_ids;
            nlohmann::json msu_mr_telemetry;
            nlohmann::json msu_mr_telemetry_calib;

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

                        if (pkt.header.apid == 70 && pkt.payload.size() >= 13)
                        {
                            uint8_t msumr_id = pkt.payload[8 + 12] >> 4;
                            msumr_ids.push_back(msumr_id);

                            parseMSUMRTelemetry(msu_mr_telemetry, msu_mr_telemetry_calib, msumr_ids.size() - 1, &pkt.payload[8]);
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
                    [](unsigned char c) { return std::toupper(c); });

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

            satdump::ImageProducts msumr_products;
            std::vector<satdump::ImageProducts::ImageHolder> msumr_images;
            for (int i = 0; i < 6; i++)
            {
                image::Image img = msureader.getChannel(i);
                logger->info("MSU-MR Channel %d Lines  : %zu", i + 1, img.height());
                if (img.size() > 0)
                    msumr_images.push_back({"MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), img, msureader.timestamps, 8});
            }
            createMSUMRProduct(msumr_products, get_median(msureader.timestamps), norad, msumr_serial_number);
            msumr_products.images.swap(msumr_images);
            msumr_products.save(directory);

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

                satdump::ImageProducts filled_products;
                createMSUMRProduct(filled_products, get_median(msureader.timestamps), norad, msumr_serial_number);
                for (int i = 0; i < 6; i++)
                {
                    image::Image img = msureader.getChannel(i, max_fill_lines);
                    if (img.size() > 0)
                        filled_products.images.push_back({"MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), img, msureader.timestamps, 8});
                }
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
