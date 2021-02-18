#include "module_elektro_msugs.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "simpledeframer.h"
#include "msu_vis_reader.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace elektro
{
    namespace msugs
    {
        ELEKTROMSUGSDecoderModule::ELEKTROMSUGSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void ELEKTROMSUGSDecoderModule::process()
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

            SimpleDeframer<uint64_t, 64, 121680, 0x0218a7a392dd9abf> deframerVIS1, deframerVIS2, deframerVIS3;

            MSUVISReader vis1_reader, vis2_reader, vis3_reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)cadu, 1024);

                int vcid = (cadu[5] >> 1) & 7;

                //logger->critical("VCID " + std::to_string(vcid));

                if (vcid == 2)
                {
                    std::vector<uint8_t> defraVec;
                    defraVec.insert(defraVec.end(), &cadu[24], &cadu[1024]);
                    std::vector<std::vector<uint8_t>> frames = deframerVIS1.work(defraVec);
                    vis1_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                        vis1_reader.pushFrame(&frame[0]);
                }
                else if (vcid == 3)
                {
                    std::vector<uint8_t> defraVec;
                    defraVec.insert(defraVec.end(), &cadu[24], &cadu[1024]);
                    std::vector<std::vector<uint8_t>> frames = deframerVIS2.work(defraVec);
                    vis2_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                        vis2_reader.pushFrame(&frame[0]);
                }
                else if (vcid == 5)
                {
                    std::vector<uint8_t> defraVec;
                    defraVec.insert(defraVec.end(), &cadu[24], &cadu[1024]);
                    std::vector<std::vector<uint8_t>> frames = deframerVIS3.work(defraVec);
                    vis3_frames += frames.size();
                    for (std::vector<uint8_t> &frame : frames)
                        vis3_reader.pushFrame(&frame[0]);
                } //else if (vcid == 4) // IR

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("MSU-GS CH1 Lines        : " + std::to_string(vis1_frames));
            logger->info("MSU-GS CH2 Lines        : " + std::to_string(vis2_frames));
            logger->info("MSU-GS CH3 Lines        : " + std::to_string(vis3_frames));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image1 = vis1_reader.getImage();
            cimg_library::CImg<unsigned short> image2 = vis2_reader.getImage();
            cimg_library::CImg<unsigned short> image3 = vis3_reader.getImage();

            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/MSU-GS-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/MSU-GS-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/MSU-GS-3.png");
        }

        void ELEKTROMSUGSDecoderModule::drawUI()
        {
            ImGui::Begin("ELEKTRO MSU-GS Decoder", NULL, NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

            ImGui::End();
        }

        std::string ELEKTROMSUGSDecoderModule::getID()
        {
            return "elektro_msugs";
        }

        std::vector<std::string> ELEKTROMSUGSDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> ELEKTROMSUGSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<ELEKTROMSUGSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msugs
} // namespace elektro