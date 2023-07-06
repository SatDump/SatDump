#include "module_goes_lrit_data_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "common/lrit/lrit_demux.h"
#include "lrit_header.h"

namespace goes
{
    namespace hrit
    {
        GOESLRITDataDecoderModule::GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                write_images(parameters["write_images"].get<bool>()),
                                                                                                                                                write_emwin(parameters["write_emwin"].get<bool>()),
                                                                                                                                                write_messages(parameters["write_messages"].get<bool>()),
                                                                                                                                                write_dcs(parameters["write_dcs"].get<bool>()),
                                                                                                                                                write_unknown(parameters["write_unknown"].get<bool>())
        {
            goes_r_fc_composer_full_disk = std::make_shared<GOESRFalseColorComposer>();
            goes_r_fc_composer_meso1 = std::make_shared<GOESRFalseColorComposer>();
            goes_r_fc_composer_meso2 = std::make_shared<GOESRFalseColorComposer>();
        }

        std::vector<ModuleDataType> GOESLRITDataDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESLRITDataDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GOESLRITDataDecoderModule::~GOESLRITDataDecoderModule()
        {
            for (auto &decMap : all_wip_images)
            {
                auto &dec = decMap.second;

                if (dec->textureID > 0)
                {
                    delete[] dec->textureBuffer;
                }
            }

            if (goes_r_fc_composer_full_disk->textureID > 0)
                delete[] goes_r_fc_composer_full_disk->textureBuffer;
            if (goes_r_fc_composer_meso1->textureID > 0)
                delete[] goes_r_fc_composer_meso1->textureBuffer;
            if (goes_r_fc_composer_meso2->textureID > 0)
                delete[] goes_r_fc_composer_meso2->textureBuffer;
        }

        void GOESLRITDataDecoderModule::process()
        {
            std::ifstream data_in;

            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            ::lrit::LRITDemux lrit_demux;

            this->directory = directory;

            lrit_demux.onParseHeader =
                [this](::lrit::LRITFile &file) -> void
            {
                file.custom_flags.insert({RICE_COMPRESSED, false});

                // Check if this is image data
                if (file.hasHeader<::lrit::ImageStructureRecord>())
                {
                    ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>(); //(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                    logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

                    NOAALRITHeader noaa_header = file.getHeader<NOAALRITHeader>();

                    if (image_structure_record.compression_flag == 1 && noaa_header.noaa_specific_compression == 1) // Check this is RICE
                    {
                        file.custom_flags.insert_or_assign(RICE_COMPRESSED, true);

                        if (rice_parameters_all.count(file.filename) == 0)
                            rice_parameters_all.insert({file.filename, SZ_com_t()});

                        SZ_com_t &rice_parameters = rice_parameters_all[file.filename];

                        logger->debug("This is RICE-compressed! Decompressing...");

                        rice_parameters.bits_per_pixel = image_structure_record.bit_per_pixel;
                        rice_parameters.pixels_per_block = 16;
                        rice_parameters.pixels_per_scanline = image_structure_record.columns_count;
                        rice_parameters.options_mask = SZ_ALLOW_K13_OPTION_MASK | SZ_MSB_OPTION_MASK | SZ_NN_OPTION_MASK | SZ_RAW_OPTION_MASK;

                        if (file.hasHeader<RiceCompressionHeader>())
                        {
                            RiceCompressionHeader rice_compression_header = file.getHeader<RiceCompressionHeader>();
                            logger->debug("Rice header is present!");
                            logger->debug("Pixels per block : " + std::to_string(rice_compression_header.pixels_per_block));
                            logger->debug("Scan lines per packet : " + std::to_string(rice_compression_header.scanlines_per_packet));
                            rice_parameters.pixels_per_block = rice_compression_header.pixels_per_block;
                            rice_parameters.options_mask = rice_compression_header.flags | SZ_RAW_OPTION_MASK;
                        }
                        else if (rice_parameters.pixels_per_block <= 0)
                        {
                            logger->critical("Pixel per blocks is set to 0! Using defaults");
                            rice_parameters.pixels_per_block = 16;
                        }
                    }
                }
            };

            lrit_demux.onProcessData =
                [this](::lrit::LRITFile &file, ccsds::CCSDSPacket &pkt) -> bool
            {
                if (file.custom_flags[RICE_COMPRESSED])
                {
                    SZ_com_t &rice_parameters = rice_parameters_all[file.filename];

                    if (rice_parameters.bits_per_pixel == 0)
                        return false;

                    std::vector<uint8_t> decompression_buffer(rice_parameters.pixels_per_scanline);

                    size_t output_size = decompression_buffer.size();
                    int r = SZ_BufftoBuffDecompress(decompression_buffer.data(), &output_size, pkt.payload.data(), pkt.payload.size() - 2, &rice_parameters);
                    if (r == AEC_OK)
                        file.lrit_data.insert(file.lrit_data.end(), &decompression_buffer.data()[0], &decompression_buffer.data()[output_size]);
                    else
                        logger->warn("Rice decompression failed. This may be an issue!");

                    return false;
                }
                else
                {
                    return true;
                }
            };

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&cadu, 1024);
                else
                    input_fifo->read((uint8_t *)&cadu, 1024);

