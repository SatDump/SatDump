#include "module_meteor_msumr_lrpt.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include <cstring>
#include "lrpt_msumr_reader.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "../../meteor.h"
#include "products/image_products.h"
#include <ctime>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

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

            time_t lastTime = 0;

            uint8_t cadu[1024];

            ccsds::ccsds_1_0_1024::Demuxer ccsds_demuxer(882, true);

            lrpt::MSUMRReader msureader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                data_in.read((char *)&cadu, 1024);

                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.vcid == 5)
                {
                    std::vector<ccsds::CCSDSPacket> frames = ccsds_demuxer.work(cadu);
                    for (ccsds::CCSDSPacket pkt : frames)
                    {
                        msureader.work(pkt);
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

            logger->info("MSU-MR Channel 1 Lines  : " + std::to_string(msureader.getChannel(0).height()));
            logger->info("MSU-MR Channel 2 Lines  : " + std::to_string(msureader.getChannel(1).height()));
            logger->info("MSU-MR Channel 3 Lines  : " + std::to_string(msureader.getChannel(2).height()));
            logger->info("MSU-MR Channel 4 Lines  : " + std::to_string(msureader.getChannel(3).height()));
            logger->info("MSU-MR Channel 5 Lines  : " + std::to_string(msureader.getChannel(4).height()));
            logger->info("MSU-MR Channel 6 Lines  : " + std::to_string(msureader.getChannel(5).height()));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            satdump::ImageProducts msumr_products;
            msumr_products.instrument_name = "msu_mr";
            msumr_products.has_timestamps = true;
            msumr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
            msumr_products.needs_correlation = true;
            // msumr_products.set_timestamps(msumr_timestamps);

            for (int i = 0; i < 6; i++)
            {
                image::Image<uint16_t> img = msureader.getChannel(i).to16bits();
                if (img.size() > 0)
                    msumr_products.images.push_back({"MSU-MR-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), img, msureader.timestamps, 8});
            }

            msumr_products.save(directory);

            // TODO : Add detection system!!!!!!!!
            // Currently not *that* mandatory as only M2 is active on VHF
            // int norad = 40069;
        }

        void METEORMSUMRLRPTDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR MSU-MR LRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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
