#include <sstream>
#include <fstream>
#include "dcs_decoder.h"
#include "common/utils.h"
#include "common/lrit/crc_table.h"
#include "nlohmann/json_utils.h"
#include "logger.h"
#include "resources.h"
#include "init.h"

#include "../module_goes_lrit_data_decoder.h"

namespace goes
{
    namespace hrit
    {
        inline std::string bcdDateToString(uint8_t *data)
        {
            struct tm timestruct;
            timestruct.tm_year = ((data[6] & 0xf0) >> 4) * 10 + (data[6] & 0x0f) + 100;
            timestruct.tm_mon = 0;
            timestruct.tm_mday = ((data[5] & 0xf0) >> 4) * 100 + (data[5] & 0x0f) * 10 + ((data[4] & 0xf0) >> 4);
            timestruct.tm_hour = (data[4] & 0x0f) * 10 + ((data[3] & 0xf0) >> 4);
            timestruct.tm_min = (data[3] & 0x0f) * 10 + ((data[2] & 0xf0) >> 4);
            timestruct.tm_sec = (data[2] & 0x0f) * 10 + ((data[1] & 0xf0) >> 4);
            int ms = (data[1] & 0x0f) * 100 + ((data[0] & 0xf0) >> 4) * 10 + (data[0] & 0x0f);

            char return_buffer[20];
            mktime(&timestruct);
            strftime(return_buffer, 20, "%Y-%m-%d %H:%M:%S", &timestruct);
            return std::string(return_buffer) + "." + std::to_string(ms);
        }

        void GOESLRITDataDecoderModule::initDCS()
        {
            if (!write_dcs)
                return;

            shef_codes = loadJsonFile(resources::getResourcePath("dcs/shef_codes.json"));

            {
                std::string file_data;
                logger->info("Updating DCP list for DCS processing. Please wait...");
                if (!perform_http_request("https://hads.ncep.noaa.gov/compressed_defs/all_dcp_defs.txt", file_data))
                {
                    std::ofstream save_hads(satdump::user_path + "/all_dcp_defs.txt");
                    save_hads << file_data;
                    save_hads.close();
                }
                else
                    logger->warn("Unable to downloadload DCP list!");
            }

            if (std::filesystem::exists(satdump::user_path + "/all_dcp_defs.txt"))
            {
                logger->info("Loading DCP list...");
                std::ifstream read_hads(satdump::user_path + "/all_dcp_defs.txt");
                std::string hads_line;
                while (std::getline(read_hads, hads_line))
                {
                    try
                    {
                        std::stringstream hads_stream(hads_line);
                        std::string tmp;
                        std::vector<std::string> hads_tokens;
                        while (std::getline(hads_stream, tmp, '|'))
                        {
                            size_t first_letter = std::string::npos, last_letter = std::string::npos;
                            for (size_t i = 0; i < tmp.size(); i++)
                            {
                                if (first_letter == std::string::npos)
                                {
                                    if (tmp[i] == ' ')
                                        continue;
                                    first_letter = i;
                                }
                                if (tmp[i] != ' ')
                                    last_letter = i;
                            }
                            if (first_letter == std::string::npos)
                                hads_tokens.push_back("");
                            else
                                hads_tokens.push_back(tmp.substr(first_letter, last_letter - first_letter + 1));
                        }

                        std::shared_ptr<DCP> new_dcp = std::make_shared<DCP>();
                        new_dcp->nwsli = hads_tokens[1];
                        new_dcp->dcp_owner = hads_tokens[2];
                        new_dcp->state_location = hads_tokens[3];
                        new_dcp->hydrologic_service_area = hads_tokens[4];

                        // Latitude
                        std::stringstream degree_stream;
                        std::string degree_str;
                        degree_stream = std::stringstream(hads_tokens[5]);
                        std::getline(degree_stream, degree_str, ' ');
                        new_dcp->lat = std::stof(degree_str);
                        std::getline(degree_stream, degree_str, ' ');
                        new_dcp->lat += std::stof(degree_str) / 60.0f;
                        std::getline(degree_stream, degree_str, ' ');
                        new_dcp->lat += std::stof(degree_str) / 3600.0f;

                        // Longitude
                        degree_stream = std::stringstream(hads_tokens[6]);
                        std::getline(degree_stream, degree_str, ' ');
                        new_dcp->lon = std::stof(degree_str);
                        std::getline(degree_stream, degree_str, ' ');
                        new_dcp->lon += std::stof(degree_str) / 60.0f;
                        std::getline(degree_stream, degree_str, ' ');
                        new_dcp->lon += std::stof(degree_str) / 3600.0f;

                        new_dcp->initial_transmission_time = hads_tokens[7];
                        new_dcp->transmission_interval = std::stoi(hads_tokens[8]);
                        new_dcp->location_name = hads_tokens[9];

                        if (hads_tokens[10] == "S")
                            new_dcp->decoding_mode = "Self-Timed";
                        else if (hads_tokens[10] == "R")
                            new_dcp->decoding_mode = "Random";
                        else if (hads_tokens[10] == "B")
                            new_dcp->decoding_mode = "Both";
                        else
                            new_dcp->decoding_mode = "Unknown - " + hads_tokens[10];

                        for (size_t i = 11; i < hads_tokens.size() - 5; i += 5)
                        {
                            if (hads_tokens[i] == "")
                                break;

                            PEInfo new_info;
                            new_info.record_interval = std::stoi(hads_tokens[i + 1]);
                            new_info.read_to_transmit_offset = std::stoi(hads_tokens[i + 2]);
                            new_info.base_elevation = std::stoi(hads_tokens[i + 3]);
                            new_info.correction = std::stoi(hads_tokens[i + 4]);
                            
                            if (shef_codes.count(hads_tokens[i]) > 0)
                                new_info.name = shef_codes[hads_tokens[i]];
                            else
                                new_info.name = hads_tokens[i];

                            new_dcp->pe_info.emplace_back(new_info);
                        }

                        dcp_list[std::stoul(hads_tokens[0], nullptr, 16)] = new_dcp;
                    }
                    catch (std::exception &)
                    {
                        logger->warn("Error loading DCP list! Some DCS data will not be parsed");
                        dcp_list.clear();
                        read_hads.close();
                        break;
                    }
                }
                read_hads.close();
            }
            else
                logger->warn("Unable to load DCP list! Some DCS data will not be parsed");
        }

