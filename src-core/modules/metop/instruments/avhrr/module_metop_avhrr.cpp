#include "module_metop_avhrr.h"
#include <fstream>
#include "avhrr_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/vegetation_index.h"
#include "common/image/earth_curvature.h"
#include "modules/metop/metop.h"
#include "nlohmann/json_utils.h"
#include "nlohmann/json_utils.h"
#include "common/projection/leo_to_equirect.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace avhrr
    {
        MetOpAVHRRDecoderModule::MetOpAVHRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        std::string getHRPTReaderTimeStamp()
        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);

            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +                                                                               // Year yyyy
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" + // Month MM
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "-" +          // Day dd
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +                // Hour HH
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));                    // Minutes mm

            return timestamp;
        }

        void MetOpAVHRRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            AVHRRReader reader;
            uint64_t avhrr_cadu = 0, ccsds = 0, avhrr_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            // Get satellite info
            nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
            int scid = satData.contains("scid") > 0 ? satData["scid"].get<int>() : 0;

            // Name the file properly
            std::string hpt_prefix = "M0x_";

            if (scid == METOP_A_SCID)
                hpt_prefix = "M02_";
            else if (scid == METOP_B_SCID)
                hpt_prefix = "M03_";
            else if (scid == METOP_C_SCID)
                hpt_prefix = "M04_";

            std::string hpt_filename = hpt_prefix + getHRPTReaderTimeStamp() + ".hpt";
            std::ofstream output_hrpt_reader(directory + "/" + hpt_filename, std::ios::binary);
            d_output_files.push_back(directory + "/" + hpt_filename);

            uint8_t hpt_buffer[13864];
            int counter = 0, counter2 = 0;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 30/42 is MODIS)
                if (vcdu.vcid == 9)
                {
                    avhrr_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 103 and 104)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 103 || pkt.header.apid == 104)
                        {
                            avhrr_ccsds++;
                            reader.work(pkt);

                            // Write out HPT
                            if (pkt.payload.size() >= 12960)
                            {
                                // Clean it up
                                std::fill(hpt_buffer, &hpt_buffer[13864], 0);

                                // Write header
                                hpt_buffer[0] = 0xa1;
                                hpt_buffer[1] = 0x16;
                                hpt_buffer[2] = 0xfd;
                                hpt_buffer[3] = 0x71;
                                hpt_buffer[4] = 0x9d;
                                hpt_buffer[5] = 0x83;
                                hpt_buffer[6] = 0xc9;

                                // Counter
                                hpt_buffer[7] = 0b01010 << 3 | (counter & 0b111) << 1 | 0b1;
                                counter++;
                                if (counter == 4)
                                    counter = 0;

                                // Timestamp
                                uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
                                days -= 502;         // Scale from 1/1/2000 to days since first frame
                                days &= 0b111111111; // Cap to 9-bits

                                hpt_buffer[10] = days >> 1;
                                hpt_buffer[11] = (days & 0b1) << 7 | 0b0101 << 3 | (pkt.payload[2] & 0b111);
                                std::memcpy(&hpt_buffer[12], &pkt.payload[3], 3);

                                // Other marker
                                hpt_buffer[21] = counter2 == 0 ? 0 : 0b1100;
                                hpt_buffer[22] = counter2 == 0 ? 0 : 0b0011;
                                hpt_buffer[24] = counter2 == 0 ? 0 : 0b11000000;
                                counter2++;
                                if (counter2 == 5)
                                    counter2 = 0;

                                // Imagery
                                std::memcpy(&hpt_buffer[937], &pkt.payload[76], 12800);

                                // Write it out
                                output_hrpt_reader.write((char *)hpt_buffer, 13864);
                            }
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
            output_hrpt_reader.close();

            logger->info("VCID 9 (AVHRR) Frames  : " + std::to_string(avhrr_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("AVHRR CCSDS Frames     : " + std::to_string(avhrr_ccsds));
            logger->info("AVHRR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
            cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
            cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
            cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
            cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/AVHRR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/AVHRR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/AVHRR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/AVHRR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/AVHRR-5.png");

            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(2048, reader.lines, 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image221, directory + "/AVHRR-RGB-221.png");
                image221.equalize(1000);
                image221.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image221, directory + "/AVHRR-RGB-221-EQU.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  METOP_ORBIT_HEIGHT,
                                                                                                                  METOP_AVHRR_SWATH,
                                                                                                                  METOP_AVHRR_RES);
                WRITE_IMAGE(corrected221, directory + "/AVHRR-RGB-221-EQU-CORRECTED.png");
            }

            logger->info("321 Composite...");
            {
                cimg_library::CImg<unsigned short> image321(2048, reader.lines, 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image321, directory + "/AVHRR-RGB-321.png");
                image321.equalize(1000);
                image321.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image321, directory + "/AVHRR-RGB-321-EQU.png");
                cimg_library::CImg<unsigned short> corrected321 = image::earth_curvature::correct_earth_curvature(image321,
                                                                                                                  METOP_ORBIT_HEIGHT,
                                                                                                                  METOP_AVHRR_SWATH,
                                                                                                                  METOP_AVHRR_RES);
                WRITE_IMAGE(corrected321, directory + "/AVHRR-RGB-321-EQU-CORRECTED.png");
            }

            logger->info("Equalized Ch 4...");
            {
                image4.equalize(1000);
                image4.normalize(0, std::numeric_limits<unsigned short>::max());
                WRITE_IMAGE(image4, directory + "/AVHRR-4-EQU.png");
                cimg_library::CImg<unsigned short> corrected4 = image::earth_curvature::correct_earth_curvature(image4,
                                                                                                                METOP_ORBIT_HEIGHT,
                                                                                                                METOP_AVHRR_SWATH,
                                                                                                                METOP_AVHRR_RES);
                WRITE_IMAGE(corrected4, directory + "/AVHRR-4-EQU-CORRECTED.png");
            }

            // Reproject to an equirectangular proj
            {
                int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;
                //image4.equalize(1000);

                // Setup Projecition
                projection::LEOScanProjector projector(5,                           // Pixel offset
                                                       2050,                        // Correction swath
                                                       1,                           // Instrument res
                                                       800,                         // Orbit height
                                                       METOP_AVHRR_SWATH,           // Instrument swath
                                                       2.515,                        // Scale
                                                       0.6,                         // Az offset
                                                       0,                           // Tilt
                                                       0.7,                         // Time offset
                                                       image4.width(),              // Image width
                                                       true,                        // Invert scan
                                                       tle::getTLEfromNORAD(norad), // TLEs
                                                       reader.timestamps            // Timestamps
                );

                logger->info("Projected channel 4...");
                cimg_library::CImg<unsigned char> projected_image = projection::projectLEOToEquirectangularMapped(image4, projector, 2048 * 4, 1024 * 4, 1);
                WRITE_IMAGE(projected_image, directory + "/AVHRR-4-PROJ.png");

                cimg_library::CImg<unsigned short> image321(2048, reader.lines, 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }

                logger->info("Projected channel 4...");
                projected_image = projection::projectLEOToEquirectangularMapped(image321, projector, 2048 * 4, 1024 * 4, 3);
                WRITE_IMAGE(projected_image, directory + "/AVHRR-RGB-321-PROJ.png");
            }
        }

        void MetOpAVHRRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp AVHRR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
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
    } // namespace avhrr
} // namespace metop