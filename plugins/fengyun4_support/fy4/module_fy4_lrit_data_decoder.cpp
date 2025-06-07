#include "module_fy4_lrit_data_decoder.h"
#include "common/lrit/lrit_demux.h"
#include "common/utils.h"
#include "core/resources.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "lrit_header.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fy4
{
    namespace lrit
    {
        FY4LRITDataDecoderModule::FY4LRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        FY4LRITDataDecoderModule::~FY4LRITDataDecoderModule()
        {
#if 0
            for (auto &decMap : all_wip_images)
            {
                auto &dec = decMap.second;

                if (dec->textureID > 0)
                {
                    delete[] dec->textureBuffer;
                }
            }
#endif
        }

        void FY4LRITDataDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            uint8_t cadu[1024];

#if 0
            if (resources::resourceExists("gk2a/EncryptionKeyMessage.bin"))
            {
                // Load decryption keys
                std::ifstream keyFile(resources::getResourcePath("gk2a/EncryptionKeyMessage.bin"), std::ios::binary);

                // Read key count
                uint16_t key_count = 0;
                keyFile.read((char *)&key_count, 2);
                key_count = (key_count & 0xFF) << 8 | (key_count >> 8);

                uint8_t readBuf[10];
                for (int i = 0; i < key_count; i++)
                {
                    keyFile.read((char *)readBuf, 10);
                    int index = readBuf[0] << 8 | readBuf[1];
                    uint64_t key = (uint64_t)readBuf[2] << 56 |
                                   (uint64_t)readBuf[3] << 48 |
                                   (uint64_t)readBuf[4] << 40 |
                                   (uint64_t)readBuf[5] << 32 |
                                   (uint64_t)readBuf[6] << 24 |
                                   (uint64_t)readBuf[7] << 16 |
                                   (uint64_t)readBuf[8] << 8 |
                                   (uint64_t)readBuf[9];
                    std::memcpy(&key, &readBuf[2], 8);
                    decryption_keys.emplace(std::pair<int, uint64_t>(index, key));
                }

                keyFile.close();
            }
            else
            {
                logger->critical("GK-2A Decryption keys could not be loaded!!! Nothing apart from encrypted data will be decoded.");
            }
#endif

            logger->info("Demultiplexing and deframing...");

            ::lrit::LRITDemux lrit_demux(1012, false);

            this->directory = directory;

            lrit_demux.onParseHeader = [](::lrit::LRITFile &file) -> void
            {
                // Check if this is image data
                if (file.hasHeader<ImageInformationRecord>())
                {
                    ImageInformationRecord image_structure_record = file.getHeader<ImageInformationRecord>(); //(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                    logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

#if 0
                    if (image_structure_record.compression_flag == 2 /* Progressive JPEG */)
                    {
                        logger->debug("JPEG Compression is used, decompressing...");
                        file.custom_flags.insert_or_assign(JPG_COMPRESSED, true);
                        file.custom_flags.insert_or_assign(J2K_COMPRESSED, false);
                    }
                    else if (image_structure_record.compression_flag == 1 /* Wavelet */)
                    {
                        logger->debug("Wavelet Compression is used, decompressing...");
                        file.custom_flags.insert_or_assign(JPG_COMPRESSED, false);
                        file.custom_flags.insert_or_assign(J2K_COMPRESSED, true);
                    }
                    else
                    {
                        file.custom_flags.insert_or_assign(JPG_COMPRESSED, false);
                        file.custom_flags.insert_or_assign(J2K_COMPRESSED, false);
                    }
#endif
                }

                if (file.hasHeader<KeyHeader>())
                {
                    KeyHeader key_header = file.getHeader<KeyHeader>();
                    if (key_header.key != 0)
                    {
                        logger->debug("This is encrypted!");
                        file.custom_flags.insert_or_assign(IS_ENCRYPTED, true);
                        file.custom_flags.insert_or_assign(KEY_INDEX, key_header.key);
                    }
                    else
                    {
                        file.custom_flags.insert_or_assign(IS_ENCRYPTED, false);
                    }
                }
                else
                {
                    file.custom_flags.insert_or_assign(IS_ENCRYPTED, false);
                }
            };

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                std::vector<::lrit::LRITFile> files = lrit_demux.work(cadu);

                for (auto &file : files)
                    processLRITFile(file);
            }

            cleanup();

            for (auto &segmentedDecoder : segmentedDecoders)
                if (segmentedDecoder.second.image_id != "")
                    image::save_img(segmentedDecoder.second.image, std::string(directory + "/IMAGES/" + segmentedDecoder.second.image_id).c_str());
        }

        void FY4LRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FY-4x LRIT Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

            drawProgressBar();

            ImGui::End();
        }

        std::string FY4LRITDataDecoderModule::getID() { return "fy4_lrit_data_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> FY4LRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FY4LRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace lrit
} // namespace fy4