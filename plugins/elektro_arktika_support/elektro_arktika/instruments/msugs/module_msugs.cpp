#include "module_msugs.h"
#include "common/simple_deframer.h"
#include "common/utils.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "products/dataset.h"
#include "products/image_product.h"

#include "common/tracking/tle.h"
#include "core/resources.h"
#include "nlohmann/json_utils.h"

namespace elektro_arktika
{
    namespace msugs
    {
        MSUGSDecoderModule::MSUGSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void MSUGSDecoderModule::process()
        {

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSU-GS";

            uint8_t cadu[1024];

            def::SimpleDeframer deframerVIS1(0x0218a7a392dd9abf, 64, 121680, 10, true);
            def::SimpleDeframer deframerVIS2(0x0218a7a392dd9abf, 64, 121680, 10, true);
            def::SimpleDeframer deframerVIS3(0x0218a7a392dd9abf, 64, 121680, 10, true);
            def::SimpleDeframer deframerIR(0x0218a7a392dd9abf, 64, 14560, 10, true);
            // def::SimpleDeframer deframerUnknown(0xa6007c, 24, 1680, 0, false);

            // std::ofstream data_unknown(directory + "/data_unknown.bin", std::ios::binary);

            logger->info("Demultiplexing and deframing...");

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)cadu, 1024);

                int vcid = (cadu[5] >> 1) & 7;
                int vcid_2 = (cadu[11] >> 1) & 7;

                if ((vcid == 2) || (vcid_2 == 2))
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS1.work(&cadu[24], 1024 - 24);
                    for (std::vector<uint8_t> &frame : frames)
                        vis1_reader.pushFrame(&frame[0]);
                }
                else if ((vcid == 3) || (vcid_2 == 3))
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS2.work(&cadu[24], 1024 - 24);
                    for (std::vector<uint8_t> &frame : frames)
                        vis2_reader.pushFrame(&frame[0]);
                }
                else if ((vcid == 5) || (vcid_2 == 5))
                {
                    std::vector<std::vector<uint8_t>> frames = deframerVIS3.work(&cadu[24], 1024 - 24);
                    for (std::vector<uint8_t> &frame : frames)
                        vis3_reader.pushFrame(&frame[0]);
                }
                else if ((vcid == 4) || (vcid_2 == 4))
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
            }

            // data_unknown.close();
            cleanup();

            // TODOREWORK satelliteID
            std::string sat_name = "ELEKTRO-L";

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = time(0); // satdump::get_median(vis1_reader.timestamps);

            logger->info("----------- MSU-GS");
            logger->info("MSU-GS CH1 Lines        : " + std::to_string(vis1_reader.frames));
            logger->info("MSU-GS CH2 Lines        : " + std::to_string(vis2_reader.frames));
            logger->info("MSU-GS CH3 Lines        : " + std::to_string(vis3_reader.frames));
            logger->info("MSU-GS IR Frames        : " + std::to_string(infr_reader.frames));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            // MSUVIS1 TODOREWORK
            {
                ////////////////////////////////////////// mtvza_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSUGS_VIS1";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                //   logger->info("----------- KMSS MSU-100 1");
                //   logger->info("Lines : " + std::to_string(kmss_lines));

                satdump::products::ImageProduct msuvis_product;
                msuvis_product.instrument_name = "msugs_vis";
                //                    msuvis_products.has_timestamps = true; // TODOREWORK
                //                    msuvis_products.set_tle(satdump::general_tle_registry.get_from_norad(norad));
                //                    msuvis_products.set_timestamps(timestamps);
                //     msuvis_product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/meteor_m2-2_kmss_msu100_1.json")));

                msuvis_product.images.push_back({0, "MSUGS-VIS-1", "1", vis1_reader.getImage1(), 10, satdump::ChannelTransform().init_none()});
                msuvis_product.images.push_back({1, "MSUGS-VIS-2", "2", vis2_reader.getImage1(), 10, satdump::ChannelTransform().init_none()});
                msuvis_product.images.push_back({2, "MSUGS-VIS-3", "3", vis3_reader.getImage1(), 10, satdump::ChannelTransform().init_none()});

                msuvis_product.save(directory);
                dataset.products_list.push_back("MSUGS_VIS1");

                ////////////////////////////////////////////////     mtvza_status = DONE;
            }

            // MSUVIS2 TODOREWORK
            {
                ////////////////////////////////////////// mtvza_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MSUGS_VIS2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                //   logger->info("----------- KMSS MSU-100 1");
                //   logger->info("Lines : " + std::to_string(kmss_lines));

                satdump::products::ImageProduct msuvis_product;
                msuvis_product.instrument_name = "msugs_vis";
                //                    msuvis_products.has_timestamps = true; // TODOREWORK
                //                    msuvis_products.set_tle(satdump::general_tle_registry.get_from_norad(norad));
                //                    msuvis_products.set_timestamps(timestamps);
                // msuvis_product.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/elektro_l3_msugs_vis2.json")),
                //                                            satdump::general_tle_registry->get_from_norad(44903), vis1_reader.timestamps);

                msuvis_product.images.push_back({0, "MSUGS-VIS-1", "1", vis1_reader.getImage2(), 10, satdump::ChannelTransform().init_none()});
                msuvis_product.images.push_back({1, "MSUGS-VIS-2", "2", vis2_reader.getImage2(), 10, satdump::ChannelTransform().init_none()});
                msuvis_product.images.push_back({2, "MSUGS-VIS-3", "3", vis3_reader.getImage2(), 10, satdump::ChannelTransform().init_none()});

                msuvis_product.save(directory);
                dataset.products_list.push_back("MSUGS_VIS2");

                ////////////////////////////////////////////////     mtvza_status = DONE;
            }

#if 0
            channels_statuses[0] = channels_statuses[1] = channels_statuses[2] = PROCESSING;
            image::Image image1 = vis1_reader.getImage();
            image::Image image2 = vis2_reader.getImage();
            image::Image image3 = vis3_reader.getImage();

            //     image1.crop(0, 1421, 12008, 1421 + 12008);
            channels_statuses[0] = SAVING;
            image::save_img(image1, directory + "/MSU-GS-1");
            channels_statuses[0] = DONE;

            //     image2.crop(0, 1421 + 1804, 12008, 1421 + 1804 + 12008);
            channels_statuses[1] = SAVING;
            image::save_img(image2, directory + "/MSU-GS-2");
            channels_statuses[1] = DONE;

            //    image3.crop(0, 1421 + 3606, 12008, 1421 + 3606 + 12008);
            channels_statuses[2] = SAVING;
            image::save_img(image3, directory + "/MSU-GS-3");
            channels_statuses[2] = DONE;
#endif

            for (int i = 0; i < 7; i++)
            {
                channels_statuses[3 + i] = PROCESSING;
                logger->info("Channel IR " + std::to_string(i + 4) + "...");
                image::Image img = infr_reader.getImage(i);
                //        img.crop(183, 3294);
                channels_statuses[3 + i] = SAVING;
                image::save_img(img, directory + "/MSU-GS-" + std::to_string(i + 4));
                channels_statuses[3 + i] = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
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
                    ImGui::TextColored(style::theme.green, "%d", frames);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(channels_statuses[i]);
                }

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string MSUGSDecoderModule::getID() { return "elektro_arktika_msugs"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> MSUGSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MSUGSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace msugs
} // namespace elektro_arktika
