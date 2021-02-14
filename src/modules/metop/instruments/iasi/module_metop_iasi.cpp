#include "module_metop_iasi.h"
#include <fstream>
#include "iasi_imaging_reader.h"
#include "iasi_reader.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace iasi
    {
        MetOpIASIDecoderModule::MetOpIASIDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                              write_all(std::stoi(parameters["write_all"]))
        {
        }

        void MetOpIASIDecoderModule::process()
        {
            size_t filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IASI";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            libccsds::Demuxer ccsdsDemuxer(882, true);

            IASIReader iasireader;
            IASIIMGReader iasireader_img;

            uint64_t iasi_cadu = 0, ccsds = 0, iasi_ccsds = 0;
            libccsds::CADU cadu;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                libccsds::VCDU vcdu = libccsds::parseVCDU(cadu);

                // Right channel? (VCID 10 is IASI)
                if (vcdu.vcid == 10)
                {
                    iasi_cadu++;

                    // Demux
                    std::vector<libccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (libccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 150)
                        {
                            iasireader_img.work(pkt);
                            iasi_ccsds++;
                        }
                        else if (pkt.header.apid == 130 || pkt.header.apid == 135 || pkt.header.apid == 140 || pkt.header.apid == 145)
                        {
                            iasireader.work(pkt);
                            iasi_ccsds++;
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

            logger->info("VCID 10 (IASI) Frames : " + std::to_string(iasi_cadu));
            logger->info("CCSDS Frames          :  " + std::to_string(ccsds));
            logger->info("IASI Frames           : " + std::to_string(iasi_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            if (write_all)
            {
                if (!std::filesystem::exists(directory + "/IASI_all"))
                    std::filesystem::create_directory(directory + "/IASI_ALL");

                for (int i = 0; i < 8461; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    iasireader.getChannel(i).save_png(std::string(directory + "/IASI_ALL/IASI-" + std::to_string(i + 1) + ".png").c_str());
                    d_output_files.push_back(directory + "/IASI_ALL/IASI-" + std::to_string(i + 1) + ".png");
                }
            }

            logger->info("Channel IR imaging...");
            iasireader_img.getIRChannel().save_png(std::string(directory + "/IASI-IMG.png").c_str());
            d_output_files.push_back(directory + "/IASI-IMG.png");

            // Output a few nice composites as well
            logger->info("Global Composite...");
            int all_width_count = 150;
            int all_height_count = 60;
            cimg_library::CImg<unsigned short> imageAll(60 * all_width_count, iasireader.getChannel(0).height() * all_height_count, 1, 1);
            {
                int height = iasireader.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 8461)
                            break;

                        imageAll.draw_image(60 * column, height * row, 0, 0, iasireader.getChannel(row * all_width_count + column));
                    }
                }
            }
            imageAll.save_png(std::string(directory + "/IASI-ALL.png").c_str());
            d_output_files.push_back(directory + "/IASI-ALL.png");
        }

        std::string MetOpIASIDecoderModule::getID()
        {
            return "metop_iasi";
        }

        std::vector<std::string> MetOpIASIDecoderModule::getParameters()
        {
            return {"write_all"};
        }

        std::shared_ptr<ProcessingModule> MetOpIASIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<MetOpIASIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace iasi
} // namespace metop