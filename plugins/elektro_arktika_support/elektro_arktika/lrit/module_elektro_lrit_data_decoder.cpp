#include "module_elektro_lrit_data_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "common/lrit/lrit_demux.h"

namespace elektro
{
    namespace lrit
    {
        ELEKTROLRITDataDecoderModule::ELEKTROLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                      productizer("msu_gs", false, d_output_file_hint.substr(0, d_output_file_hint.rfind('/')))
        {
        }

        std::vector<ModuleDataType> ELEKTROLRITDataDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> ELEKTROLRITDataDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        ELEKTROLRITDataDecoderModule::~ELEKTROLRITDataDecoderModule()
        {
            for (auto &decMap : all_wip_images)
            {
                auto &dec = decMap.second;

                if (dec->textureID > 0)
                {
                    delete[] dec->textureBuffer;
                }
            }
        }

        void ELEKTROLRITDataDecoderModule::process()
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
                [](::lrit::LRITFile &file) -> void
            {
                // Check if this is image data
                if (file.hasHeader<::lrit::ImageStructureRecord>())
                {
                    ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>(); //(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                    logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

                    if (image_structure_record.compression_flag == 2 /* Progressive JPEG */)
                    {
                        logger->debug("JPEG Compression is used, decompressing...");
                        file.custom_flags.insert_or_assign(JPEG_COMPRESSED, true);
                        file.custom_flags.insert_or_assign(WT_COMPRESSED, false);
                    }
                    else if (image_structure_record.compression_flag == 1 /* Wavelet */)
                    {
                        logger->debug("Wavelet Compression is used, decompressing...");
                        file.custom_flags.insert_or_assign(JPEG_COMPRESSED, false);
                        file.custom_flags.insert_or_assign(WT_COMPRESSED, true);
                    }
                    else
                    {
                        file.custom_flags.insert_or_assign(JPEG_COMPRESSED, false);
                        file.custom_flags.insert_or_assign(WT_COMPRESSED, false);
                    }
                }
            };

            if (!std::filesystem::exists(directory + "/IMAGES/Unknown"))
                std::filesystem::create_directories(directory + "/IMAGES/Unknown");

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
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();

            for (auto &segmentedDecoder : segmentedDecoders)
                if (segmentedDecoder.second.image_id != "")
                    saveImageP(segmentedDecoder.second.meta, segmentedDecoder.second.image);
        }

        void ELEKTROLRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("ELEKTRO-L LRIT Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                            updateImageTexture(dec->textureID, dec->textureBuffer, 1000, 1000);
                            dec->hasToUpdate = false;
                        }

                        hasImage = true;

                        if (ImGui::BeginTabItem(std::string("Ch " + std::to_string(decMap.first)).c_str()))
                        {
                            ImGui::Image((void *)(intptr_t)dec->textureID, {200 * ui_scale, 200 * ui_scale});
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                            if (dec->imageStatus == SAVING)
                                ImGui::TextColored(style::theme.green, "Writing image...");
                            else if (dec->imageStatus == RECEIVING)
                                ImGui::TextColored(style::theme.orange, "Receiving...");
                            else
                                ImGui::TextColored(style::theme.red, "Idle (Image)...");
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
                        ImGui::TextColored(style::theme.red, "Idle (Image)...");
                        ImGui::EndGroup();
                        ImGui::EndTabItem();
                    }
                }
            }
            ImGui::EndTabBar();

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

            ImGui::End();
        }

        std::string ELEKTROLRITDataDecoderModule::getID()
        {
            return "elektro_lrit_data_decoder";
        }

        std::vector<std::string> ELEKTROLRITDataDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> ELEKTROLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<ELEKTROLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop