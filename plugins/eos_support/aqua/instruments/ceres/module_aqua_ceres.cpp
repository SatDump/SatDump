#include "module_aqua_ceres.h"
#include <fstream>
#include "ceres_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace aqua
{
    namespace ceres
    {
        AquaCERESDecoderModule::AquaCERESDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AquaCERESDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/CERES";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t ceres_cadu = 0, ccsds = 0, modis_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer1, ccsdsDemuxer2;

            CERESReader reader1, reader2;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 10-15 is CERES)
                if (vcdu.vcid == 10)
                {
                    ceres_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer1.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 141)
                        {
                            modis_ccsds++;
                            reader1.work(pkt);
                        }
                    }
                }
                else if (vcdu.vcid == 15)
                {
                    ceres_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer2.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 157)
                        {
                            modis_ccsds++;
                            reader2.work(pkt);
                        }
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

            logger->info("VCID (CERES) Frames    : " + std::to_string(ceres_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("CERES CCSDS Frames     : " + std::to_string(modis_ccsds));
            logger->info("CERES 1 Lines          : " + std::to_string(reader1.lines));
            logger->info("CERES 2 Lines          : " + std::to_string(reader2.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            image::Image<uint16_t> image_shortwave1 = reader1.getImage(0);
            image::Image<uint16_t> image_longwave1 = reader1.getImage(1);
            image::Image<uint16_t> image_total1 = reader1.getImage(2);
            image::Image<uint16_t> image_shortwave2 = reader2.getImage(0);
            image::Image<uint16_t> image_longwave2 = reader2.getImage(1);
            image::Image<uint16_t> image_total2 = reader2.getImage(2);

            // Resize to be... Readable?
            image_shortwave1.resize(image_shortwave1.width(), image_shortwave1.height() * 7);
            image_longwave1.resize(image_longwave1.width(), image_longwave1.height() * 7);
            image_total1.resize(image_total1.width(), image_total1.height() * 7);
            image_shortwave2.resize(image_shortwave2.width(), image_shortwave2.height() * 7);
            image_longwave2.resize(image_longwave2.width(), image_longwave2.height() * 7);
            image_total2.resize(image_total2.width(), image_total2.height() * 7);

            // Equalize
            image_shortwave1.equalize();
            image_longwave1.equalize();
            image_total1.equalize();
            image_shortwave2.equalize();
            image_longwave2.equalize();
            image_total2.equalize();

            logger->info("Shortwave Channel 1...");
            WRITE_IMAGE(image_shortwave1, directory + "/CERES1-SHORTWAVE.png");

            logger->info("Longwave Channel 1...");
            WRITE_IMAGE(image_longwave1, directory + "/CERES1-LONGWAVE.png");

            logger->info("Total Channel 1...");
            WRITE_IMAGE(image_total1, directory + "/CERES1-TOTAL.png");

            // Output a few nice composites as well
            logger->info("Global Composite 1...");
            image::Image<uint16_t> imageAll1(image_shortwave1.width() + image_longwave1.width() + image_total1.width(), image_shortwave1.height(), 1);
            {
                imageAll1.draw_image(0, image_shortwave1);
                imageAll1.draw_image(0, image_longwave1, image_shortwave1.width());
                imageAll1.draw_image(0, image_total1, image_shortwave1.width() * 2);
            }
            WRITE_IMAGE(imageAll1, directory + "/CERES1-ALL.png");

            logger->info("Shortwave Channel 2...");
            WRITE_IMAGE(image_shortwave2, directory + "/CERES2-SHORTWAVE.png");

            logger->info("Longwave Channel 2...");
            WRITE_IMAGE(image_longwave2, directory + "/CERES2-LONGWAVE.png");

            logger->info("Total Channel 2...");
            WRITE_IMAGE(image_total2, directory + "/CERES2-TOTAL.png");

            // Output a few nice composites as well
            logger->info("Global Composite 2...");
            image::Image<uint16_t> imageAll2(image_shortwave2.width() + image_longwave2.width() + image_total2.width(), image_shortwave2.height(), 1);
            {
                imageAll2.draw_image(0, image_shortwave2);
                imageAll2.draw_image(0, image_longwave2, image_shortwave2.width());
                imageAll2.draw_image(0, image_total2, image_shortwave2.width() * 2);
            }
            WRITE_IMAGE(imageAll2, directory + "/CERES2-ALL.png");
        }

        void AquaCERESDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("Aqua CERES Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string AquaCERESDecoderModule::getID()
        {
            return "aqua_ceres";
        }

        std::vector<std::string> AquaCERESDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> AquaCERESDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<AquaCERESDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace ceres
} // namespace aqua