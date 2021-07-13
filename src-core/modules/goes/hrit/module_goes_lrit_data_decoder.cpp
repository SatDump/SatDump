#include "module_goes_lrit_data_decoder.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace hrit
    {
        GOESLRITDataDecoderModule::GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                    write_images(std::stoi(parameters["write_images"])),
                                                                                                                                                                    write_emwin(std::stoi(parameters["write_emwin"])),
                                                                                                                                                                    write_messages(std::stoi(parameters["write_messages"])),
                                                                                                                                                                    write_dcs(std::stoi(parameters["write_dcs"])),
                                                                                                                                                                    write_unknown(std::stoi(parameters["write_unknown"]))
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
            for (std::pair<int, std::shared_ptr<LRITDataDecoder>> decMap : decoders)
            {
                std::shared_ptr<LRITDataDecoder> dec = decMap.second;

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

            std::map<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>> demuxers;

            while (!data_in.eof())
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)&cadu, 1024);
                else
                    input_fifo->read((uint8_t *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.vcid == 63)
                    continue;

                if (demuxers.count(vcdu.vcid) <= 0)
                    demuxers.emplace(std::pair<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>>(vcdu.vcid, std::make_shared<ccsds::ccsds_1_0_1024::Demuxer>(884, false)));

                // Demux
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxers[vcdu.vcid]->work(cadu);

                // Push into processor (filtering APID 103 and 104)
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                {
                    if (pkt.header.apid != 2047)
                    {
                        if (decoders.count(vcdu.vcid) <= 0)
                        {
                            decoders.emplace(std::pair<int, std::shared_ptr<LRITDataDecoder>>(vcdu.vcid, std::make_shared<LRITDataDecoder>(directory)));
                            decoders[vcdu.vcid]->write_images = write_images;
                            decoders[vcdu.vcid]->write_emwin = write_emwin;
                            decoders[vcdu.vcid]->write_messages = write_messages;
                            decoders[vcdu.vcid]->write_dcs = write_dcs;
                            decoders[vcdu.vcid]->write_unknown = write_unknown;

                            decoders[vcdu.vcid]->goes_r_fc_composer_full_disk = goes_r_fc_composer_full_disk;
                            decoders[vcdu.vcid]->goes_r_fc_composer_meso1 = goes_r_fc_composer_meso1;
                            decoders[vcdu.vcid]->goes_r_fc_composer_meso2 = goes_r_fc_composer_meso2;
                        }
                        decoders[vcdu.vcid]->work(pkt);
                    }
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            for (const std::pair<const int, std::shared_ptr<LRITDataDecoder>> &dec : decoders)
                dec.second->save();
        }

        void GOESLRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES HRIT Data Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            if (ImGui::BeginTabBar("Images TabBar", ImGuiTabBarFlags_None))
            {
                bool hasImage = false;

                for (std::pair<int, std::shared_ptr<LRITDataDecoder>> decMap : decoders)
                {
                    std::shared_ptr<LRITDataDecoder> dec = decMap.second;

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

        std::shared_ptr<ProcessingModule> GOESLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<GOESLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop