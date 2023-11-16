#include "module_msugs.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/hue_saturation.h"
#include "common/utils.h"
#include "common/simple_deframer.h"

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

            def::SimpleDeframer deframerVIS1(0x0218a7a392dd9abf, 64, 121680, 10, true);
            def::SimpleDeframer deframerVIS2(0x0218a7a392dd9abf, 64, 121680, 10, true);
            def::SimpleDeframer deframerVIS3(0x0218a7a392dd9abf, 64, 121680, 10, true);
            def::SimpleDeframer deframerIR(0x0218a7a392dd9abf, 64, 14560, 10, true);
            // def::SimpleDeframer deframerUnknown(0xa6007c, 24, 1680, 0, false);

            // std::ofstream data_unknown(directory + "/data_unknown.bin", std::ios::binary);

            logger->info("Demultiplexing and deframing...");

            int offset = d_parameters["msugs_offset"].get<int>();

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)cadu, 1024);

                int vcid = (cadu[5] >> 1) & 7;

                if (vcid == 2)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS1.work(&cadu[24], 1024 - 24);
                    for (std::vector<uint8_t> &frame : frames)
                        vis1_reader.pushFrame(&frame[0], offset);
                }
                else if (vcid == 3)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS2.work(&cadu[24], 1024 - 24);
                    for (std::vector<uint8_t> &frame : frames)
                        vis2_reader.pushFrame(&frame[0], offset);
                }
                else if (vcid == 5)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS3.work(&cadu[24], 1024 - 24);
                    for (std::vector<uint8_t> &frame : frames)
                        vis3_reader.pushFrame(&frame[0], offset);
                }
                else if (vcid == 4)
                {
                    std::vector<std::vector<uint8_t>> frames = deframerIR.work(&cadu[24], 1024 - 24);
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
                        // logger->error(marker);
                        if (marker == 5) // 0, 2, 3, 4, 5
                            data_unknown.write((char *)frame.data(), frame.size());
                    }
                }*/

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)data_in.tellg() / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            // data_unknown.close();
            data_in.close();

            logger->info("----------- MSU-GS");
            logger->info("MSU-GS CH1 Lines        : " + std::to_string(vis1_reader.frames));
            logger->info("MSU-GS CH2 Lines        : " + std::to_string(vis2_reader.frames));
            logger->info("MSU-GS CH3 Lines        : " + std::to_string(vis3_reader.frames));
            logger->info("MSU-GS IR Frames        : " + std::to_string(infr_reader.frames));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            channels_statuses[0] = channels_statuses[1] = channels_statuses[2] = PROCESSING;
            image::Image<uint16_t> image1 = vis1_reader.getImage();
            image::Image<uint16_t> image2 = vis2_reader.getImage();
            image::Image<uint16_t> image3 = vis3_reader.getImage();

            image1.crop(0, 1421, 12008, 1421 + 12008);
            channels_statuses[0] = SAVING;
            WRITE_IMAGE(image1, directory + "/MSU-GS-1");
            channels_statuses[0] = DONE;

            image2.crop(0, 1421 + 1804, 12008, 1421 + 1804 + 12008);
            channels_statuses[1] = SAVING;
            WRITE_IMAGE(image2, directory + "/MSU-GS-2");
            channels_statuses[1] = DONE;

            image3.crop(0, 1421 + 3606, 12008, 1421 + 3606 + 12008);
            channels_statuses[2] = SAVING;
            WRITE_IMAGE(image3, directory + "/MSU-GS-3");
            channels_statuses[2] = DONE;

            for (int i = 0; i < 7; i++)
            {
                channels_statuses[3 + i] = PROCESSING;
                logger->info("Channel IR " + std::to_string(i + 4) + "...");
                image::Image<uint16_t> img = infr_reader.getImage(i);
                img.crop(183, 3294);
                channels_statuses[3 + i] = SAVING;
                WRITE_IMAGE(img, directory + "/MSU-GS-" + std::to_string(i + 4));
                channels_statuses[3 + i] = DONE;
            }

            /*
            logger->info("221 Composite...");
            {
                image::Image<uint16_t> image221(image1.width(), std::max<int>(image1.height(), image2.height()), 3);
                {
                    image221.draw_image(0, image2, 0, 0);
                    image221.draw_image(1, image2, 0, 0);
                    image221.draw_image(2, image1, 0, 0);
                }
                image221.white_balance();
                WRITE_IMAGE(image221, directory + "/MSU-GS-RGB-221");
            }

            logger->info("Natural Color Composite...");
            {
                image::Image<uint16_t> imageNC(image1.width(), std::max<int>(image1.height(), std::max<int>(image2.height(), image3.height())), 3);
                {
                    imageNC.draw_image(0, image3, 0, 0);
                    imageNC.draw_image(1, image2, 0, 0);
                    imageNC.draw_image(2, image1, 0, 0);

                    image::HueSaturation hueTuning;
                    hueTuning.hue[image::HUE_RANGE_YELLOW] = -45.0 / 180.0;
                    hueTuning.hue[image::HUE_RANGE_RED] = 90.0 / 180.0;
                    hueTuning.overlap = 100.0 / 100.0;
                    image::hue_saturation(imageNC, hueTuning);

                    imageNC.white_balance();
                }
                WRITE_IMAGE(imageNC, directory + "/MSU-GS-RGB-NC");
            }
            */
        }

        void MSUGSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO / ARKTIKA MSU-GS Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##msugstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("MSU-GS Channel");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                for (int i = 0; i < 10; i++)
                {
                    int frames = 0;

                    if (i == 0)
                        frames = vis1_reader.frames;
                    else if (i == 1)
                        frames = vis2_reader.frames;
                    else if (i == 2)
                        frames = vis3_reader.frames;
                    else
                        frames = infr_reader.frames;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Channel %d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", frames);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(channels_statuses[i]);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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
