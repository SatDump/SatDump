#include "module_goes_lrit_data_decoder.h"
#include "core/plugin.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "lrit_header.h"
#include "xrit/processor/xrit_channel_processor_render.h"
#include "xrit/transport/xrit_demux.h"
#include <filesystem>
#include <fstream>

namespace goes
{
    namespace hrit
    {
        GOESLRITDataDecoderModule::GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), write_images(parameters["write_images"].get<bool>()),
              write_emwin(parameters["write_emwin"].get<bool>()), write_dcs(parameters["write_dcs"].get<bool>()), write_messages(parameters["write_messages"].get<bool>()),
              write_unknown(parameters["write_unknown"].get<bool>()), max_fill_lines(parameters["max_fill_lines"].get<int>())
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

            fsfsm_enable_output = false;
        }

        GOESLRITDataDecoderModule::~GOESLRITDataDecoderModule() {}

        void GOESLRITDataDecoderModule::process()
        {
            directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);
            if (write_images && !std::filesystem::exists(directory + "/IMAGES/Unknown"))
                std::filesystem::create_directories(directory + "/IMAGES/Unknown");
            if (parse_dcs && !std::filesystem::exists(directory + "/DCS"))
                std::filesystem::create_directory(directory + "/DCS");
            if (write_dcs && !std::filesystem::exists(directory + "/DCS_LRIT"))
                std::filesystem::create_directory(directory + "/DCS_LRIT");

            uint8_t cadu[1024];

            initDCS();

            logger->info("Demultiplexing and deframing...");

            satdump::xrit::XRITDemux lrit_demux;

            lrit_demux.onParseHeader = [this](satdump::xrit::XRITFile &file) -> void
            {
                file.custom_flags.insert({RICE_COMPRESSED, false});

                // Check if this is image data
                if (file.hasHeader<satdump::xrit::ImageStructureRecord>())
                {
                    satdump::xrit::ImageStructureRecord image_structure_record = file.getHeader<satdump::xrit::ImageStructureRecord>(); //(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
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

            lrit_demux.onProcessData = [this](satdump::xrit::XRITFile &file, ccsds::CCSDSPacket &pkt, bool bad_crc) -> bool
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
                            satdump::xrit::ImageStructureRecord image_structure_record = file.getHeader<satdump::xrit::ImageStructureRecord>();
                            size_t to_fill = rice_parameters.pixels_per_scanline * (diff - 1);
                            size_t max_fill = image_structure_record.columns_count * image_structure_record.lines_count + file.total_header_length - (file.lrit_data.size() + output_size);

                            // Repeat next line if the user selected the fill missing option and it is below the threshold; otherwise fill with black
                            if (to_fill <= max_fill)
                            {
                                if (fill_missing && diff <= max_fill_lines)
                                {
                                    for (uint16_t i = 0; i < diff - 1; i++)
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

            lrit_demux.onFinalizeData = [this](satdump::xrit::XRITFile &file) -> void
            {
                // On image data, make sure buffer contains the right amount of data
                if (file.hasHeader<satdump::xrit::ImageStructureRecord>() && file.hasHeader<satdump::xrit::PrimaryHeader>() && file.hasHeader<NOAALRITHeader>())
                {
                    satdump::xrit::PrimaryHeader primary_header = file.getHeader<satdump::xrit::PrimaryHeader>();
                    satdump::xrit::ImageStructureRecord image_header = file.getHeader<satdump::xrit::ImageStructureRecord>();
                    NOAALRITHeader noaa_header = file.getHeader<NOAALRITHeader>();

                    if (primary_header.file_type_code == 0 && (image_header.compression_flag == 0 || (image_header.compression_flag == 1 && noaa_header.noaa_specific_compression == 1)))
                    {
                        if (fill_missing && file.lrit_data.size() > primary_header.total_header_length + image_header.columns_count)
                        {
                            // Fill remaining data with last line
                            uint32_t to_fill = (image_header.lines_count * image_header.columns_count - (file.lrit_data.size() - file.total_header_length)) / image_header.columns_count;
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

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                std::vector<satdump::xrit::XRITFile> files = lrit_demux.work(cadu);

                for (auto &file : files)
                {
                    processLRITFile(file);
                    if (write_lrit)
                        saveLRITFile(file, directory + "/LRIT");
                }
            }

            cleanup();
            satdump::taskScheduler->del_task("goes_dcp_updater");

            for (auto &p : all_processors)
                p.second->flush();
        }

        void GOESLRITDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES HRIT Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            all_processors_mtx.lock();
            satdump::xrit::renderAllTabsFromProcessors(all_processors);
            all_processors_mtx.unlock();

            drawProgressBar();

            ImGui::End();

            if (d_is_streaming_input && parse_dcs)
                drawDCSUI();
        }

        std::string GOESLRITDataDecoderModule::getID() { return "goes_lrit_data_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GOESLRITDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESLRITDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace hrit
} // namespace goes
