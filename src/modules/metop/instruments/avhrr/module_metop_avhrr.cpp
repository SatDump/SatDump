#include "module_metop_avhrr.h"
#include <fstream>
#include "avhrr_reader.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

MetOpAVHRRDecoderModule::MetOpAVHRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
{
}

void MetOpAVHRRDecoderModule::process()
{
    size_t filesize = getFilesize(d_input_file);
    std::ifstream data_in(d_input_file, std::ios::binary);

    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

    logger->info("Using input frames " + d_input_file);
    logger->info("Decoding to " + directory);

    time_t lastTime = 0;

    libccsds::Demuxer ccsdsDemuxer(882, true);
    AVHRRReader reader;
    uint64_t avhrr_cadu = 0, ccsds = 0, avhrr_ccsds = 0;
    libccsds::CADU cadu;

    logger->info("Demultiplexing and deframing...");

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)&cadu, 1024);

        // Parse this transport frame
        libccsds::VCDU vcdu = libccsds::parseVCDU(cadu);

        // Right channel? (VCID 30/42 is MODIS)
        if (vcdu.vcid == 9)
        {
            avhrr_cadu++;

            // Demux
            std::vector<libccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

            // Count frames
            ccsds += ccsdsFrames.size();

            // Push into processor (filtering APID 103 and 104)
            for (libccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 103 || pkt.header.apid == 104)
                {
                    avhrr_ccsds++;
                    reader.work(pkt);
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

    logger->info("VCID 9 (AVHRR) Frames  : " + std::to_string(avhrr_cadu));
    logger->info("CCSDS Frames           : " + std::to_string(ccsds));
    logger->info("AVHRR CCSDS Frames     : " + std::to_string(avhrr_ccsds));
    logger->info("AVHRR Lines            : " + std::to_string(reader.lines));

    logger->info("Writing images.... (Can take a while)");

    if (!std::filesystem::exists(directory))
        std::filesystem::create_directory(directory);

    cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
    cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
    cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
    cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
    cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);

    logger->info("Channel 1...");
    image1.save_png(std::string(directory + "/AVHRR-1.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-1.png");

    logger->info("Channel 2...");
    image2.save_png(std::string(directory + "/AVHRR-2.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-2.png");

    logger->info("Channel 3...");
    image3.save_png(std::string(directory + "/AVHRR-3.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-3.png");

    logger->info("Channel 4...");
    image4.save_png(std::string(directory + "/AVHRR-4.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-4.png");

    logger->info("Channel 5...");
    image5.save_png(std::string(directory + "/AVHRR-5.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-5.png");

    logger->info("221 Composite...");
    cimg_library::CImg<unsigned short> image221(2048, reader.lines, 1, 3);
    {
        image221.draw_image(0, 0, 0, 0, image2);
        image221.draw_image(0, 0, 0, 1, image2);
        image221.draw_image(0, 0, 0, 2, image1);
    }
    image221.save_png(std::string(directory + "/AVHRR-RGB-221.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-RGB-221.png");
    image221.equalize(1000);
    image221.normalize(0, std::numeric_limits<unsigned char>::max());
    image221.save_png(std::string(directory + "/AVHRR-RGB-221-EQU.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-RGB-221-EQU.png");

    logger->info("321 Composite...");
    cimg_library::CImg<unsigned short> image321(2048, reader.lines, 1, 3);
    {
        image321.draw_image(0, 0, 0, 0, image3);
        image321.draw_image(0, 0, 0, 1, image2);
        image321.draw_image(0, 0, 0, 2, image1);
    }
    image321.save_png(std::string(directory + "/AVHRR-RGB-321.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-RGB-321.png");
    image321.equalize(1000);
    image321.normalize(0, std::numeric_limits<unsigned char>::max());
    image321.save_png(std::string(directory + "/AVHRR-RGB-321-EQU.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-RGB-321-EQU.png");
}

std::string MetOpAVHRRDecoderModule::getID()
{
    return "metop_avhrr";
}

std::vector<std::string> MetOpAVHRRDecoderModule::getParameters()
{
    return {};
}

std::shared_ptr<ProcessingModule> MetOpAVHRRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<MetOpAVHRRDecoderModule>(input_file, output_file_hint, parameters);
}