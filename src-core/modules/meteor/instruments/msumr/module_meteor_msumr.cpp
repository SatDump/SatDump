#include "module_meteor_msumr.h"
#include <fstream>
#include "modules/meteor/simpledeframer.h"
#include "logger.h"
#include <filesystem>
#include "msumr_reader.h"
#include "imgui/imgui.h"
#include "common/image/earth_curvature.h"
#include "modules/meteor/meteor.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    namespace msumr
    {
        METEORMSUMRDecoderModule::METEORMSUMRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void METEORMSUMRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-MR";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1024];

            std::vector<uint8_t> msumrData;

            // MSU-MR data
            SimpleDeframer<uint64_t, 64, 11850 * 8, 0x0218A7A392DD9ABF, 10> msumrDefra;

            MSUMRReader reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract MSU-MR
                msumrData.insert(msumrData.end(), &buffer[23 - 1], &buffer[23 - 1] + 238);
                msumrData.insert(msumrData.end(), &buffer[279 - 1], &buffer[279 - 1] + 238);
                msumrData.insert(msumrData.end(), &buffer[535 - 1], &buffer[535 - 1] + 238);
                msumrData.insert(msumrData.end(), &buffer[791 - 1], &buffer[791 - 1] + 234);

                // Deframe
                std::vector<std::vector<uint8_t>> msumr_frames = msumrDefra.work(msumrData);

                for (std::vector<uint8_t> frame : msumr_frames)
                    reader.work(frame.data());

                msumrData.clear();

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("MSU-MR Lines          : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
            cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
            cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
            cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
            cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);
            cimg_library::CImg<unsigned short> image6 = reader.getChannel(5);

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/MSU-MR-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/MSU-MR-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/MSU-MR-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/MSU-MR-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/MSU-MR-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(image5, directory + "/MSU-MR-6.png");

            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(1572, reader.lines, 1, 3);
                {
                    image221.draw_image(0, 0, 0, 0, image2);
                    image221.draw_image(0, 0, 0, 1, image2);
                    image221.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221.png");
                image221.equalize(1000);
                image221.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image221, directory + "/MSU-MR-RGB-221-EQU.png");
                cimg_library::CImg<unsigned short> corrected221 = image::earth_curvature::correct_earth_curvature(image221,
                                                                                                                  METEOR_ORBIT_HEIGHT,
                                                                                                                  METEOR_MSUMR_SWATH,
                                                                                                                  METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected221, directory + "/MSU-MR-RGB-221-EQU-CORRECTED.png");
            }

            logger->info("321 Composite...");
            {
                cimg_library::CImg<unsigned short> image321(1572, reader.lines, 1, 3);
                {
                    image321.draw_image(0, 0, 0, 0, image3);
                    image321.draw_image(0, 0, 0, 1, image2);
                    image321.draw_image(0, 0, 0, 2, image1);
                }
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321.png");
                image321.equalize(1000);
                image321.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image321, directory + "/MSU-MR-RGB-321-EQU.png");
                cimg_library::CImg<unsigned short> corrected321 = image::earth_curvature::correct_earth_curvature(image321,
                                                                                                                  METEOR_ORBIT_HEIGHT,
                                                                                                                  METEOR_MSUMR_SWATH,
                                                                                                                  METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected321, directory + "/MSU-MR-RGB-321-EQU-CORRECTED.png");
            }

            logger->info("Equalized Ch 5...");
            {
                image5.equalize(1000);
                image5.normalize(0, std::numeric_limits<unsigned char>::max());
                WRITE_IMAGE(image5, directory + "/MSU-MR-5-EQU.png");
                cimg_library::CImg<unsigned short> corrected5 = image::earth_curvature::correct_earth_curvature(image5,
                                                                                                                METEOR_ORBIT_HEIGHT,
                                                                                                                METEOR_MSUMR_SWATH,
                                                                                                                METEOR_MSUMR_RES);
                WRITE_IMAGE(corrected5, directory + "/MSU-MR-5-EQU-CORRECTED.png");
            }
        }

        void METEORMSUMRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("METEOR MSU-MR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string METEORMSUMRDecoderModule::getID()
        {
            return "meteor_msumr";
        }

        std::vector<std::string> METEORMSUMRDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> METEORMSUMRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<METEORMSUMRDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msumr
} // namespace meteor