#include "module_meteor_msumr_lrpt.h"
#include <fstream>
#include "common/ccsds/ccsds_weather/demuxer.h"
#include "common/ccsds/ccsds_weather/vcdu.h"
#include "logger.h"
#include <filesystem>
#include <cstring>
#include "lrpt_msumr_reader.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "../../meteor.h"
#include "products/image_products.h"
#include <ctime>
#include "products/dataset.h"
#include "resources.h"
#include "common/utils.h"

#define BUFFER_SIZE 8192

namespace meteor
{
    namespace msumr
    {
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

            ccsds::ccsds_weather::Demuxer ccsds_demuxer(882, true);

            lrpt::MSUMRReader msureader(meteorm2x_mode);

            logger->info("Demultiplexing and deframing...");

            std::vector<uint8_t> msumr_ids;

            while (!data_in.eof())
            {
                data_in.read((char *)&cadu, 1024);

                ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

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

            logger->info("MSU-MR Channel 1 Lines  : " + std::to_string(msureader.getChannel(0).height()));
            logger->info("MSU-MR Channel 2 Lines  : " + std::to_string(msureader.getChannel(1).height()));
            logger->info("MSU-MR Channel 3 Lines  : " + std::to_string(msureader.getChannel(2).height()));
            logger->info("MSU-MR Channel 4 Lines  : " + std::to_string(msureader.getChannel(3).height()));
            logger->info("MSU-MR Channel 5 Lines  : " + std::to_string(msureader.getChannel(4).height()));
            logger->info("MSU-MR Channel 6 Lines  : " + std::to_string(msureader.getChannel(5).height()));

            logger->info("Writing images.... (Can take a while)");

            // Identify satellite, and apply per-sat settings...
            int msumr_serial_number = most_common(msumr_ids.begin(), msumr_ids.end());
            msumr_ids.clear();

            logger->trace("MSU-MR ID %d", msumr_serial_number);

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
            dataset.timestamp = get_median(msureader.timestamps);

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            satdump::ImageProducts msumr_products;
            msumr_products.instrument_name = "msu_mr";
            msumr_products.has_timestamps = true;
            msumr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
            msumr_products.needs_correlation = true;
            msumr_products.set_tle(satdump::general_tle_registry.get_from_norad(norad));
            if (msumr_serial_number == 0) // M2
                msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr_lrpt.json")));
            else if (msumr_serial_number == 3) // M2-3
                msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-3_msumr_lrpt.json")));
            else // Default to M2
                msumr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2_msumr_lrpt.json")));

            for (int i = 0; i < 6; i++)
            {
                image::Image<uint16_t> img = msureader.getChannel(i).to16bits();
                if (img.size() > 0)
                    msumr_products.images.push_back({"MSU-MR-" + std::to_string(i + 1), std::to_string(i + 1), img, msureader.timestamps, 8});
            }

            msumr_products.save(directory);
            dataset.products_list.push_back("MSU-MR");

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void METEORMSUMRLRPTDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR MSU-MR LRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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