        bool GOESLRITDataDecoderModule::processDCS(uint8_t *data, size_t size)
        {
            DCSFile dcs_file;

            // DCS Header
            dcs_file.name = std::string(&data[0], &data[32]);
            int file_size = std::stoi(std::string(&data[32], &data[40]));
            dcs_file.source = std::string(&data[40], &data[44]);
            dcs_file.type = std::string(&data[44], &data[48]);
            // dcs_file.exp_field = std::string(&data[48], &data[60]);
            dcs_file.header_crc_pass =
                (uint32_t)(data[63] << 24 | data[62] << 16 | data[61] << 8 | data[60]) == dcs_crc32.compute(data, 60);

            if (size != file_size)
            {
                logger->warn("Invalid DCS File size! Not parsing");
                return false;
            }

            dcs_file.file_crc_pass =
                (uint32_t)(data[size - 1] << 24 | data[size - 2] << 16 | data[size - 3] << 8 | data[size - 4]) == dcs_crc32.compute(data, size - 4);

            // Trim whitepace
            dcs_file.name.erase(dcs_file.name.find_last_not_of(" ") + 1);
            dcs_file.source.erase(dcs_file.source.find_last_not_of(" ") + 1);
            dcs_file.type.erase(dcs_file.type.find_last_not_of(" ") + 1);
            // dcs_file.exp_field.erase(dcs_file.exp_field.find_last_not_of(" ") + 1);

            // Loop through all blocks
            uint8_t* block_ptr = &data[64];
            while (block_ptr < &data[size - 4])
            {
                // Variables frequently re-used later
                uint16_t block_crc = 0xffff;
                std::stringstream hex_stream;

                // Get Block ID and size
                uint8_t block_id = block_ptr[0];
                uint16_t block_length = block_ptr[2] << 8 | block_ptr[1];

                // Sanity checks
                if (&block_ptr[block_length] > &data[size - 4])
                {
                    logger->warn("Invalid DCS block length! Skipping remainder of file");
                    return false;
                }
                if (block_length == 5)
                {
                    // Empty file, ignore
                    block_ptr += 5;
                    continue;
                }

                // Calculate Block CRC
                for (uint16_t i = 0; i < block_length - 2; i++)
                    block_crc = (block_crc << 8) ^ lrit::crc_table[(block_crc >> 8) ^ (uint16_t)block_ptr[i]];
                bool message_crc_pass = (uint16_t)(block_ptr[block_length - 1] << 8 | block_ptr[block_length - 2]) == block_crc;

                // DCP Message Processing
                if (block_id == 1)
                {
                    DCSMessage dcs_message;
                    dcs_message.header.crc_pass = message_crc_pass;

                    // DCP Sequence Number
                    dcs_message.header.sequence_number = block_ptr[5] << 16 | block_ptr[4] << 8 | block_ptr[3];

                    // Message Flags: Data rate (Unknown, 100, 300, or 1200)
                    if ((block_ptr[6] & 0b111) == 0b001)
                        dcs_message.header.data_rate = "100";
                    else if ((block_ptr[6] & 0b111) == 0b010)
                        dcs_message.header.data_rate = "300";
                    else if ((block_ptr[6] & 0b111) == 0b011)
                        dcs_message.header.data_rate = "1200";
                    else if ((block_ptr[6] & 0b111) == 0b000)
                        dcs_message.header.data_rate = "Undefined";
                    else
                        dcs_message.header.data_rate = "Reserved";

                    // Other Message Flags
                    dcs_message.header.cs_platform = (block_ptr[6] & 0b1000) == 0b1000 ? "CS2" : "CS1";
                    dcs_message.header.parity_errors = (block_ptr[6] & 0b10000) == 0b10000;
                    dcs_message.header.no_eot = (block_ptr[6] & 0b100000) == 0b100000;
                    //Last 2 bits of block_ptr[6] are reserved (should be 0)

                    // Abnormal Received Message Flags
                    dcs_message.header.address_corrected = (block_ptr[7] & 0b1) == 0b1;
                    dcs_message.header.uncorrectable_address = (block_ptr[7] & 0b10) == 0b10;
                    dcs_message.header.invalid_address = (block_ptr[7] & 0b100) == 0b100;
                    dcs_message.header.pdt_incomplete = (block_ptr[7] & 0b1000) == 0b1000;
                    dcs_message.header.timing_error = (block_ptr[7] & 0b10000) == 0b10000;
                    dcs_message.header.unexpected_message = (block_ptr[7] & 0b100000) == 0b100000;
                    dcs_message.header.wrong_channel = (block_ptr[7] & 0b1000000) == 0b1000000;
                    //Last bit of block_ptr[6] is reserved (should be 0)

                    // Address
                    hex_stream << std::hex << std::setfill('0')
                        << std::setw(2) << +block_ptr[11]
                        << std::setw(2) << +block_ptr[10]
                        << std::setw(2) << +block_ptr[9]
                        << std::setw(2) << +block_ptr[8];

                    // Pull in DCP info
                    uint32_t address_int = (uint32_t)block_ptr[11] << 24 | (uint32_t)block_ptr[10] << 16 |
                        (uint32_t)block_ptr[9] << 8 | (uint32_t)block_ptr[8];
                    if (dcp_list.count(address_int) > 0)
                        dcs_message.dcp = dcp_list[address_int];

                    dcs_message.header.corrected_address = hex_stream.str();
                    hex_stream.clear();

                    //Start time, end time, and signal strength
                    dcs_message.header.carrier_start = bcdDateToString(&block_ptr[12]);
                    dcs_message.header.message_end = bcdDateToString(&block_ptr[19]);
                    dcs_message.header.signal_strength = ((block_ptr[27] << 8 | block_ptr[26]) & 0x03ff) / 10.0f; // First 6 bits reserved

                    // Frequency offset. First 2 bits reserved; signed!
                    uint16_t masked_freq_offset_10x = (block_ptr[29] << 8 | block_ptr[28]) & 0x3fff;
                    dcs_message.header.freq_offset = (float)((int)((masked_freq_offset_10x ^ 0x2000) - 0x2000)) / 10.0f;

                    // Phase noise (last 12 bits) and modulation index (first 2 bits). Bits 3-4 reserved
                    dcs_message.header.phase_noise = ((block_ptr[31] << 8 | block_ptr[30]) & 0x0fff) / 100.0f;
                    if ((block_ptr[31] >> 6) == 0b00)
                        dcs_message.header.modulation_index = "Unknown";
                    else if ((block_ptr[31] >> 6) == 0b01)
                        dcs_message.header.modulation_index = "Normal";
                    else if ((block_ptr[31] >> 6) == 0b10)
                        dcs_message.header.modulation_index = "High";
                    else if ((block_ptr[31] >> 6) == 0b11)
                        dcs_message.header.modulation_index = "Low";

                    // Good phase
                    dcs_message.header.good_phase = block_ptr[32] / 2.0f;

                    // Spacecraft ID (first 4 bits) and Channel (last 10 bits). Bits 5-6 reserved
                    dcs_message.header.channel = (block_ptr[34] << 8 | block_ptr[33]) & 0x03ff;
                    if ((block_ptr[34] >> 4) == 0b0000)
                        dcs_message.header.spacecraft = "Unknown";
                    else if ((block_ptr[34] >> 4) == 0b0001)
                        dcs_message.header.spacecraft = "GOES-East";
                    else if ((block_ptr[34] >> 4) == 0b0010)
                        dcs_message.header.spacecraft = "GOES-West";
                    else if ((block_ptr[34] >> 4) == 0b0011)
                        dcs_message.header.spacecraft = "GOES-Central";
                    else if ((block_ptr[34] >> 4) == 0b0100)
                        dcs_message.header.spacecraft = "GOES-Test";
                    else
                        dcs_message.header.spacecraft = "Reserved";

                    // Source ID
                    std::string source_id = std::string(&block_ptr[35], &block_ptr[37]);
                    if (source_id == "UP")
                        dcs_message.header.drgs_source = "NOAA WCDA E/W Prime - Wallops Island, VA";
                    else if (source_id == "UB")
                        dcs_message.header.drgs_source = "NOAA WCDA E/W Backup - Wallops Island, VA";
                    else if (source_id == "NP")
                        dcs_message.header.drgs_source = "NOAA NSOF E/W Prime - Suitland, MD";
                    else if (source_id == "NB")
                        dcs_message.header.drgs_source = "NOAA NSOF E/W Backup - Suitland, MD";
                    else if (source_id == "XE")
                        dcs_message.header.drgs_source = "USGS EDDN East - EROS, Sioux Falls, SD";
                    else if (source_id == "XW")
                        dcs_message.header.drgs_source = "USGS EDDN West - EROS, Sioux Falls, SD";
                    else if (source_id == "RE")
                        dcs_message.header.drgs_source = "USACE MVR East - Rock Island, IL";
                    else if (source_id == "RW")
                        dcs_message.header.drgs_source = "USACE MVR West - Rock Island, IL";
                    else if (source_id == "d1")
                        dcs_message.header.drgs_source = "NIFC Unit 1 West - Boise, ID";
                    else if (source_id == "d2")
                        dcs_message.header.drgs_source = "NIFC Unit 2 West - Boise, ID";
                    else if (source_id == "LE")
                        dcs_message.header.drgs_source = "USACE LRD East - Cincinnati, OH";
                    else if (source_id == "SF")
                        dcs_message.header.drgs_source = "SFWMD East - Palm Beach, FL";
                    else if (source_id == "OW")
                        dcs_message.header.drgs_source = "USACE NOW - Omaha, NE";
                    else
                        dcs_message.header.drgs_source = "Unknown - " + source_id;

                    //std::string source_secondary = std::string(&block_ptr[37], &block_ptr[39]);

                    dcs_message.header.type_id = block_ptr[39] & 0x3f;

                    // Initial body parsing
                    bool found_beginning = false;
                    hex_stream << std::hex << std::setfill('0') << std::setw(2) << +block_ptr[39];
                    for (size_t i = 40; i < block_length - 2; i ++)
                    {
                        // Save raw data as hex
                        hex_stream << std::setw(2) << +block_ptr[i];

                        // Modified ascii to ascii
                        if (!found_beginning && block_ptr[i] == 0x20)
                            continue;
                        found_beginning = true;
                        dcs_message.data_ascii += block_ptr[i] & 0x7F;
                    }

                    dcs_message.data_raw = hex_stream.str();
                    hex_stream.clear();

                    // Parse known message types
                    if (dcs_message.data_ascii.empty())
                    {
                        // Do nothing
                    }

                    // PRS Messages - modified SHEF
                    else if (dcs_message.data_ascii.substr(0, 4) == ":PRS" && dcs_message.data_ascii.size() > 3)
                    {
                        // Ignore for now...
                    }

                    // SHEF
                    else if (dcs_message.data_ascii[0] == ':' && dcs_message.data_ascii.size() > 1)
                    {
                        std::stringstream message_stream(dcs_message.data_ascii);
                        std::string instrument_string;
                        while (std::getline(message_stream, instrument_string, ':'))
                        {
                            if (instrument_string.empty())
                                continue;

                            std::stringstream value_stream(instrument_string);
                            std::vector<std::string> value_strings;
                            std::string tmp_value;
                            while (std::getline(value_stream, tmp_value, ' '))
                            {
                                if (tmp_value.empty())
                                    continue;

                                bool is_number = true;
                                if (value_strings.size() == 1 && value_stream.peek() != EOF)
                                {
                                    for (size_t i = 0; i < tmp_value.size(); i++)
                                    {
                                        if (!((tmp_value[i] >= '0' && tmp_value[i] <= '9') ||
                                            (tmp_value.size() > 1 && (tmp_value[i] == '-' || tmp_value[i] == '.'))))
                                        {
                                            is_number = false;
                                            break;
                                        }
                                    }
                                }

                                if (is_number)
                                    value_strings.push_back(tmp_value);
                                else
                                    value_strings[0] += " " + tmp_value;
                            }

                            if (value_strings.size() == 0)
                                continue;

                            DCSValue new_value;
                            std::string sensor_label = value_strings[0];
                            std::string sensor_suffix;
                            if (sensor_label.size() == 3 && sensor_label[0] >= 'A' && sensor_label[0] <= 'Z' &&
                                sensor_label[1] >= 'A' && sensor_label[1] <= 'Z' &&
                                sensor_label[2] >= '0' && sensor_label[2] <= '9')
                            {
                                sensor_suffix = " #";
                                sensor_suffix.push_back(sensor_label[2]);
                                sensor_label = sensor_label.substr(0, sensor_label.size() - 1);
                            }

                            if (shef_codes.count(sensor_label) > 0)
                                new_value.name = shef_codes[sensor_label] + sensor_suffix;
                            else
                                new_value.name = value_strings[0];

                            // Header with no body
                            if (value_strings.size() < 2)
                                continue;

                            // Simple header/value
                            else if (value_strings.size() == 2)
                                new_value.values.push_back(value_strings[1]);

                            // Standard value
                            else
                            {
                                int val_offset = 2;
                                try
                                {
                                    new_value.reading_age = std::stoi(value_strings[1]);
                                    if (value_strings[2][0] == '#' && value_strings[2].size() > 1)
                                    {
                                        val_offset++;
                                        new_value.interval = std::stoi(value_strings[2].substr(1, value_strings[2].size() - 1));
                                    }
                                }
                                catch (std::exception &)
                                {
                                    continue;
                                }

                                while (val_offset < value_strings.size())
                                    new_value.values.push_back(value_strings[val_offset++]);
                            }

                            dcs_message.data_values.emplace_back(new_value);
                        }

                        if (dcs_message.data_values.size() > 0)
                            dcs_message.data_type = "SHEF";
                    }

                    // Pseudobinary B
                    else if (dcs_message.data_ascii[0] == 'B' && (dcs_message.data_ascii[1] == '1' ||
                        dcs_message.data_ascii[1] == '2' || dcs_message.data_ascii[1] == '3' ||
                        dcs_message.data_ascii[1] == '4' || dcs_message.data_ascii[1] == 'C')
                        && dcs_message.data_ascii.size() > 4)
                    {
                        // Pseudobinary B and D can only be decoded if we know what data to expect.
                        // 
                        // - Type D is not used on platforms listed in HADS as of November 2024,
                        //   so no point in decoding it here.
                        // - BS* seems to be similar to BC*, but the values are interleaved. We'd
                        //   need to know how many of each to expect, which we don't. So, no
                        //   decoding for BS* Pseudobinary files unfortunately!

                        if (dcs_message.dcp != nullptr && dcs_message.dcp->pe_info.size() > 0)
                        {
                            size_t num_physical_elements = dcs_message.dcp->pe_info.size();
                            if (dcs_message.data_ascii[1] == 'C' && dcs_message.dcp->pe_info.back().name == "Voltage - battery (volt)")
                                num_physical_elements--;

                            int data_bytes = dcs_message.data_ascii.size() - 4;
                            size_t data_end = dcs_message.data_ascii.find_first_of(' ');
                            if (data_end != std::string::npos)
                                data_bytes -= dcs_message.data_ascii.size() - data_end;

                            if (data_bytes % (num_physical_elements * 3) == 0)
                            {
                                dcs_message.data_type = "Pseudobinary B";
                                int sensor_offset = 3;

                                std::string pseudo_string = dcs_message.data_ascii;
                                std::replace(pseudo_string.begin(), pseudo_string.end(), '?', (char)0x7f); //Del

                                for (int pe_count = 0; pe_count < num_physical_elements; pe_count++)
                                {
                                    DCSValue new_value;
                                    new_value.reading_age = pseudo_string[2] - 0x40;
                                    new_value.name = dcs_message.dcp->pe_info[pe_count].name;
                                    new_value.interval = dcs_message.dcp->pe_info[pe_count].record_interval;
                                    for (size_t i = 0; i < data_bytes / num_physical_elements / 3; i++)
                                    {
                                        // No reading
                                        if (pseudo_string.substr(sensor_offset + (3 * i), 3) == "///")
                                        {
                                            new_value.values.emplace_back("0");
                                            continue;
                                        }

                                        uint32_t val_int = (uint32_t)(pseudo_string[sensor_offset + (3 * i) + 2] - 0x40) |
                                            (uint32_t)((pseudo_string[sensor_offset + (3 * i) + 1] - 0x40) << 6) |
                                            (uint32_t)((pseudo_string[sensor_offset + (3 * i)] - 0x40) << 12);

                                        new_value.values.emplace_back(std::to_string((float)((int)((val_int ^ 0x20000) - 0x20000)) / 100.0f));
                                    }

                                    sensor_offset += data_bytes / num_physical_elements;
                                    dcs_message.data_values.emplace_back(new_value);
                                }

                                // Get Battery Value
                                if (dcs_message.data_ascii[1] != 'C')
                                {
                                    DCSValue battery_value;
                                    battery_value.reading_age = dcs_message.data_ascii[2] - 0x40;
                                    battery_value.name = "Voltage - battery (volt)";
                                    battery_value.values.emplace_back(std::to_string((float)(dcs_message.data_ascii[sensor_offset] - 0x40) * 0.234 + 10.6));
                                    dcs_message.data_values.emplace_back(battery_value);
                                }

                                // BC Battery value - reverse engineered equation. Seems good with wide range of sample values
                                else if (dcs_message.dcp->pe_info.back().name == "Voltage - battery (volt)")
                                {
                                    DCSValue battery_value;
                                    battery_value.reading_age = dcs_message.data_ascii[2] - 0x40;
                                    battery_value.name = "Voltage - battery (volt)";
                                    battery_value.values.emplace_back(std::to_string((float)(dcs_message.data_ascii[sensor_offset] - 0x40) * 0.313 + 0.285));
                                    dcs_message.data_values.emplace_back(battery_value);
                                }
                            }
                        }
                    }

                    // Pseudobinary C
                    // Rarely used, but only pseudobinary format that
                    // can be accurately decoded without DCP info
                    else if (dcs_message.data_ascii[0] == 'C' && (dcs_message.data_ascii[1] == '1' ||
                        dcs_message.data_ascii[1] == '2' || dcs_message.data_ascii[1] == '3' ||
                        dcs_message.data_ascii[1] == '4') && dcs_message.data_ascii[2] == '+' && dcs_message.data_ascii.size() > 13)
                    {
                        size_t end_of_data = dcs_message.data_ascii.find_last_of('.');
                        if (end_of_data != std::string::npos)
                        {
                            dcs_message.data_type = "Pseudobinary C";

                            std::string pseudo_string = dcs_message.data_ascii.substr(3, end_of_data - 3);
                            std::replace(pseudo_string.begin(), pseudo_string.end(), '?', (char)0x7f); //Del

                            std::stringstream message_stream(pseudo_string);
                            std::string instrument_string;
                            for (int measurement_index = 1; std::getline(message_stream, instrument_string, '+'); measurement_index++)
                            {
                                DCSValue new_value;
                                new_value.name = "Sensor " + std::to_string((int)(instrument_string[0] - 0x40));

                                // Skipping Day of year

                                // Minute of day to compute reading age
                                uint16_t recorded_minute = (((uint16_t)instrument_string[3] - 0x40) << 6) | (instrument_string[4] - 0x40);
                                uint16_t transmission_minute = ((block_ptr[16] & 0x0f) * 10 + ((block_ptr[15] & 0xf0) >> 4)) * 60 +
                                    (block_ptr[15] & 0x0f) * 10 + ((block_ptr[14] & 0xf0) >> 4);
                                new_value.reading_age = transmission_minute - recorded_minute;

                                // Interval
                                new_value.interval = (((uint16_t)instrument_string[5] - 0x40) << 6) | (instrument_string[6] - 0x40);

                                // Reading values
                                for (int i = 7; i + 2 < instrument_string.size(); i += 3)
                                {
                                    // No reading
                                    if (instrument_string.substr(i, 3) == "///")
                                    {
                                        new_value.values.emplace_back("0");
                                        continue;
                                    }

                                    uint32_t val_int = (uint32_t)(instrument_string[i + 2] - 0x40) |
                                        (uint32_t)((instrument_string[i + 1] - 0x40) << 6) |
                                        (uint32_t)((instrument_string[i] - 0x40) << 12);

                                    new_value.values.emplace_back(std::to_string((float)((int)((val_int ^ 0x20000) - 0x20000)) / 100.0f));
                                }

                                dcs_message.data_values.emplace_back(new_value);
                            }

                            if (dcs_message.dcp != nullptr)
                            {
                                int i = 0;
                                for (PEInfo &pe_info : dcs_message.dcp->pe_info)
                                {
                                    if (i >= dcs_message.data_values.size())
                                        break;
                                    dcs_message.data_values[i].name = pe_info.name;
                                    i++;
                                }
                            }

                            // Get Battery Value
                            if (end_of_data < dcs_message.data_ascii.size() - 1)
                            {
                                DCSValue battery_value;
                                battery_value.name = "Voltage - battery (volt)";
                                battery_value.values.emplace_back(std::to_string((float)(dcs_message.data_ascii[end_of_data + 1] - 0x40) * 0.234 + 10.6));
                                dcs_message.data_values.emplace_back(battery_value);
                            }
                        }
                    }

                    // Pseudobinary 2 (B Random)
                    else if (dcs_message.data_ascii[0] == '2' && dcs_message.data_ascii.size() > 7 && (dcs_message.data_ascii.size() - 5) % 3 == 0)
                    {
                        int data_bytes = dcs_message.data_ascii.size() - 5;
                        if (dcs_message.dcp != nullptr && dcs_message.dcp->pe_info.size() > 0 && data_bytes / dcs_message.dcp->pe_info.size() / 3 > 0 && 
                            data_bytes % (dcs_message.dcp->pe_info.size() * 3) == 0)
                        {
                            dcs_message.data_type = "Pseudobinary 2";
                            int sensor_offset = 2;

                            std::string pseudo_string = dcs_message.data_ascii;
                            std::replace(pseudo_string.begin(), pseudo_string.end(), '?', (char)0x7f); //Del

                            for (int pe_count = 0; pe_count < dcs_message.dcp->pe_info.size(); pe_count++)
                            {
                                DCSValue new_value;
                                new_value.reading_age = pseudo_string[1] - 0x40;
                                new_value.name = dcs_message.dcp->pe_info[pe_count].name;
                                new_value.interval = dcs_message.dcp->pe_info[pe_count].record_interval;
                                for (size_t i = 0; i < data_bytes / dcs_message.dcp->pe_info.size() / 3; i++)
                                {
                                    // No reading
                                    if (pseudo_string.substr(sensor_offset + (3 * i), 3) == "///")
                                    {
                                        new_value.values.emplace_back("0");
                                        continue;
                                    }

                                    uint32_t val_int = (uint32_t)(pseudo_string[sensor_offset + (3 * i) + 2] - 0x40) |
                                        (uint32_t)((pseudo_string[sensor_offset + (3 * i) + 1] - 0x40) << 6) |
                                        (uint32_t)((pseudo_string[sensor_offset + (3 * i)] - 0x40) << 12);

                                    new_value.values.emplace_back(std::to_string((float)((int)((val_int ^ 0x20000) - 0x20000)) / 100.0f));
                                }

                                sensor_offset += data_bytes / dcs_message.dcp->pe_info.size();
                                dcs_message.data_values.emplace_back(new_value);
                            }

                            // Transmission count
                            {
                                DCSValue transmission_count;
                                transmission_count.name = "Transmissions since reboot";
                                transmission_count.values.emplace_back(std::to_string((uint16_t)((dcs_message.data_ascii[sensor_offset] - 0x40) << 6 |
                                    (dcs_message.data_ascii[sensor_offset + 1] - 0x40))));
                                dcs_message.data_values.emplace_back(transmission_count);
                            }

                            // Get Battery Value
                            {
                                DCSValue battery_value;
                                battery_value.name = "Voltage - battery (volt)";
                                battery_value.values.emplace_back(std::to_string((float)(dcs_message.data_ascii[sensor_offset + 2] - 0x40) * 0.234 + 10.6));
                                dcs_message.data_values.emplace_back(battery_value);
                            }
                        }
                    }

                    // ASCII Files
                    else if (dcs_message.data_ascii.substr(0, 2) == "\r\n")
                    {
                        dcs_message.data_type = "ASCII";

                        // Check for column/row data - requires dcp info!
                        if (dcs_message.dcp != nullptr && dcs_message.dcp->pe_info.size() > 0 && 
                            dcs_message.data_ascii.find_first_not_of("0123456789-. \r\n") == std::string::npos)
                        {
                            std::vector<std::vector<std::string>> column_matrix;
                            std::string filtered_ascii_data = dcs_message.data_ascii;
                            filtered_ascii_data.erase(std::remove(filtered_ascii_data.begin(), filtered_ascii_data.end(), '\r'), filtered_ascii_data.end());
                            std::stringstream ascii_linestream(filtered_ascii_data);
                            std::string ascii_line;

                            while (std::getline(ascii_linestream, ascii_line))
                            {
                                if (ascii_line.empty())
                                    continue;

                                column_matrix.emplace_back(std::vector<std::string>());
                                std::stringstream value_stream(ascii_line);
                                std::string value_string;

                                while (std::getline(value_stream, value_string, ' '))
                                {
                                    if (value_string.empty())
                                        continue;

                                    column_matrix.back().emplace_back(value_string);
                                }
                            }

                            // Values are each on their own line
                            if (column_matrix.size() == dcs_message.dcp->pe_info.size())
                            {
                                dcs_message.data_type = "ASCII Rows";

                                for (size_t i = 0; i < column_matrix.size(); i++)
                                {
                                    DCSValue new_value;
                                    new_value.name = dcs_message.dcp->pe_info[i].name;
                                    new_value.interval = dcs_message.dcp->transmission_interval;
                                    for (size_t j = 0; j < column_matrix[i].size(); j++)
                                        new_value.values.push_back(column_matrix[i][j]);

                                    dcs_message.data_values.emplace_back(new_value);
                                }
                            }

                            // Values are in their own column, with the occasional exception of battery
                            else if (column_matrix.size() > 0 && (column_matrix[0].size() == dcs_message.dcp->pe_info.size() ||
                                (column_matrix.back().size() == dcs_message.dcp->pe_info.size() &&
                                column_matrix[0].size() == column_matrix.back().size() - 1)))
                            {
                                dcs_message.data_type = "ASCII Columns";

                                for (size_t i = 0; i < column_matrix[0].size(); i++)
                                {
                                    DCSValue new_value;
                                    new_value.name = dcs_message.dcp->pe_info[i].name;
                                    new_value.interval = dcs_message.dcp->transmission_interval;
                                    for (size_t j = 0; j < column_matrix.size(); j++)
                                        if(i < column_matrix[j].size())
                                            new_value.values.push_back(column_matrix[j][i]);

                                    dcs_message.data_values.emplace_back(new_value);
                                }

                                if (column_matrix[0].size() == column_matrix.back().size() - 1)
                                {
                                    DCSValue trailing_value;
                                    trailing_value.name = dcs_message.dcp->pe_info.back().name;
                                    trailing_value.interval = dcs_message.dcp->transmission_interval;
                                    trailing_value.values.push_back(column_matrix.back().back());
                                    dcs_message.data_values.emplace_back(trailing_value);
                                }
                            }
                        }
                    }

                    dcs_file.blocks.emplace_back(dcs_message);
                }

                // Missed Message Processing
                else if (block_id == 2)
                {
                    MissedMessage missed_message;
                    missed_message.header.crc_pass = message_crc_pass;

                    // DCP Sequence Number
                    missed_message.header.sequence_number = block_ptr[5] << 16 | block_ptr[4] << 8 | block_ptr[3];

                    // Message Flags: Data rate (Unknown, 100, 300, or 1200)
                    // First 5 bits not used
                    if ((block_ptr[6] & 0b111) == 0b001)
                        missed_message.header.data_rate = "100";
                    else if ((block_ptr[6] & 0b111) == 0b010)
                        missed_message.header.data_rate = "300";
                    else if ((block_ptr[6] & 0b111) == 0b011)
                        missed_message.header.data_rate = "1200";
                    else if ((block_ptr[6] & 0b111) == 0b000)
                        missed_message.header.data_rate = "Undefined";
                    else
                        missed_message.header.data_rate = "Reserved";

                    // Address
                    hex_stream << std::hex << std::setfill('0')
                        << std::setw(2) << +block_ptr[10]
                        << std::setw(2) << +block_ptr[9]
                        << std::setw(2) << +block_ptr[8] 
                        << std::setw(2) << +block_ptr[7];

                    uint32_t address_int = (uint32_t)block_ptr[10] << 24 | (uint32_t)block_ptr[9] << 16 |
                        (uint32_t)block_ptr[8] << 8 | (uint32_t)block_ptr[7];
                    if (dcp_list.count(address_int) > 0)
                        missed_message.dcp = dcp_list[address_int];

                    missed_message.header.platform_address = hex_stream.str();
                    hex_stream.clear();

                    // Window start and end
                    missed_message.header.window_start = bcdDateToString(&block_ptr[11]);
                    missed_message.header.window_end = bcdDateToString(&block_ptr[18]);

                    // Spacecraft ID (first 4 bits) and Channel (last 10 bits). Bits 5-6 reserved
                    missed_message.header.channel = (block_ptr[26] << 8 | block_ptr[25]) & 0x03ff;
                    if ((block_ptr[26] >> 4) == 0b0000)
                        missed_message.header.spacecraft = "Unknown";
                    else if ((block_ptr[26] >> 4) == 0b0001)
                        missed_message.header.spacecraft = "GOES-East";
                    else if ((block_ptr[26] >> 4) == 0b0010)
                        missed_message.header.spacecraft = "GOES-West";
                    else if ((block_ptr[26] >> 4) == 0b0011)
                        missed_message.header.spacecraft = "GOES-Central";
                    else if ((block_ptr[26] >> 4) == 0b0100)
                        missed_message.header.spacecraft = "GOES-Test";
                    else
                        missed_message.header.spacecraft = "Reserved";

                    dcs_file.blocks.emplace_back(missed_message);
                }

                // Unknown block
                else
                    logger->warn("Unknown Block ID %hu, skipping", block_id);

                block_ptr += block_length;
            }

            // Write out data
            nlohmann::ordered_json export_json = dcs_file;
            std::ofstream json_writer(directory + "/DCS/" + dcs_file.name + ".json");
            json_writer << export_json.dump(4);
            json_writer.close();

            return true;
        }
    }
}