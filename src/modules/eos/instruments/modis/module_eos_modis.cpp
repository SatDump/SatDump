#include "module_eos_modis.h"
#include <fstream>
#include "modis_reader.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

template <class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
T most_common(InputIt begin, InputIt end)
{
    std::map<T, int> counts;
    for (InputIt it = begin; it != end; ++it)
    {
        if (counts.find(*it) != counts.end())
            ++counts[*it];
        else
            counts[*it] = 1;
    }
    return std::max_element(counts.begin(), counts.end(), [](const std::pair<T, int> &pair1, const std::pair<T, int> &pair2) { return pair1.second < pair2.second; })->first;
}

EOSMODISDecoderModule::EOSMODISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                    terra(std::stoi(parameters["terra_mode"]))
{
}

void EOSMODISDecoderModule::process()
{
    size_t filesize = getFilesize(d_input_file);
    std::ifstream data_in(d_input_file, std::ios::binary);

    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MODIS";

    logger->info("Using input frames " + d_input_file);
    logger->info("Decoding to " + directory);

    time_t lastTime = 0;

    // Read buffer
    libccsds::CADU cadu;

    // Counters
    uint64_t modis_cadu = 0, ccsds = 0, modis_ccsds = 0;

    logger->info("Computing time statistics for filtering...");

    // Time values
    uint16_t common_day;
    uint32_t common_coarse;

    // Compute time statistics for filtering later on.
    // The idea of doing it that way originates from Fred's weathersat software (readbin_modis)
    {
        std::vector<uint16_t> dayCounts;
        std::vector<uint32_t> coarseCounts;

        std::ifstream data_in_tmp(d_input_file, std::ios::binary);
        libccsds::Demuxer ccsdsDemuxer;

        while (!data_in_tmp.eof())
        {
            // Read buffer
            data_in_tmp.read((char *)&cadu, 1024);

            // Parse this transport frame
            libccsds::VCDU vcdu = libccsds::parseVCDU(cadu);

            // Right channel? (VCID 30/42 is MODIS)
            if (vcdu.vcid == (terra ? 42 : 30))
            {
                modis_cadu++;

                // Demux
                std::vector<libccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                // Push into processor (filtering APID 64)
                for (libccsds::CCSDSPacket &pkt : ccsdsFrames)
                {
                    if (pkt.header.apid == 64)
                    {
                        // Filter out bad packets
                        if (pkt.payload.size() < 10)
                            continue;

                        MODISHeader modisHeader(pkt);

                        // Store all parse values
                        dayCounts.push_back(modisHeader.day_count);
                        coarseCounts.push_back(modisHeader.coarse_time);
                    }
                }
            }

            if (time(NULL) % 2 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        data_in_tmp.close();

        // Compute the most recurrent value
        common_day = most_common(dayCounts.begin(), dayCounts.end());
        common_coarse = most_common(coarseCounts.begin(), coarseCounts.end());

        logger->info("Detected year         : " + std::to_string(1958 + (common_day / 365.25)));
        logger->info("Detected coarse time  : " + std::to_string(common_coarse));
    }

    logger->info("Demultiplexing and deframing...");

    libccsds::Demuxer ccsdsDemuxer;

    MODISReader reader;
    reader.common_day = common_day;
    reader.common_coarse = common_coarse;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)&cadu, 1024);

        // Parse this transport frame
        libccsds::VCDU vcdu = libccsds::parseVCDU(cadu);

        // Right channel? (VCID 30/42 is MODIS)
        if (vcdu.vcid == (terra ? 42 : 30))
        {
            modis_cadu++;

            // Demux
            std::vector<libccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

            // Count frames
            ccsds += ccsdsFrames.size();

            // Push into processor (filtering APID 64)
            for (libccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 64)
                {
                    modis_ccsds++;
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

    logger->info("VCID 30 (MODIS) Frames : " + std::to_string(modis_cadu));
    logger->info("CCSDS Frames           : " + std::to_string(ccsds));
    logger->info("MODIS CCSDS Frames     : " + std::to_string(modis_ccsds));
    logger->info("MODIS Day frames       : " + std::to_string(reader.day_count));
    logger->info("MODIS Night frames     : " + std::to_string(reader.night_count));
    logger->info("MODIS 1km lines        : " + std::to_string(reader.lines));
    logger->info("MODIS 500m lines       : " + std::to_string(reader.lines * 2));
    logger->info("MODIS 250m lines       : " + std::to_string(reader.lines * 4));

    logger->info("Writing images.... (Can take a while)");

    if (!std::filesystem::exists(directory))
        std::filesystem::create_directory(directory);

    for (int i = 0; i < 2; i++)
    {
        cimg_library::CImg<unsigned short> image = reader.getImage250m(i);
        logger->info("Channel " + std::to_string(i + 1) + "...");
        image.save_png(std::string(directory + "/MODIS-" + std::to_string(i + 1) + ".png").c_str());
        d_output_files.push_back(directory + "/MODIS-" + std::to_string(i + 1) + ".png");
    }

    for (int i = 0; i < 5; i++)
    {
        cimg_library::CImg<unsigned short> image = reader.getImage500m(i);
        logger->info("Channel " + std::to_string(i + 3) + "...");
        image.save_png(std::string(directory + "/MODIS-" + std::to_string(i + 3) + ".png").c_str());
        d_output_files.push_back(directory + "/MODIS-" + std::to_string(i + 3) + ".png");
    }

    for (int i = 0; i < 31; i++)
    {
        cimg_library::CImg<unsigned short> image = reader.getImage1000m(i);
        if (i < 5)
        {
            logger->info("Channel " + std::to_string(i + 8) + "...");
            image.save_png(std::string(directory + "/MODIS-" + std::to_string(i + 8) + ".png").c_str());
            d_output_files.push_back(directory + "/MODIS-" + std::to_string(i + 8) + ".png");
        }
        else if (i == 5)
        {
            logger->info("Channel 13L...");
            image.save_png(std::string(directory + "/MODIS-13L.png").c_str());
            d_output_files.push_back(directory + "/MODIS-13L.png");
        }
        else if (i == 6)
        {
            logger->info("Channel 13H...");
            image.save_png(std::string(directory + "/MODIS-13H.png").c_str());
            d_output_files.push_back(directory + "/MODIS-13H.png");
        }
        else if (i == 7)
        {
            logger->info("Channel 14L...");
            image.save_png(std::string(directory + "/MODIS-14L.png").c_str());
            d_output_files.push_back(directory + "/MODIS-14L.png");
        }
        else if (i == 8)
        {
            logger->info("Channel 14H...");
            image.save_png(std::string(directory + "/MODIS-14H.png").c_str());
            d_output_files.push_back(directory + "/MODIS-14H.png");
        }
        else
        {
            logger->info("Channel " + std::to_string(i + 6) + "...");
            image.save_png(std::string(directory + "/MODIS-" + std::to_string(i + 6) + ".png").c_str());
            d_output_files.push_back(directory + "/MODIS-" + std::to_string(i + 6) + ".png");
        }
    }

    // Output a few nice composites as well
    logger->info("221 Composite...");
    cimg_library::CImg<unsigned short> image221(1354 * 4, reader.lines * 4, 1, 3);
    {
        cimg_library::CImg<unsigned short> tempImage2 = reader.getImage250m(1), tempImage1 = reader.getImage250m(0);
        tempImage2.equalize(1000);
        tempImage1.equalize(1000);
        image221.draw_image(0, 0, 0, 0, tempImage2);
        image221.draw_image(0, 0, 0, 1, tempImage2);
        image221.draw_image(0, 0, 0, 2, tempImage1);
    }
    image221.save_png(std::string(directory + "/MODIS-RGB-221.png").c_str());
    d_output_files.push_back(directory + "/MODIS-RGB-221.png");

    logger->info("121 Composite...");
    cimg_library::CImg<unsigned short> image121(1354 * 4, reader.lines * 4, 1, 3);
    {
        cimg_library::CImg<unsigned short> tempImage2 = reader.getImage250m(1), tempImage1 = reader.getImage250m(0);
        tempImage2.equalize(1000);
        tempImage1.equalize(1000);
        image121.draw_image(0, 0, 0, 0, tempImage1);
        image121.draw_image(0, 0, 0, 1, tempImage2);
        image121.draw_image(0, 0, 0, 2, tempImage1);
    }
    image121.save_png(std::string(directory + "/MODIS-RGB-121.png").c_str());
    d_output_files.push_back(directory + "/MODIS-RGB-121.png");

    logger->info("143 Composite...");
    cimg_library::CImg<unsigned short> image143(1354 * 4, reader.lines * 4, 1, 3);
    {
        cimg_library::CImg<unsigned short> tempImage4 = reader.getImage500m(1), tempImage3 = reader.getImage500m(0), tempImage1 = reader.getImage250m(0);
        tempImage4.equalize(1000);
        tempImage3.equalize(1000);
        tempImage1.equalize(1000);
        image143.draw_image(0, 0, 0, 0, tempImage1);
        tempImage3.resize(tempImage3.width() * 2, tempImage3.height() * 2);
        tempImage4.resize(tempImage4.width() * 2, tempImage4.height() * 2);
        image143.draw_image(0, 0, 0, 1, tempImage4);
        image143.draw_image(0, 0, 0, 2, tempImage3);
        image143.equalize(1000);
    }
    image143.save_png(std::string(directory + "/MODIS-RGB-143.png").c_str());
    d_output_files.push_back(directory + "/MODIS-RGB-143.png");
}

std::string EOSMODISDecoderModule::getID()
{
    return "eos_modis";
}

std::vector<std::string> EOSMODISDecoderModule::getParameters()
{
    return {"terra_mode"};
}

std::shared_ptr<ProcessingModule> EOSMODISDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<EOSMODISDecoderModule>(input_file, output_file_hint, parameters);
}