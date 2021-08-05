#include "module_gk2a_lrit_data_decoder.h"
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

namespace gk2a
{
    namespace lrit
    {
        GK2ALRITDataDecoderModule::GK2ALRITDataDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                    write_images(std::stoi(parameters["write_images"])),
                                                                                                                                                                    write_additional(std::stoi(parameters["write_additional"])),
                                                                                                                                                                    write_unknown(std::stoi(parameters["write_unknown"]))
        {
            windowTitle = "GK-2A LRIT Data Decoder";
        }

        std::vector<ModuleDataType> GK2ALRITDataDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GK2ALRITDataDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GK2ALRITDataDecoderModule::~GK2ALRITDataDecoderModule()
        {
            for (std::pair<int, std::shared_ptr<LRITDataDecoder>> decMap : decoders)
            {
                std::shared_ptr<LRITDataDecoder> dec = decMap.second;

                for (std::pair<const std::string, SegmentedLRITImageDecoder> &dec2 : dec->segmentedDecoders)
                {
                    std::string channel = dec2.first;
                    if (dec->textureIDs[channel] > 0)
                    {
                        delete[] dec->textureBuffers[channel];
                    }
                }
            }
        }

        void GK2ALRITDataDecoderModule::process()
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

            logger->warn("All credits for decoding GK-2A encrypted xRIT files goes to @sam210723, and xrit-rx over on Github.");
            logger->warn("See https://vksdr.com/xrit-rx for a lot more information!");

            logger->info("Demultiplexing and deframing...");

            std::map<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>> demuxers;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
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
                            decoders[vcdu.vcid]->write_additional = write_additional;
                            decoders[vcdu.vcid]->write_unknown = write_unknown;
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

        void GK2ALRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin(windowTitle.c_str(), NULL, window ? NULL : NOWINDOW_FLAGS);

            if (ImGui::BeginTabBar("Images TabBar", ImGuiTabBarFlags_None))
            {
                bool hasImage = false;

                for (std::pair<int, std::shared_ptr<LRITDataDecoder>> decMap : decoders)
                {
                    std::shared_ptr<LRITDataDecoder> dec = decMap.second;

                    for (std::pair<const std::string, SegmentedLRITImageDecoder> &dec2 : dec->segmentedDecoders)
                    {
                        std::string channel = dec2.first;

                        if (dec->textureIDs[channel] == 0)
                        {
                            dec->textureIDs[channel] = makeImageTexture();
                            dec->textureBuffers[channel] = new uint32_t[1000 * 1000];
                        }

                        if (dec->imageStatus[channel] != IDLE)
                        {
                            if (dec->hasToUpdates[channel])
                            {
                                dec->hasToUpdates[channel] = true;
                                updateImageTexture(dec->textureIDs[channel], dec->textureBuffers[channel], dec->img_widths[channel], dec->img_heights[channel]);
                            }

                            hasImage = true;

                            if (ImGui::BeginTabItem(std::string(channel).c_str()))
                            {
                                ImGui::Image((void *)(intptr_t)dec->textureIDs[channel], {200 * ui_scale, 200 * ui_scale});
                                ImGui::SameLine();
                                ImGui::BeginGroup();
                                ImGui::Button("Status", {200 * ui_scale, 20 * ui_scale});
                                if (dec->imageStatus[channel] == SAVING)
                                    ImGui::TextColored(IMCOLOR_SYNCED, "Writing image...");
                                else if (dec->imageStatus[channel] == RECEIVING)
                                    ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                                else
                                    ImGui::TextColored(IMCOLOR_NOSYNC, "Idle (Image)...");
                                ImGui::EndGroup();
                                ImGui::EndTabItem();
                            }
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

        std::string GK2ALRITDataDecoderModule::getID()
        {
            return "gk2a_lrit_data_decoder";
        }

        std::vector<std::string> GK2ALRITDataDecoderModule::getParameters()
        {
            return {"write_images", "write_additional", "write_unknown"};
        }

        std::shared_ptr<ProcessingModule> GK2ALRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<GK2ALRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop