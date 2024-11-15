#include "module_goes_lrit_data_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/lrit/lrit_demux.h"
#include "lrit_header.h"

namespace goes
{
    namespace hrit
    {
        GOESLRITDataDecoderModule::GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                write_images(parameters["write_images"].get<bool>()),
                                                                                                                                                write_emwin(parameters["write_emwin"].get<bool>()),
                                                                                                                                                write_dcs(parameters["write_dcs"].get<bool>()),
                                                                                                                                                write_messages(parameters["write_messages"].get<bool>()),
                                                                                                                                                write_unknown(parameters["write_unknown"].get<bool>()),
                                                                                                                                                max_fill_lines(parameters["max_fill_lines"].get<int>()),
                                                                                                                                                productizer("abi", true, d_output_file_hint.substr(0, d_output_file_hint.rfind('/')))
        {
            fill_missing = parameters.contains("fill_missing") ? parameters["fill_missing"].get<bool>() : false;
            parse_dcs = parameters.contains("parse_dcs") ? parameters["parse_dcs"].get<bool>() : false;
            write_lrit = parameters.contains("write_lrit") ? parameters["write_lrit"].get<bool>() : false;

            if (parse_dcs && parameters.contains("tracked_addresses") && parameters["tracked_addresses"].is_string())
            {
                std::string tracked_address_str = parameters["tracked_addresses"];
                std::string tracked_address_token;
                tracked_address_str.erase(std::remove_if(tracked_address_str.begin(), tracked_address_str.end(), ::isspace), tracked_address_str.end());
                std::stringstream tracked_address_stream(tracked_address_str);

                while (std::getline(tracked_address_stream, tracked_address_token, ','))
                {
                    if (tracked_address_token.empty())
                        continue;

                    try
                    {
                        filtered_dcps.insert(std::stoul(tracked_address_token, nullptr, 16));
                    }
                    catch (std::exception &e)
                    {
                        logger->warn("Invalid Tracked Address Parameter! Will not filter DCP data. Error: %s", e.what());
                        filtered_dcps.clear();
                        break;
                    }
                }
            }
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

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);
            if (write_images && !std::filesystem::exists(directory + "/IMAGES/Unknown"))
                std::filesystem::create_directories(directory + "/IMAGES/Unknown");
            if (parse_dcs && !std::filesystem::exists(directory + "/DCS"))
                std::filesystem::create_directory(directory + "/DCS");
            if (write_dcs && !std::filesystem::exists(directory + "/DCS_LRIT"))
                std::filesystem::create_directory(directory + "/DCS_LRIT");

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            initDCS();

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
                [this](::lrit::LRITFile &file, ccsds::CCSDSPacket &pkt, bool bad_crc) -> bool
            {
                if (file.custom_flags[RICE_COMPRESSED])
                {
                    if (fill_missing && bad_crc)
                        return false;

                    SZ_com_t &rice_parameters = rice_parameters_all[file.filename];

                    if (rice_parameters.bits_per_pixel == 0)
                        return false;

                    std::vector<uint8_t> decompression_buffer(rice_parameters.pixels_per_scanline);

                    size_t output_size = decompression_buffer.size();
                    int r = SZ_BufftoBuffDecompress(decompression_buffer.data(), &output_size, pkt.payload.data(), pkt.payload.size() - 2, &rice_parameters);
                    if (r == AEC_OK)
                    {
                        // Check to see if there are missing lines
                        uint16_t diff;
                        if (file.last_tracked_counter < pkt.header.packet_sequence_count)
                            diff = pkt.header.packet_sequence_count - file.last_tracked_counter;
                        else
                            diff = 16384 - file.last_tracked_counter + pkt.header.packet_sequence_count;

                        // There are missing lines
                        if (diff > 1)
                        {
                            ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();
                            size_t to_fill = rice_parameters.pixels_per_scanline * (diff - 1);
                            size_t max_fill = image_structure_record.columns_count * image_structure_record.lines_count + file.total_header_length -
                                (file.lrit_data.size() + output_size);

                            // Repeat next line if the user selected the fill missing option and it is below the threshold; otherwise fill with black
                            if (to_fill <= max_fill)
                            {
                                if (fill_missing && diff <= max_fill_lines)
                                {
                                    for(uint16_t i = 0; i < diff - 1; i++)
                                        file.lrit_data.insert(file.lrit_data.end(), &decompression_buffer.data()[0], &decompression_buffer.data()[output_size]);
                                }
                                else
                                    file.lrit_data.insert(file.lrit_data.end(), to_fill, 0);
                            }
                        }

                        file.lrit_data.insert(file.lrit_data.end(), &decompression_buffer.data()[0], &decompression_buffer.data()[output_size]);
                        file.last_tracked_counter = pkt.header.packet_sequence_count;
                    }
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
                [this](::lrit::LRITFile &file) -> void
            {
                // On image data, make sure buffer contains the right amount of data
                if (file.hasHeader<::lrit::ImageStructureRecord>() && file.hasHeader<::lrit::PrimaryHeader>() && file.hasHeader<NOAALRITHeader>())
                {
                    ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();
                    ::lrit::ImageStructureRecord image_header = file.getHeader<::lrit::ImageStructureRecord>();
                    NOAALRITHeader noaa_header = file.getHeader<NOAALRITHeader>();

                    if (primary_header.file_type_code == 0 &&
                        (image_header.compression_flag == 0 || (image_header.compression_flag == 1 && noaa_header.noaa_specific_compression == 1)))
                    {
                        if (fill_missing && file.lrit_data.size() > primary_header.total_header_length + image_header.columns_count)
                        {
                            // Fill remaining data with last line
                            uint32_t to_fill = (image_header.lines_count * image_header.columns_count -
                                (file.lrit_data.size() - file.total_header_length)) / image_header.columns_count;
                            if (to_fill > 0 && to_fill <= (uint32_t)max_fill_lines)
                            {
                                file.lrit_data.reserve(file.lrit_data.size() + to_fill * image_header.columns_count);
                                for (uint32_t i = 0; i < to_fill; i++)
                                    file.lrit_data.insert(file.lrit_data.end(), file.lrit_data.end() - image_header.columns_count, file.lrit_data.end());
                            }
                        }

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
                    if (write_lrit)
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
                if (segmentedDecoder.second.image_id != -1)
                    saveImageP(segmentedDecoder.second.meta, *segmentedDecoder.second.image);
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
                        int vcid = decMap.first > 63 ? 60 : decMap.first; // Himawari images use VCID > 63 internally

                        if (ImGui::BeginTabItem(std::string("VCID " + std::to_string(vcid)).c_str()))
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

            if (streamingInput && parse_dcs)
                drawDCSUI();
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
