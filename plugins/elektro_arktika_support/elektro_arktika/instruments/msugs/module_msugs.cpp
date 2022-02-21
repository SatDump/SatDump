#include "module_msugs.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "simpledeframer.h"
#include "msu_vis_reader.h"
#include "msu_ir_reader.h"
#include "imgui/imgui.h"
#include "common/image/hue_saturation.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace elektro_arktika
{
    namespace msugs
    {
        MSUGSDecoderModule::MSUGSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MSUGSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            data_in = std::ifstream(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-GS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            int vis1_frames = 0;
            int vis2_frames = 0;
            int vis3_frames = 0;
            int infr_frames = 0;

            SimpleDeframer<uint64_t, 64, 121680, 0x0218a7a392dd9abf, 10> deframerVIS1, deframerVIS2, deframerVIS3;
            SimpleDeframer<uint64_t, 64, 14560, 0x0218a7a392dd9abf, 10> deframerIR;
            SimpleDeframer<uint64_t, 24, 1680, 0xa6007c, 0> deframerUnknown;

            //std::ofstream data_unknown(directory + "/data_unknown.bin", std::ios::binary);

            MSUVISReader vis1_reader, vis2_reader, vis3_reader;
            MSUIRReader infr_reader;

            logger->info("Demultiplexing and deframing...");

            //std::ofstream data_out1(directory + "/data_msugs1.bin", std::ios::binary);
            //std::ofstream data_out2(directory + "/data_msugs2.bin", std::ios::binary);
            //std::ofstream data_out3(directory + "/data_msugs3.bin", std::ios::binary);

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)cadu, 1024);

                int vcid = (cadu[5] >> 1) & 7;

                // logger->critical("VCID " + std::to_string(vcid));

                if (vcid == 2)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS1.work(&cadu[24], 1024 - 24);
                    vis1_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                    {
                        vis1_reader.pushFrame(&frame[0]);
                        //data_out1.write((char *)frame.data(), frame.size());
                    }
                }
                else if (vcid == 3)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS2.work(&cadu[24], 1024 - 24);
                    vis2_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                    {
                        vis2_reader.pushFrame(&frame[0]);
                        //data_out2.write((char *)frame.data(), frame.size());
                    }
                }
                else if (vcid == 5)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS3.work(&cadu[24], 1024 - 24);
                    vis3_frames += frames.size();

                    for (std::vector<uint8_t> &frame : frames)
                    {
                        vis3_reader.pushFrame(&frame[0]);
                        //data_out3.write((char *)frame.data(), frame.size());
                    }
                }
                else if (vcid == 4)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerIR.work(&cadu[24], 1024 - 24);
                    infr_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                        infr_reader.pushFrame(&frame[0]);
                }

                // Unknown additional data, 0xa6007c ASM 1680-bits frames
                /*{
                    // datatest.write((char *)&cadu[12], 10); // , no idea what that is yet
                    std::vector<std::vector<uint8_t>> frames = deframerUnknown.work(&cadu[12], 10);
                    infr_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                    {
                        int marker = (frame[3] >> 1) & 0b111;
                        //logger->error(marker);
                        if (marker == 5) // 0, 2, 3, 4, 5
                            data_unknown.write((char *)frame.data(), frame.size());
                    }
                }*/

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            //data_unknown.close();

            data_in.close();

            logger->info("MSU-GS CH1 Lines        : " + std::to_string(vis1_frames));
            logger->info("MSU-GS CH2 Lines        : " + std::to_string(vis2_frames));
            logger->info("MSU-GS CH3 Lines        : " + std::to_string(vis3_frames));
            logger->info("MSU-GS IR Frames        : " + std::to_string(infr_frames));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            image::Image<uint16_t> image1 = vis1_reader.getImage();
            image::Image<uint16_t> image2 = vis2_reader.getImage();
            image::Image<uint16_t> image3 = vis3_reader.getImage();

            logger->info("Channel VIS 1...");
            WRITE_IMAGE(image1, directory + "/MSU-GS-1.png");

            logger->info("Channel VIS 2...");
            WRITE_IMAGE(image2, directory + "/MSU-GS-2.png");

            logger->info("Channel VIS 3...");
            WRITE_IMAGE(image3, directory + "/MSU-GS-3.png");

            for (int i = 0; i < 7; i++)
            {
                logger->info("Channel IR " + std::to_string(i + 4) + "...");
                WRITE_IMAGE(infr_reader.getImage(i), directory + "/MSU-GS-" + std::to_string(i + 4) + ".png");
            }

            logger->info("221 Composite...");
            {
                image::Image<uint16_t> image221(image1.width(), std::max<int>(image1.height(), image2.height()), 3);
                {
                    image221.draw_image(0, image2, 31 - 2, -2220 + 13 - 6);
                    image221.draw_image(1, image2, 31 - 2, -2220 + 13 - 6);
                    image221.draw_image(2, image1, 23 + 46 + 13 - 30 - 2, -440 + 10 - 17 + 40 - 10);
                }
                image221.white_balance();
                WRITE_IMAGE(image221, directory + "/MSU-GS-RGB-221.png");
            }

            logger->info("Natural Color Composite...");
            {
                image::Image<uint16_t> imageNC(image1.width(), std::max<int>(image1.height(), std::max<int>(image2.height(), image3.height())), 3);
                {
                    imageNC.draw_image(0, image3);
                    imageNC.draw_image(1, image2, 31 - 2, -2220 + 13 - 6);
                    imageNC.draw_image(2, image1, 23 + 46 + 13 - 30 - 2, -440 + 10 - 17 + 40 - 10);

                    image::Image<uint8_t> imageNC_8bits(image1.width(), std::max<int>(image1.height(), std::max<int>(image2.height(), image3.height())), 3);

                    for (int i = 0; i < imageNC.height() * imageNC.width() * 3; i++)
                        imageNC_8bits[i] = imageNC[i] / 255;

                    image::HueSaturation hueTuning;
                    hueTuning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
                    hueTuning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
                    hueTuning.overlap = 100.0 / 100.0;
                    image::hue_saturation(imageNC_8bits, hueTuning);

                    for (int i = 0; i < imageNC.height() * imageNC.width() * 3; i++)
                        imageNC[i] = imageNC_8bits[i] * 255;

                    imageNC.white_balance();
                }
                WRITE_IMAGE(imageNC, directory + "/MSU-GS-RGB-NC.png");
            }
        }

        void MSUGSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO / ARKTIKA MSU-GS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MSUGSDecoderModule::getID()
        {
            return "elektro_arktika_msugs";
        }

        std::vector<std::string> MSUGSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MSUGSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MSUGSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msugs
} // namespace elektro_arktika
