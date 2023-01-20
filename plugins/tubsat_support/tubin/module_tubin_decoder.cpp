#include "module_tubin_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "../png_fix.h"

namespace tubin
{
    TUBINDecoderModule::TUBINDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                              d_check_crc(parameters["check_crc"])
    {
    }

    bool TUBINDecoderModule::crc_valid(uint8_t *cadu)
    {
        uint64_t crc_frm = cadu[552 - 2] << 8 | cadu[552 - 1];
        uint64_t crc_com = crc_check.compute(&cadu[8], 552 - 2 - 8);
        return crc_frm == crc_com;
    }

    void TUBINDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        time_t lastTime = 0;

        uint8_t cadu[552];

        const int payload_size = 514 - 2; // CADU Payload area size

        while (!data_in.eof())
        {
            // Read cadu
            data_in.read((char *)cadu, 552);

            if (cadu[4] == 0x20) // Imagery Channel, VIS
            {
                if (crc_valid(cadu) || !d_check_crc)
                {
                    // File chunk
                    uint32_t chunk_counter = cadu[21] << 24 |
                                             cadu[22] << 16 |
                                             cadu[23] << 8 |
                                             cadu[24];
                    // File ID
                    uint64_t file_id = (uint64_t)cadu[26] << 56 |
                                       (uint64_t)cadu[27] << 48 |
                                       (uint64_t)cadu[28] << 40 |
                                       (uint64_t)cadu[29] << 32 |
                                       (uint64_t)cadu[31] << 24 |
                                       (uint64_t)cadu[32] << 16 |
                                       (uint64_t)cadu[33] << 8 |
                                       (uint64_t)cadu[34];

                    // logger->info("{:d} {:d}", chunk_counter, file_id);

                    if (all_files_vis.count(file_id) == 0)
                        all_files_vis.insert({file_id, std::vector<uint8_t>()});

                    if (chunk_counter * payload_size <= 67108864)
                    {
                        if (all_files_vis[file_id].size() < chunk_counter * payload_size + payload_size)
                            all_files_vis[file_id].resize(chunk_counter * payload_size + payload_size);

                        memcpy(&all_files_vis[file_id][chunk_counter * payload_size], &cadu[38], payload_size);
                    }
                }
                else
                {
                    logger->error("Bad CRC");
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

        logger->info("Processing images...");

        int norad = 48900;
        std::string sat_name = "TUBIN";

        // Products dataset
        satdump::ProductDataSet dataset;
        dataset.satellite_name = sat_name;
        dataset.timestamp = time(0);

        // Satellite ID
        {
            logger->info("----------- Satellite");
            logger->info("NORAD : " + std::to_string(norad));
            logger->info("Name  : " + sat_name);
        }

        for (auto f : all_files_vis) // VIS
        {
            if (f.second.size() <= 512 * 4)
                continue; // Discard small files

            std::vector<uint8_t> out;

            // TUBIN's format is *so* robust to noise PNGs,
            // they are often at least a bit corrupted...
            // Hence, this function will re-construct a valid
            // PNG as much as possible, in order to allow
            // decoding as much imagery that can actually be.
            if (png_fix::repair_png(f.second, out))
            {
                logger->error("Could not repair PNG, discarding...");
                continue;
            }

            image::Image<uint16_t> image;
            image.load_png(out.data(), out.size());

            logger->info("New image is {:d}x{:d}", image.width(), image.height());

            if (image.size() > 0)
            {
                image::Image<uint16_t> image_ch1(image.width() / 2, image.height() / 2, 1);
                image::Image<uint16_t> image_ch2(image.width() / 2, image.height() / 2, 1);
                image::Image<uint16_t> image_ch3(image.width() / 2, image.height() / 2, 1);
                image::Image<uint16_t> image_ch4(image.width() / 2, image.height() / 2, 1);

                // Deinterleave
                for (int x = 0; x < (int)image.width() / 2; x++)
                {
                    for (int y = 0; y < (int)image.height() / 2; y++)
                    {
                        image_ch1[y * image_ch1.width() + x] = image[(y * 2 + 0) * image.width() + (x * 2 + 0)];
                        image_ch2[y * image_ch1.width() + x] = image[(y * 2 + 0) * image.width() + (x * 2 + 1)];
                        image_ch3[y * image_ch1.width() + x] = image[(y * 2 + 1) * image.width() + (x * 2 + 0)];
                        image_ch4[y * image_ch1.width() + x] = image[(y * 2 + 1) * image.width() + (x * 2 + 1)];
                    }
                }

                std::string product_name = "VIS_" + std::to_string(f.first);
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/" + product_name;

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- TUBIN Vis");
                logger->info("Width  : " + std::to_string(image.width() / 2));
                logger->info("Height : " + std::to_string(image.height() / 2));

                satdump::ImageProducts tubin_products;
                tubin_products.instrument_name = "tubin_vis";
                // tubin_products.has_timestamps = true;
                // tubin_products.set_tle(satellite_tle);
                tubin_products.bit_depth = 16;
                // tubin_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                // tubin_products.set_timestamps(avhrr_reader.timestamps);
                // tubin_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/metop_abc_avhrr.json")));

                tubin_products.images.push_back({"TUBIN-1.png", "1", image_ch1});
                tubin_products.images.push_back({"TUBIN-2.png", "2", image_ch2});
                tubin_products.images.push_back({"TUBIN-3.png", "3", image_ch3});
                tubin_products.images.push_back({"TUBIN-4.png", "4", image_ch4});

                tubin_products.save(directory);
                dataset.products_list.push_back(product_name);
            }
            else
            {
                logger->error("Image could not be read, discarding...");
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }
    }

    void TUBINDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("TUBIN Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string TUBINDecoderModule::getID()
    {
        return "tubin_decoder";
    }

    std::vector<std::string> TUBINDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> TUBINDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<TUBINDecoderModule>(input_file, output_file_hint, parameters);
    }
}