                std::vector<::lrit::LRITFile> files = lrit_demux.work(cadu);

                for (auto &file : files)
                    processLRITFile(file);

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%%");
                }
            }

            data_in.close();

            for (auto &segmentedDecoder : segmentedDecoders)
            {
                if (segmentedDecoder.second.image.size())
                {
                    logger->info("Writing image " + directory + "/IMAGES/" + segmentedDecoder.second.filename + ".png" + "...");
                    segmentedDecoder.second.image.save_png(std::string(directory + "/IMAGES/" + segmentedDecoder.second.filename + ".png").c_str());
                }
            }

            if (goes_r_fc_composer_full_disk->hasData)
                goes_r_fc_composer_full_disk->save(directory);
            if (goes_r_fc_composer_meso1->hasData)
                goes_r_fc_composer_meso1->save(directory);
            if (goes_r_fc_composer_meso2->hasData)
                goes_r_fc_composer_meso2->save(directory);
        }

        void GOESLRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES HRIT Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTabBar("Images TabBar", ImGuiTabBarFlags_None))
            {
                bool hasImage = false;

                for (auto &decMap : all_wip_images)
                {
                    auto &dec = decMap.second;

                    if (dec->textureID == 0)
                    {
                        dec->textureID = makeImageTexture();
                        dec->textureBuffer = new uint32_t[1000 * 1000];
                    }

                    if (dec->imageStatus != IDLE)
                    {
                        if (dec->hasToUpdate)
                        {
                            dec->hasToUpdate = true;
                            updateImageTexture(dec->textureID, dec->textureBuffer, dec->img_width, dec->img_height);
                        }

                        hasImage = true;

                        if (ImGui::BeginTabItem(std::string("VCID " + std::to_string(decMap.first)).c_str()))
                        {
                            ImGui::Image((void *)(intptr_t)dec->textureID, {200 * ui_scale, 200 * ui_scale});
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                            if (dec->imageStatus == SAVING)
                                ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                            else if (dec->imageStatus == RECEIVING)
                                ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                            else
                                ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                            ImGui::EndGroup();
                            ImGui::EndTabItem();
                        }
                    }
                }

                // Full disk FC
                {
                    if (goes_r_fc_composer_full_disk->textureID == 0)
                    {
                        goes_r_fc_composer_full_disk->textureID = makeImageTexture();
                        goes_r_fc_composer_full_disk->textureBuffer = new uint32_t[1000 * 1000];
                    }

                    if (goes_r_fc_composer_full_disk->imageStatus != IDLE)
                    {
                        if (goes_r_fc_composer_full_disk->hasToUpdate)
                        {
                            goes_r_fc_composer_full_disk->hasToUpdate = true;
                            updateImageTexture(goes_r_fc_composer_full_disk->textureID,
                                               goes_r_fc_composer_full_disk->textureBuffer,
                                               goes_r_fc_composer_full_disk->img_width,
                                               goes_r_fc_composer_full_disk->img_height);
                        }

                        hasImage = true;

                        if (ImGui::BeginTabItem("FD Color"))
                        {
                            ImGui::Image((void *)(intptr_t)goes_r_fc_composer_full_disk->textureID, {200 * ui_scale, 200 * ui_scale});
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                            if (goes_r_fc_composer_full_disk->imageStatus == SAVING)
                                ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                            else if (goes_r_fc_composer_full_disk->imageStatus == RECEIVING)
                                ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                            else
                                ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                            ImGui::EndGroup();
                            ImGui::EndTabItem();
                        }
                    }
                }

                // Meso 1 FC
                {
                    if (goes_r_fc_composer_meso1->textureID == 0)
                    {
                        goes_r_fc_composer_meso1->textureID = makeImageTexture();
                        goes_r_fc_composer_meso1->textureBuffer = new uint32_t[1000 * 1000];
                    }

                    if (goes_r_fc_composer_meso1->imageStatus != IDLE)
                    {
                        if (goes_r_fc_composer_meso1->hasToUpdate)
                        {
                            goes_r_fc_composer_meso1->hasToUpdate = true;
                            updateImageTexture(goes_r_fc_composer_meso1->textureID,
                                               goes_r_fc_composer_meso1->textureBuffer,
                                               goes_r_fc_composer_meso1->img_width,
                                               goes_r_fc_composer_meso1->img_height);
                        }

                        hasImage = true;

                        if (ImGui::BeginTabItem("Meso 1 Color"))
                        {
                            ImGui::Image((void *)(intptr_t)goes_r_fc_composer_meso1->textureID, {200 * ui_scale, 200 * ui_scale});
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                            if (goes_r_fc_composer_meso1->imageStatus == SAVING)
                                ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                            else if (goes_r_fc_composer_meso1->imageStatus == RECEIVING)
                                ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                            else
                                ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                            ImGui::EndGroup();
                            ImGui::EndTabItem();
                        }
                    }
                }

                // Meso 2 FC
                {
                    if (goes_r_fc_composer_meso2->textureID == 0)
                    {
                        goes_r_fc_composer_meso2->textureID = makeImageTexture();
                        goes_r_fc_composer_meso2->textureBuffer = new uint32_t[1000 * 1000];
                    }

                    if (goes_r_fc_composer_meso2->imageStatus != IDLE)
                    {
                        if (goes_r_fc_composer_meso2->hasToUpdate)
                        {
                            goes_r_fc_composer_meso2->hasToUpdate = true;
                            updateImageTexture(goes_r_fc_composer_meso2->textureID,
                                               goes_r_fc_composer_meso2->textureBuffer,
                                               goes_r_fc_composer_meso2->img_width,
                                               goes_r_fc_composer_meso2->img_height);
                        }

                        hasImage = true;

                        if (ImGui::BeginTabItem("Meso 2 Color"))
                        {
                            ImGui::Image((void *)(intptr_t)goes_r_fc_composer_meso2->textureID, {200 * ui_scale, 200 * ui_scale});
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                            if (goes_r_fc_composer_meso2->imageStatus == SAVING)
                                ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                            else if (goes_r_fc_composer_meso2->imageStatus == RECEIVING)
                                ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                            else
                                ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                            ImGui::EndGroup();
                            ImGui::EndTabItem();
                        }
                    }
                }

                if (!hasImage) // Add empty tab if there is no image yet
                {
                    if (ImGui::BeginTabItem("No image yet"))
                    {
                        ImGui::Dummy({200 * ui_scale, 200 * ui_scale});
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                        ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                }
            }
            ImGui::EndTabBar();

            if (!streamingInput)
                ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESLRITDataDecoderModule::getID()
        {
            return "goes_lrit_data_decoder";
        }

        std::vector<std::string> GOESLRITDataDecoderModule::getParameters()
        {
            return {"write_images", "write_emwin", "write_messages", "write_dcs", "write_unknown"};
        }

        std::shared_ptr<ProcessingModule> GOESLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop