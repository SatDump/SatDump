#include "module_aqua_airs.h"
#include <fstream>
#include "airs_reader.h"
#include "modules/common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "modules/common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace aqua
{
    namespace airs
    {
        AquaAIRSDecoderModule::AquaAIRSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void AquaAIRSDecoderModule::process()
        {
            size_t filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AIRS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t airs_cadu = 0, ccsds = 0, airs_ccsds = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer = ccsds::ccsds_1_0_1024::Demuxer();

            // Readers
            AIRSReader airs_reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 35 is AIRS)
                if (vcdu.vcid == 35)
                {
                    airs_cadu++;

                    // Demux
                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 404)
                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 404)
                        {
                            airs_reader.work(pkt);
                            airs_ccsds++;
                        }
                    }
                }

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("VCID 35 (AIRS) Frames : " + std::to_string(airs_cadu));
            logger->info("CCSDS Frames          : " + std::to_string(ccsds));
            logger->info("AIRS Frames           : " + std::to_string(airs_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 4; i++)
            {
                logger->info("HD Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(airs_reader.getHDChannel(i), directory + "/AIRS-HD-" + std::to_string(i + 1) + ".png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            int all_width_count = 100;
            int all_height_count = 27;
            cimg_library::CImg<unsigned short> imageAll(90 * all_width_count, airs_reader.getChannel(0).height() * all_height_count, 1, 1);
            {
                int height = airs_reader.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 2666)
                            break;

                        imageAll.draw_image(90 * column, height * row, 0, 0, airs_reader.getChannel(row * all_width_count + column));
                    }
                }
            }
            WRITE_IMAGE(imageAll, directory + "/AIRS-ALL.png");

            logger->info("HD 221 Composite...");
            cimg_library::CImg<unsigned short> image221(90 * 8, airs_reader.getHDChannel(0).height(), 1, 3);
            {
                image221.draw_image(0, 0, 0, 0, airs_reader.getHDChannel(1));
                image221.draw_image(0, 0, 0, 1, airs_reader.getHDChannel(1));
                image221.draw_image(0, 0, 0, 2, airs_reader.getHDChannel(0));
            }
            WRITE_IMAGE(image221, directory + "/AIRS-HD-RGB-221.png");

            logger->info("HD 332 Composite...");
            cimg_library::CImg<unsigned short> image332(90 * 8, airs_reader.getHDChannel(0).height(), 1, 3);
            {
                image332.draw_image(0, 0, 0, 0, airs_reader.getHDChannel(2));
                image332.draw_image(0, 0, 0, 1, airs_reader.getHDChannel(2));
                image332.draw_image(0, 0, 0, 2, airs_reader.getHDChannel(1));
            }
            WRITE_IMAGE(image332, directory + "/AIRS-HD-RGB-332.png");

            logger->info("HD 321 Composite...");
            cimg_library::CImg<unsigned short> image321(90 * 8, airs_reader.getHDChannel(0).height(), 1, 3);
            {
                image321.draw_image(0, 0, 0, 0, airs_reader.getHDChannel(2));
                image321.draw_image(0, 0, 0, 1, airs_reader.getHDChannel(1));
                image321.draw_image(0, 0, 0, 2, airs_reader.getHDChannel(0));
            }
            WRITE_IMAGE(image321, directory + "/AIRS-HD-RGB-321.png");
        }

        std::string AquaAIRSDecoderModule::getID()
        {
            return "aqua_airs";
        }

        std::vector<std::string> AquaAIRSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> AquaAIRSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<AquaAIRSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace airs
} // namespace aqua