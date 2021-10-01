#include "module_eos_modis.h"
#include <fstream>
#include "modis_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/image/image.h"
#include "common/utils.h"
#include "common/image/earth_curvature.h"
#include "modules/eos/eos.h"
#include "nlohmann/json_utils.h"
#include "common/projection/leo_to_equirect.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace eos
{
    namespace modis
    {
        EOSMODISDecoderModule::EOSMODISDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            terra(std::stoi(parameters["terra_mode"])),
                                                                                                                                                            bowtie(std::stoi(parameters["correct_bowtie"]))
        {
        }

        void EOSMODISDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MODIS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t cadu[1024];

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
                ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer;

                while (!data_in_tmp.eof())
                {
                    // Read buffer
                    data_in_tmp.read((char *)&cadu, 1024);

                    // Parse this transport frame
                    ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                    // Right channel? (VCID 30/42 is MODIS)
                    if (vcdu.vcid == (terra ? 42 : 30))
                    {
                        modis_cadu++;

                        // Demux
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                        // Push into processor (filtering APID 64)
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
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

                    progress = data_in_tmp.tellg();

                    if (time(NULL) % 2 == 0 && lastTime != time(NULL))
                    {
                        lastTime = time(NULL);
                        logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
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

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer;

            MODISReader reader;
            reader.common_day = common_day;
            reader.common_coarse = common_coarse;

            std::ofstream data_out(directory + "/modis_ccsds.bin", std::ios::binary);
            logger->info("Writing CCSDS frames to " + directory + "/modis_ccsds.bin");
            uint8_t ccsds_modis[642];

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 30/42 is MODIS)
                if (vcdu.vcid == (terra ? 42 : 30))
                {
                    modis_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 64)
                        {
                            modis_ccsds++;
                            reader.work(pkt);

                            // Write it out
                            std::fill(&ccsds_modis[0], &ccsds_modis[642], 0);
                            std::memcpy(&ccsds_modis[0], pkt.header.raw, 6);
                            std::memcpy(&ccsds_modis[6], pkt.payload.data(), std::min<size_t>(pkt.payload.size(), 636));
                            data_out.write((char *)ccsds_modis, 642);
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
            data_out.close();

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

            // Get NORAD
            int norad = terra ? EOS_TERRA_NORAD : EOS_AQUA_NORAD;

            // BowTie values
            const float alpha = 1.0 / 1.8;
            const float beta = 0.58333;
            const long scanHeight_250 = 40;
            const long scanHeight_500 = 20;
            const long scanHeight_1000 = 10;

            for (int i = 0; i < 2; i++)
            {
                cimg_library::CImg<unsigned short> image = reader.getImage250m(i);

                if (bowtie)
                    image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_250, alpha, beta);

                logger->info("Channel " + std::to_string(i + 1) + "...");
                WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 1) + ".png");
            }

            for (int i = 0; i < 5; i++)
            {
                cimg_library::CImg<unsigned short> image = reader.getImage500m(i);

                if (bowtie)
                    image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_500, alpha, beta);

                logger->info("Channel " + std::to_string(i + 3) + "...");
                WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 3) + ".png");
            }

            for (int i = 0; i < 31; i++)
            {
                cimg_library::CImg<unsigned short> image = reader.getImage1000m(i);

                if (bowtie)
                    image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_1000, alpha, beta);

                if (i < 5)
                {
                    logger->info("Channel " + std::to_string(i + 8) + "...");
                    WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 8) + ".png");
                }
                else if (i == 5)
                {
                    logger->info("Channel 13L...");
                    WRITE_IMAGE(image, directory + "/MODIS-13L.png");
                }
                else if (i == 6)
                {
                    logger->info("Channel 13H...");
                    WRITE_IMAGE(image, directory + "/MODIS-13H.png");
                }
                else if (i == 7)
                {
                    logger->info("Channel 14L...");
                    WRITE_IMAGE(image, directory + "/MODIS-14L.png");
                }
                else if (i == 8)
                {
                    logger->info("Channel 14H...");
                    WRITE_IMAGE(image, directory + "/MODIS-14H.png");
                }
                else
                {
                    logger->info("Channel " + std::to_string(i + 6) + "...");
                    WRITE_IMAGE(image, directory + "/MODIS-" + std::to_string(i + 6) + ".png");
                }
            }

            // Output a few nice composites as well
            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(1354 * 4, reader.lines * 4, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage2 = reader.getImage250m(1), tempImage1 = reader.getImage250m(0);
                    image221.draw_image(0, 0, 0, 0, tempImage2);
                    image221.draw_image(0, 0, 0, 1, tempImage2);
                    image221.draw_image(0, 0, 0, 2, tempImage1);

                    image::white_balance(image221);

                    if (bowtie)
                        image221 = image::bowtie::correctGenericBowTie(image221, 3, scanHeight_250, alpha, beta);
                }
                WRITE_IMAGE(image221, directory + "/MODIS-RGB-221.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  EOS_ORBIT_HEIGHT,
                                                                                                                  EOS_MODIS_SWATH,
                                                                                                                  EOS_MODIS_RES250);
                WRITE_IMAGE(corrected221, directory + "/MODIS-RGB-221-CORRECTED.png");
                image221.equalize(1000);
                WRITE_IMAGE(image221, directory + "/MODIS-RGB-221-EQU.png");
                cimg_library::CImg<unsigned short> corrected221equ = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                     EOS_ORBIT_HEIGHT,
                                                                                                                     EOS_MODIS_SWATH,
                                                                                                                     EOS_MODIS_RES250);
                WRITE_IMAGE(corrected221equ, directory + "/MODIS-RGB-221-EQU-CORRECTED.png");
            }

            logger->info("121 Composite...");
            {
                cimg_library::CImg<unsigned short> image121(1354 * 4, reader.lines * 4, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage2 = reader.getImage250m(1), tempImage1 = reader.getImage250m(0);
                    tempImage2.equalize(1000);
                    tempImage1.equalize(1000);
                    image121.draw_image(0, 0, 0, 0, tempImage1);
                    image121.draw_image(0, 0, 0, 1, tempImage2);
                    image121.draw_image(0, 0, 0, 2, tempImage1);

                    if (bowtie)
                        image121 = image::bowtie::correctGenericBowTie(image121, 3, scanHeight_250, alpha, beta);
                }
                WRITE_IMAGE(image121, directory + "/MODIS-RGB-121.png");
                cimg_library::CImg<unsigned short> corrected121 = image::earth_curvature::correct_earth_curvature(image121,
                                                                                                                  EOS_ORBIT_HEIGHT,
                                                                                                                  EOS_MODIS_SWATH,
                                                                                                                  EOS_MODIS_RES250);
                WRITE_IMAGE(corrected121, directory + "/MODIS-RGB-121-EQU-CORRECTED.png");
            }

            logger->info("143 Composite...");
            {
                cimg_library::CImg<unsigned short> image143(1354 * 4, reader.lines * 4, 1, 3), pre_wb(1354 * 4, reader.lines * 4, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage4 = reader.getImage500m(1), tempImage3 = reader.getImage500m(0), tempImage1 = reader.getImage250m(0);
                    tempImage3.resize(tempImage3.width() * 2, tempImage3.height() * 2);
                    tempImage4.resize(tempImage4.width() * 2, tempImage4.height() * 2);

                    image143.draw_image(0, 0, 0, 0, tempImage1);
                    image143.draw_image(0, 0, 0, 1, tempImage4);
                    image143.draw_image(0, 0, 0, 2, tempImage3);

                    tempImage1.equalize(1000);
                    tempImage4.equalize(1000);
                    tempImage3.equalize(1000);
                    pre_wb.draw_image(0, 0, 0, 0, tempImage1);
                    pre_wb.draw_image(0, 0, 0, 1, tempImage4);
                    pre_wb.draw_image(0, 0, 0, 2, tempImage3);

                    image::white_balance(image143);

                    if (bowtie)
                    {
                        image143 = image::bowtie::correctGenericBowTie(image143, 3, scanHeight_250, alpha, beta);
                        pre_wb = image::bowtie::correctGenericBowTie(pre_wb, 3, scanHeight_250, alpha, beta);
                    }
                }

                WRITE_IMAGE(image143, directory + "/MODIS-RGB-143.png");
                cimg_library::CImg<unsigned short> corrected143 = image::earth_curvature::correct_earth_curvature(image143,
                                                                                                                  EOS_ORBIT_HEIGHT,
                                                                                                                  EOS_MODIS_SWATH,
                                                                                                                  EOS_MODIS_RES250);
                WRITE_IMAGE(corrected143, directory + "/MODIS-RGB-143-CORRECTED.png");
                image143.equalize(1000);
                WRITE_IMAGE(image143, directory + "/MODIS-RGB-143-EQU.png");
                cimg_library::CImg<unsigned short> corrected143equ = image::earth_curvature::correct_earth_curvature(image143,
                                                                                                                     EOS_ORBIT_HEIGHT,
                                                                                                                     EOS_MODIS_SWATH,
                                                                                                                     EOS_MODIS_RES250);
                WRITE_IMAGE(corrected143equ, directory + "/MODIS-RGB-143-EQU-CORRECTED.png");
                WRITE_IMAGE(pre_wb, directory + "/MODIS-RGB-143-EQURAW.png");
                cimg_library::CImg<unsigned short> corrected143equraw = image::earth_curvature::correct_earth_curvature(pre_wb,
                                                                                                                        EOS_ORBIT_HEIGHT,
                                                                                                                        EOS_MODIS_SWATH,
                                                                                                                        EOS_MODIS_RES250);
                WRITE_IMAGE(corrected143equraw, directory + "/MODIS-RGB-143-EQURAW-CORRECTED.png");

                // Reproject to an equirectangular proj
                {
                    //pre_wb.resize(pre_wb.width() / 4, pre_wb.height() / 4);

                    // Setup Projecition
                    projection::LEOScanProjector projector({
                        0,                           // Pixel offset
                        1950,                        // Correction swath
                        EOS_MODIS_RES250,            // Instrument res
                        800,                         // Orbit height
                        2220,                        // Instrument swath
                        2.45,                        // Scale
                        -2.2,                        // Az offset
                        0,                           // Tilt
                        -2.2,                        // Time offset
                        pre_wb.width(),              // Image width
                        true,                        // Invert scan
                        tle::getTLEfromNORAD(norad), // TLEs
                        reader.timestamps_250        // Timestamps
                    });

                    logger->info("Projected Channel 143 EQURAW...");
                    cimg_library::CImg<unsigned char> projected_image = projection::projectLEOToEquirectangularMapped(pre_wb, projector, 2048 * 4, 1024 * 4, 3);
                    WRITE_IMAGE(projected_image, directory + "/MODIS-143-EQURAW-PROJ.png");
                }
            }

            logger->info("Equalized Ch 29...");
            {
                cimg_library::CImg<unsigned short> image23 = reader.getImage1000m(23);
                image::linear_invert(image23);
                image23.equalize(1000);
                if (bowtie)
                    image23 = image::bowtie::correctGenericBowTie(image23, 1, scanHeight_1000, alpha, beta);
                WRITE_IMAGE(image23, directory + "/MODIS-29-EQU.png");
                cimg_library::CImg<unsigned short> corrected23 = image::earth_curvature::correct_earth_curvature(image23,
                                                                                                                 EOS_ORBIT_HEIGHT,
                                                                                                                 EOS_MODIS_SWATH,
                                                                                                                 EOS_MODIS_RES1000);
                WRITE_IMAGE(corrected23, directory + "/MODIS-29-EQU-CORRECTED.png");

                // Reproject to an equirectangular proj
                {
                    // Setup Projecition
                    projection::LEOScanProjector projector({
                        0,                           // Pixel offset
                        1950,                        // Correction swath
                        EOS_MODIS_RES250,            // Instrument res
                        800,                         // Orbit height
                        2220,                        // Instrument swath
                        2.45,                        // Scale
                        -2.2,                        // Az offset
                        0,                           // Tilt
                        -2.2,                        // Time offset
                        image23.width(),             // Image width
                        true,                        // Invert scan
                        tle::getTLEfromNORAD(norad), // TLEs
                        reader.timestamps_1000       // Timestamps
                    });

                    logger->info("Projected Channel 29...");
                    cimg_library::CImg<unsigned char> projected_image = projection::projectLEOToEquirectangularMapped(image23, projector, 2048 * 4, 1024 * 4, 1);
                    WRITE_IMAGE(projected_image, directory + "/MODIS-29-EQU-PROJ.png");
                }
            }
        }

        void EOSMODISDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("EOS MODIS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string EOSMODISDecoderModule::getID()
        {
            return "eos_modis";
        }

        std::vector<std::string> EOSMODISDecoderModule::getParameters()
        {
            return {"terra_mode", "correct_bowtie"};
        }

        std::shared_ptr<ProcessingModule> EOSMODISDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<EOSMODISDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace modis
} // namespace eos