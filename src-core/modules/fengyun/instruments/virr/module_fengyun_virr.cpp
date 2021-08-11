#include "module_fengyun_virr.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "virr_deframer.h"
#include "virr_reader.h"
#include "common/image/xfr.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/fengyun/fengyun3.h"
#include "nlohmann/json_utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace virr
    {
        FengyunVIRRDecoderModule::FengyunVIRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            windowTitle = "FengYun VIRR Decoder";
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

        void FengyunVIRRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIRR";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int vcidFrames = 0, virr_frames = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            VIRRDeframer virrDefra;

            VIRRReader reader;

            logger->info("Demultiplexing and deframing...");

            // Get satellite info
            nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
            int scid = satData.contains("scid") > 0 ? satData["scid"].get<int>() : 0;

            // Name the file properly
            std::string hpt_prefix = "FY3x_";

            if (scid == FY3_A_SCID)
                hpt_prefix = "FY3A_";
            else if (scid == FY3_B_SCID)
                hpt_prefix = "FY3B_";
            else if (scid == FY3_C_SCID)
                hpt_prefix = "FY3C_";

            std::string c10_filename = hpt_prefix + getHRPTReaderTimeStamp() + ".C10";
            std::ofstream output_hrpt_reader(directory + "/" + c10_filename, std::ios::binary);
            d_output_files.push_back(directory + "/" + c10_filename);

            uint8_t c10_buffer[27728];

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 5)
                {
                    vcidFrames++;

                    // Deframe MERSI
                    std::vector<std::vector<uint8_t>> out = virrDefra.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        virr_frames++;
                        reader.work(frameVec);

                        // Write out C10
                        {
                            // Clean it up
                            std::fill(c10_buffer, &c10_buffer[27728], 0);

                            // Write header
                            c10_buffer[0] = 0xa1;
                            c10_buffer[1] = 0x16;
                            c10_buffer[2] = 0xfd;
                            c10_buffer[3] = 0x71;
                            c10_buffer[4] = 0x9d;
                            c10_buffer[5] = 0x83;
                            c10_buffer[6] = 0xc9;
                            c10_buffer[7] = 0x50;
                            c10_buffer[8] = 0x34;
                            c10_buffer[9] = 0x00;
                            c10_buffer[10] = 0x3d;

                            // Timestamp
                            c10_buffer[11] = 0b00101000 | (((frameVec[26044] & 0b111111) << 2 | frameVec[26045] >> 6) & 0b111);
                            c10_buffer[12] = (frameVec[26045] & 0b111111) << 2 | frameVec[26046] >> 6;
                            c10_buffer[13] = (frameVec[26046] & 0b111111) << 2 | frameVec[26047] >> 6;
                            c10_buffer[14] = (frameVec[26047] & 0b111111) << 2 | frameVec[26048] >> 6;

                            // Imagery, shifted by 2 bits
                            for (int i = 0; i < 25600 + 16; i++)
                                c10_buffer[2000 + i] = (frameVec[436 + i] & 0b111111) << 2 | frameVec[437 + i] >> 6;

                            // Last marker
                            c10_buffer[27613] = 0b0000010;

                            // Write it out
                            output_hrpt_reader.write((char *)c10_buffer, 27728);
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

            logger->info("VCID 5 Frames         : " + std::to_string(vcidFrames));
            logger->info("VIRR Frames           : " + std::to_string(virr_frames));
            logger->info("VIRR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
            cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
            cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
            cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
            cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);
            cimg_library::CImg<unsigned short> image6 = reader.getChannel(5);
            cimg_library::CImg<unsigned short> image7 = reader.getChannel(6);
            cimg_library::CImg<unsigned short> image8 = reader.getChannel(7);
            cimg_library::CImg<unsigned short> image9 = reader.getChannel(8);
            cimg_library::CImg<unsigned short> image10 = reader.getChannel(9);

            // Takes a while so we say how we're doing
            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/VIRR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/VIRR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/VIRR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/VIRR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/VIRR-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(image6, directory + "/VIRR-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE(image7, directory + "/VIRR-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE(image8, directory + "/VIRR-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE(image9, directory + "/VIRR-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE(image10, directory + "/VIRR-10.png");

            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(2048, reader.lines, 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image221, directory + "/VIRR-RGB-221.png");
                image221.equalize(1000);
                image221.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image221, directory + "/VIRR-RGB-221-EQU.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  FY3_ORBIT_HEIGHT,
                                                                                                                  FY3_VIRR_SWATH,
                                                                                                                  FY3_VIRR_RES);
                WRITE_IMAGE(corrected221, directory + "/VIRR-RGB-221-EQU-CORRECTED.png");
            }

            logger->info("621 Composite...");
            {
                cimg_library::CImg<unsigned short> image621(2048, reader.lines, 1, 3);
                {
                    image621.draw_image(2, 1, 0, 0, image6);
                    image621.draw_image(0, 0, 0, 1, image2);
                    image621.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image621, directory + "/VIRR-RGB-621.png");
                image621.equalize(1000);
                image621.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image621, directory + "/VIRR-RGB-621-EQU.png");
                cimg_library::CImg<unsigned short> corrected621 = image::earth_curvature::correct_earth_curvature(image621,
                                                                                                                  FY3_ORBIT_HEIGHT,
                                                                                                                  FY3_VIRR_SWATH,
                                                                                                                  FY3_VIRR_RES);
                WRITE_IMAGE(corrected621, directory + "/VIRR-RGB-621-EQU-CORRECTED.png");
            }

            logger->info("197 Composite...");
            {
                cimg_library::CImg<unsigned short> image197(2048, reader.lines, 1, 3);
                {
                    image197.draw_image(1, 0, 0, 0, image1);
                    image197.draw_image(0, 0, 0, 1, image9);
                    image197.draw_image(-2, 0, 0, 2, image7);
                }
                WRITE_IMAGE(image197, directory + "/VIRR-RGB-197.png");
                cimg_library::CImg<unsigned short> image197equ(2048, reader.lines, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage9 = image9, tempImage1 = image1, tempImage7 = image7;
                    tempImage9.equalize(1000);
                    tempImage1.equalize(1000);
                    tempImage7.equalize(1000);
                    image197equ.draw_image(1, 0, 0, 0, tempImage1);
                    image197equ.draw_image(0, 0, 0, 1, tempImage9);
                    image197equ.draw_image(-2, 0, 0, 2, tempImage7);
                    image197equ.equalize(1000);
                    image197equ.normalize(0, std::numeric_limits<unsigned char>::max());
                }
                WRITE_IMAGE(image197equ, directory + "/VIRR-RGB-197-EQU.png");
                cimg_library::CImg<unsigned short> corrected197 = image::earth_curvature::correct_earth_curvature(image197equ,
                                                                                                                  FY3_ORBIT_HEIGHT,
                                                                                                                  FY3_VIRR_SWATH,
                                                                                                                  FY3_VIRR_RES);
                WRITE_IMAGE(corrected197, directory + "/VIRR-RGB-197-EQU-CORRECTED.png");
            }

            logger->info("917 Composite...");
            {
                cimg_library::CImg<unsigned short> image917(2048, reader.lines, 1, 3);
                {
                    image917.draw_image(0, 0, 0, 0, image9);
                    image917.draw_image(1, 0, 0, 1, image1);
                    image917.draw_image(-1, 0, 0, 2, image7);
                }
                WRITE_IMAGE(image917, directory + "/VIRR-RGB-917.png");
                cimg_library::CImg<unsigned short> image917equ(2048, reader.lines, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage9 = image9, tempImage1 = image1, tempImage7 = image7;
                    tempImage9.equalize(1000);
                    tempImage1.equalize(1000);
                    tempImage7.equalize(1000);
                    image917equ.draw_image(0, 0, 0, 0, tempImage9);
                    image917equ.draw_image(1, 0, 0, 1, tempImage1);
                    image917equ.draw_image(-1, 0, 0, 2, tempImage7);
                    image917equ.equalize(1000);
                    image917equ.normalize(0, std::numeric_limits<unsigned char>::max());
                }
                WRITE_IMAGE(image917equ, directory + "/VIRR-RGB-917-EQU.png");
                cimg_library::CImg<unsigned short> corrected917equ = image::earth_curvature::correct_earth_curvature(image917equ,
                                                                                                                     FY3_ORBIT_HEIGHT,
                                                                                                                     FY3_VIRR_SWATH,
                                                                                                                     FY3_VIRR_RES);
                WRITE_IMAGE(corrected917equ, directory + "/VIRR-RGB-917-EQU-CORRECTED.png");
            }

            logger->info("197 True Color XFR Composite... (by ZbychuButItWasTaken)");
            {
                cimg_library::CImg<unsigned short> image197truecolorxfr(2048, reader.lines, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage1 = image1, tempImage9 = image9, tempImage7 = image7;

                    image::xfr::XFR trueColor(26, 663, 165, 34, 999, 162, 47, 829, 165);

                    image::xfr::applyXFR(trueColor, tempImage1, tempImage9, tempImage7);

                    image197truecolorxfr.draw_image(1, 0, 0, 0, tempImage1);
                    image197truecolorxfr.draw_image(0, 0, 0, 1, tempImage9);
                    image197truecolorxfr.draw_image(-2, 0, 0, 2, tempImage7);
                }
                WRITE_IMAGE(image197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR.png");
                cimg_library::CImg<unsigned short> corrected197truecolorxfr = image::earth_curvature::correct_earth_curvature(image197truecolorxfr,
                                                                                                                              FY3_ORBIT_HEIGHT,
                                                                                                                              FY3_VIRR_SWATH,
                                                                                                                              FY3_VIRR_RES);
                WRITE_IMAGE(corrected197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR-CORRECTED.png");
                image197truecolorxfr.equalize(1000);
                WRITE_IMAGE(image197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR-EQU.png");
                cimg_library::CImg<unsigned short> corrected197truecolorxfrequ = image::earth_curvature::correct_earth_curvature(image197truecolorxfr,
                                                                                                                                 FY3_ORBIT_HEIGHT,
                                                                                                                                 FY3_VIRR_SWATH,
                                                                                                                                 FY3_VIRR_RES);
                WRITE_IMAGE(corrected197truecolorxfrequ, directory + "/VIRR-RGB-197-TRUECOLOR-EQU-CORRECTED.png");
            }

            logger->info("197 Night XFR Composite... (by ZbychuButItWasTaken)");
            {
                cimg_library::CImg<unsigned short> image197nightxfr(2048, reader.lines, 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage1 = image1, tempImage9 = image9, tempImage7 = image7;

                    image::xfr::XFR trueColor(23, 610, 153, 34, 999, 162, 39, 829, 165);

                    image::xfr::applyXFR(trueColor, tempImage1, tempImage9, tempImage7);

                    image197nightxfr.draw_image(1, 0, 0, 0, tempImage1);
                    image197nightxfr.draw_image(0, 0, 0, 1, tempImage9);
                    image197nightxfr.draw_image(-2, 0, 0, 2, tempImage7);
                }
                WRITE_IMAGE(image197nightxfr, directory + "/VIRR-RGB-197-NIGHT.png");
                cimg_library::CImg<unsigned short> corrected97nightxfr = image::earth_curvature::correct_earth_curvature(image197nightxfr,
                                                                                                                         FY3_ORBIT_HEIGHT,
                                                                                                                         FY3_VIRR_SWATH,
                                                                                                                         FY3_VIRR_RES);
                WRITE_IMAGE(corrected97nightxfr, directory + "/VIRR-RGB-197-NIGHT-CORRECTED.png");
                image197nightxfr.equalize(1000);
                WRITE_IMAGE(image197nightxfr, directory + "/VIRR-RGB-197-NIGHT-EQU.png");
                cimg_library::CImg<unsigned short> corrected97nightxfrequ = image::earth_curvature::correct_earth_curvature(image197nightxfr,
                                                                                                                            FY3_ORBIT_HEIGHT,
                                                                                                                            FY3_VIRR_SWATH,
                                                                                                                            FY3_VIRR_RES);
                WRITE_IMAGE(corrected97nightxfrequ, directory + "/VIRR-RGB-197-NIGHT-EQU-CORRECTED.png");
            }

            logger->info("Equalized Ch 4...");
            {
                image4.equalize(1000);
                image4.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image4, directory + "/VIRR-4-EQU.png");
                cimg_library::CImg<unsigned short> corrected4 = image::earth_curvature::correct_earth_curvature(image4,
                                                                                                                FY3_ORBIT_HEIGHT,
                                                                                                                FY3_VIRR_SWATH,
                                                                                                                FY3_VIRR_RES);
                WRITE_IMAGE(corrected4, directory + "/VIRR-4-EQU-CORRECTED.png");
            }
        }

        void FengyunVIRRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin(windowTitle.c_str(), NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunVIRRDecoderModule::getID()
        {
            return "fengyun_virr";
        }

        std::vector<std::string> FengyunVIRRDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> FengyunVIRRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunVIRRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace virr
} // namespace fengyun