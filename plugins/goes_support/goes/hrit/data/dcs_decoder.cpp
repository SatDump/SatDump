#include <sstream>
#include <fstream>
#include "dcs_decoder.h"
#include "../module_goes_lrit_data_decoder.h"
#include "common/lrit/crc_table.h"
#include "logger.h"

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

        bool GOESLRITDataDecoderModule::parseDCS(uint8_t *data, size_t size)
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
                std::stringstream address_stream;

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
                    dcs_message.crc_pass = message_crc_pass;

                    // DCP Sequence Number
                    dcs_message.sequence_number = block_ptr[5] << 16 | block_ptr[4] << 8 | block_ptr[3];

                    // Message Flags: Data rate (Unknown, 100, 300, or 1200)
                    if ((block_ptr[6] & 0b111) == 0b001)
                        dcs_message.data_rate = "100";
                    else if ((block_ptr[6] & 0b111) == 0b010)
                        dcs_message.data_rate = "300";
                    else if ((block_ptr[6] & 0b111) == 0b011)
                        dcs_message.data_rate = "1200";
                    else if ((block_ptr[6] & 0b111) == 0b000)
                        dcs_message.data_rate = "Undefined";
                    else
                        dcs_message.data_rate = "Reserved";

                    // Other Message Flags
                    dcs_message.cs_platform = (block_ptr[6] & 0b1000) == 0b1000 ? "CS2" : "CS1";
                    dcs_message.parity_errors = (block_ptr[6] & 0b10000) == 0b10000;
                    dcs_message.no_eot = (block_ptr[6] & 0b100000) == 0b100000;
                    //Last 2 bits of block_ptr[6] are reserved (should be 0)

                    // Abnormal Received Message Flags
                    dcs_message.address_corrected = (block_ptr[7] & 0b1) == 0b1;
                    dcs_message.uncorrectable_address = (block_ptr[7] & 0b10) == 0b10;
                    dcs_message.invalid_address = (block_ptr[7] & 0b100) == 0b100;
                    dcs_message.pdt_incomplete = (block_ptr[7] & 0b1000) == 0b1000;
                    dcs_message.timing_error = (block_ptr[7] & 0b10000) == 0b10000;
                    dcs_message.unexpected_message = (block_ptr[7] & 0b100000) == 0b100000;
                    dcs_message.wrong_channel = (block_ptr[7] & 0b1000000) == 0b1000000;
                    //Last bit of block_ptr[6] is reserved (should be 0)

                    // Address
                    address_stream << std::hex << std::setfill('0')
                        << std::setw(2) << +block_ptr[11]
                        << std::setw(2) << +block_ptr[10]
                        << std::setw(2) << +block_ptr[9]
                        << std::setw(2) << +block_ptr[8];

                    dcs_message.corrected_address = address_stream.str();
                    address_stream.clear();

                    //Start time, end time, and signal strength
                    dcs_message.carrier_start = bcdDateToString(&block_ptr[12]);
                    dcs_message.message_end = bcdDateToString(&block_ptr[19]);
                    dcs_message.signal_strength = ((block_ptr[27] << 8 | block_ptr[26]) & 0x03ff) / 10.0f; // First 6 bits reserved

                    // Frequency offset. First 2 bits reserved; signed!
                    int16_t masked_freq_offset_10x = (block_ptr[29] << 8 | block_ptr[28]) & 0x3fff;
                    if ((masked_freq_offset_10x & 0b10000000000000) == 0b10000000000000)
                        masked_freq_offset_10x |= 0b1100000000000000; //Sign Extending
                    dcs_message.freq_offset = masked_freq_offset_10x / 10.0f;

                    // Phase noise (last 12 bits) and modulation index (first 2 bits). Bits 3-4 reserved
                    dcs_message.phase_noise = ((block_ptr[31] << 8 | block_ptr[30]) & 0x0fff) / 100.0f;
                    if ((block_ptr[31] >> 6) == 0b00)
                        dcs_message.modulation_index = "Unknown";
                    else if ((block_ptr[31] >> 6) == 0b01)
                        dcs_message.modulation_index = "Normal";
                    else if ((block_ptr[31] >> 6) == 0b10)
                        dcs_message.modulation_index = "High";
                    else if ((block_ptr[31] >> 6) == 0b11)
                        dcs_message.modulation_index = "Low";

                    // Good phase
                    dcs_message.good_phase = block_ptr[32] / 2.0f;

                    // Spacecraft ID (first 4 bits) and Channel (last 10 bits). Bits 5-6 reserved
                    dcs_message.channel = (block_ptr[34] << 8 | block_ptr[33]) & 0x03ff;
                    if ((block_ptr[34] >> 4) == 0b0000)
                        dcs_message.spacecraft = "Unknown";
                    else if ((block_ptr[34] >> 4) == 0b0001)
                        dcs_message.spacecraft = "GOES-East";
                    else if ((block_ptr[34] >> 4) == 0b0010)
                        dcs_message.spacecraft = "GOES-West";
                    else if ((block_ptr[34] >> 4) == 0b0011)
                        dcs_message.spacecraft = "GOES-Central";
                    else if ((block_ptr[34] >> 4) == 0b0100)
                        dcs_message.spacecraft = "GOES-Test";
                    else
                        dcs_message.spacecraft = "Reserved";

                    // Source ID
                    std::string source_id = std::string(&block_ptr[35], &block_ptr[37]);
                    if (source_id == "UP")
                        dcs_message.drgs_source = "NOAA WCDA E/W Prime - Wallops Island, VA";
                    else if (source_id == "UB")
                        dcs_message.drgs_source = "NOAA WCDA E/W Backup - Wallops Island, VA";
                    else if (source_id == "NP")
                        dcs_message.drgs_source = "NOAA NSOF E/W Prime - Suitland, MD";
                    else if (source_id == "NB")
                        dcs_message.drgs_source = "NOAA NSOF E/W Backup - Suitland, MD";
                    else if (source_id == "XE")
                        dcs_message.drgs_source = "USGS EDDN East - EROS, Sioux Falls, SD";
                    else if (source_id == "XW")
                        dcs_message.drgs_source = "USGS EDDN West - EROS, Sioux Falls, SD";
                    else if (source_id == "RE")
                        dcs_message.drgs_source = "USACE MVR East - Rock Island, IL";
                    else if (source_id == "RW")
                        dcs_message.drgs_source = "USACE MVR West - Rock Island, IL";
                    else if (source_id == "d1")
                        dcs_message.drgs_source = "NIFC Unit 1 West - Boise, ID";
                    else if (source_id == "d2")
                        dcs_message.drgs_source = "NIFC Unit 2 West - Boise, ID";
                    else if (source_id == "LE")
                        dcs_message.drgs_source = "USACE LRD East - Cincinnati, OH";
                    else if (source_id == "SF")
                        dcs_message.drgs_source = "SFWMD East - Palm Beach, FL";
                    else if (source_id == "OW")
                        dcs_message.drgs_source = "USACE NOW - Omaha, NE";
                    else
                        dcs_message.drgs_source = "Unknown - " + source_id;

                    //std::string source_secondary = std::string(&block_ptr[37], &block_ptr[39]);

                    //TODO: Parse Body
                    dcs_file.blocks.emplace_back(dcs_message);
                }

                // Missed Message Processing
                else if (block_id == 2)
                {
                    MissedMessage missed_message;
                    missed_message.crc_pass = message_crc_pass;

                    // DCP Sequence Number
                    missed_message.sequence_number = block_ptr[5] << 16 | block_ptr[4] << 8 | block_ptr[3];

                    // Message Flags: Data rate (Unknown, 100, 300, or 1200)
                    // First 5 bits not used
                    if ((block_ptr[6] & 0b111) == 0b001)
                        missed_message.data_rate = "100";
                    else if ((block_ptr[6] & 0b111) == 0b010)
                        missed_message.data_rate = "300";
                    else if ((block_ptr[6] & 0b111) == 0b011)
                        missed_message.data_rate = "1200";
                    else if ((block_ptr[6] & 0b111) == 0b000)
                        missed_message.data_rate = "Undefined";
                    else
                        missed_message.data_rate = "Reserved";

                    // Address
                    address_stream << std::hex << std::setfill('0')
                        << std::setw(2) << +block_ptr[10]
                        << std::setw(2) << +block_ptr[9]
                        << std::setw(2) << +block_ptr[8] 
                        << std::setw(2) << +block_ptr[7];

                    missed_message.platform_address = address_stream.str();
                    address_stream.clear();

                    // Window start and end
                    missed_message.window_start = bcdDateToString(&block_ptr[11]);
                    missed_message.window_end = bcdDateToString(&block_ptr[18]);

                    // Spacecraft ID (first 4 bits) and Channel (last 10 bits). Bits 5-6 reserved
                    missed_message.channel = (block_ptr[26] << 8 | block_ptr[25]) & 0x03ff;
                    if ((block_ptr[26] >> 4) == 0b0000)
                        missed_message.spacecraft = "Unknown";
                    else if ((block_ptr[26] >> 4) == 0b0001)
                        missed_message.spacecraft = "GOES-East";
                    else if ((block_ptr[26] >> 4) == 0b0010)
                        missed_message.spacecraft = "GOES-West";
                    else if ((block_ptr[26] >> 4) == 0b0011)
                        missed_message.spacecraft = "GOES-Central";
                    else if ((block_ptr[26] >> 4) == 0b0100)
                        missed_message.spacecraft = "GOES-Test";
                    else
                        missed_message.spacecraft = "Reserved";

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