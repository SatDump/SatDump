#include "module_fengyun_virr.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "virr_deframer.h"
#include "virr_reader.h"
#include "xfr.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace virr
    {
        FengyunVIRRDecoderModule::FengyunVIRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void FengyunVIRRDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIRR";

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
                    std::vector<uint8_t> defraVec;
                    defraVec.insert(defraVec.end(), &buffer[14], &buffer[14 + 882]);
                    std::vector<std::vector<uint8_t>> out = virrDefra.work(defraVec);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        virr_frames++;
                        reader.work(frameVec);
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

            logger->info("VCID 5 Frames         : " + std::to_string(vcidFrames));
            logger->info("VIRR Frames           : " + std::to_string(virr_frames));
            logger->info("VIRR Lines            : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

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

            logger->info("321 Composite...");
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

            logger->info("621 Composite...");
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

            logger->info("197 Composite...");
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

            logger->info("917 Composite...");
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

            logger->info("197 True Color XFR Composite... (by ZbychuButItWasTaken)");
            cimg_library::CImg<unsigned short> image197truecolorxfr(2048, reader.lines, 1, 3);
            {
                cimg_library::CImg<unsigned short> tempImage1 = image1, tempImage9 = image9, tempImage7 = image7;

                XFR trueColor(26, 663, 165, 34, 999, 162, 47, 829, 165);

                applyXFR(trueColor, tempImage1, tempImage9, tempImage7);

                image197truecolorxfr.draw_image(1, 0, 0, 0, tempImage1);
                image197truecolorxfr.draw_image(0, 0, 0, 1, tempImage9);
                image197truecolorxfr.draw_image(-2, 0, 0, 2, tempImage7);
            }
            WRITE_IMAGE(image197truecolorxfr, directory + "/VIRR-RGB-197-TRUECOLOR.png");

            logger->info("197 Night XFR Composite... (by ZbychuButItWasTaken)");
            cimg_library::CImg<unsigned short> image197nightxfr(2048, reader.lines, 1, 3);
            {
                cimg_library::CImg<unsigned short> tempImage1 = image1, tempImage9 = image9, tempImage7 = image7;

                XFR trueColor(23, 610, 153, 34, 999, 162, 39, 829, 165);

                applyXFR(trueColor, tempImage1, tempImage9, tempImage7);

                image197nightxfr.draw_image(1, 0, 0, 0, tempImage1);
                image197nightxfr.draw_image(0, 0, 0, 1, tempImage9);
                image197nightxfr.draw_image(-2, 0, 0, 2, tempImage7);
            }
            WRITE_IMAGE(image197nightxfr, directory + "/VIRR-RGB-197-NIGHT.png");
        }

        void FengyunVIRRDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun VIRR Decoder", NULL, window ? NULL : NOWINDOW_FLAGS );

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

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