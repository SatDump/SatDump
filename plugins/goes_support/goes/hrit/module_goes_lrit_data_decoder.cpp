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
                                                                                                                                                write_unknown(parameters["write_unknown"].get<bool>())
        {
            write_dcs = parameters.contains("write_dcs") ? parameters["write_dcs"].get<bool>() : false;
            write_lrit = parameters.contains("write_lrit") ? parameters["write_lrit"].get<bool>() : false;
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

            directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));
            goes_r_fc_composer_full_disk->directory = goes_r_fc_composer_meso1->directory = goes_r_fc_composer_meso2->directory = directory;

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            ::lrit::LRITDemux lrit_demux;

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

            lrit_demux.onFinalizeData =
                [this](::lrit::LRITFile& file) -> void
            {
                //On image data, make sure buffer contains the right amount of data
                if (file.hasHeader<::lrit::ImageStructureRecord>() && file.hasHeader<::lrit::PrimaryHeader>() && file.hasHeader<NOAALRITHeader>())
                {
                    ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();
                    ::lrit::ImageStructureRecord image_header = file.getHeader<::lrit::ImageStructureRecord>();
                    NOAALRITHeader noaa_header = file.getHeader<NOAALRITHeader>();

                    if (primary_header.file_type_code == 0 &&
                        (image_header.compression_flag == 0 || (image_header.compression_flag == 1 && noaa_header.noaa_specific_compression == 1)))
                    {
                        uint32_t target_size = image_header.lines_count * image_header.columns_count + primary_header.total_header_length;
                        if (file.lrit_data.size() != target_size)
                            file.lrit_data.resize(target_size, 0);
                    }
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
                {
                    processLRITFile(file);
                    if(write_lrit)
                        saveLRITFile(file, directory + "/LRIT");
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0f) / 10.0f) + "%%");
                }
            }

            data_in.close();

            for (auto &segmentedDecoder : segmentedDecoders)
            {
                if (segmentedDecoder.second.image.size())
                    segmentedDecoder.second.image.save_img(std::string(directory + "/IMAGES/" + segmentedDecoder.second.filename).c_str());
            }

            if (goes_r_fc_composer_full_disk->hasData)
                goes_r_fc_composer_full_disk->save();
            if (goes_r_fc_composer_meso1->hasData)
                goes_r_fc_composer_meso1->save();
            if (goes_r_fc_composer_meso2->hasData)
                goes_r_fc_composer_meso2->save();
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
                        memset(dec->textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                        dec->hasToUpdate = true;
                    }

                    if (dec->imageStatus != IDLE)
                    {
                        if (dec->hasToUpdate)
                        {
                            dec->hasToUpdate = false;
                            updateImageTexture(dec->textureID, dec->textureBuffer, 1000, 1000);
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
                        memset(goes_r_fc_composer_full_disk->textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                        goes_r_fc_composer_full_disk->hasToUpdate = true;
                    }

                    if (goes_r_fc_composer_full_disk->imageStatus != IDLE)
                    {
                        if (goes_r_fc_composer_full_disk->hasToUpdate)
                        {
                            goes_r_fc_composer_full_disk->hasToUpdate = false;
                            updateImageTexture(goes_r_fc_composer_full_disk->textureID,
                                               goes_r_fc_composer_full_disk->textureBuffer,
                                               1000, 1000);
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
                        memset(goes_r_fc_composer_meso1->textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                        goes_r_fc_composer_meso1->hasToUpdate = true;
                    }

                    if (goes_r_fc_composer_meso1->imageStatus != IDLE)
                    {
                        if (goes_r_fc_composer_meso1->hasToUpdate)
                        {
                            goes_r_fc_composer_meso1->hasToUpdate = false;
                            updateImageTexture(goes_r_fc_composer_meso1->textureID,
                                               goes_r_fc_composer_meso1->textureBuffer,
                                               1000, 1000);
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
                        memset(goes_r_fc_composer_meso2->textureBuffer, 0, sizeof(uint32_t) * 1000 * 1000);
                        goes_r_fc_composer_meso2->hasToUpdate = true;
                    }

                    if (goes_r_fc_composer_meso2->imageStatus != IDLE)
                    {
                        if (goes_r_fc_composer_meso2->hasToUpdate)
                        {
                            goes_r_fc_composer_meso2->hasToUpdate = false;
                            updateImageTexture(goes_r_fc_composer_meso2->textureID,
                                               goes_r_fc_composer_meso2->textureBuffer,
                                               1000, 1000);
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
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESLRITDataDecoderModule::getID()
        {
            return "goes_lrit_data_decoder";
        }

        std::vector<std::string> GOESLRITDataDecoderModule::getParameters()
        {
            return {"write_images", "write_emwin", "write_messages", "write_lrit", "write_dcs", "write_unknown"};
        }

        std::shared_ptr<ProcessingModule> GOESLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace hrit
} // namespace goes
