#include "pkt_parser.h"
#include "logger.h"

namespace inmarsat
{
    namespace stdc
    {
        void STDPacketParser::parse_pkt_27(uint8_t *pkt, nlohmann::json &output)
        {
            int mes_id = pkt[1] << 16 | pkt[2] << 8 | pkt[3];
            int sat = pkt[4] >> 6 & 0x03;
            int les_id = pkt[4] & 0x3F;
            int logical_channel_no = pkt[5];

            output["mes_id"] = mes_id;
            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = sat;
            output["les_name"] = get_les_name(sat, les_id);
            output["logical_channel_no"] = logical_channel_no;
        }

        void STDPacketParser::parse_pkt_2a(uint8_t *pkt, nlohmann::json &output)
        {
            parse_pkt_27(pkt, output);
            output["unknown1_hex"] = buf_to_hex_str(&pkt[6], 3);
        }

        void STDPacketParser::parse_pkt_08(uint8_t *pkt, nlohmann::json &output)
        {
            int sat = pkt[1] >> 6 & 0x03;
            int les_id = pkt[1] & 0x3F;
            int logical_channel_no = pkt[2];
            double uplink_mhz = ((pkt[3] << 8 | (uint16_t)pkt[4]) - 6000) * 0.0025 + 1626.5;

            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = sat;
            output["les_name"] = get_les_name(sat, les_id);
            output["logical_channel_no"] = logical_channel_no;
            output["uplink_channel_mhz"] = uplink_mhz;
        }

        void STDPacketParser::parse_pkt_6c(uint8_t *pkt, nlohmann::json &output)
        {
            int is_8 = pkt[1];
            double uplink_mhz = ((pkt[2] << 8 | (uint16_t)pkt[3]) - 6000) * 0.0025 + 1626.5;

            for (int i = 0, j = 0; i < 28; i += 4, j++)
            {
                output["tdm_slots"][i] = pkt[4 + j] >> 6;
                output["tdm_slots"][i + 1] = pkt[4 + j] >> 4 & 3;
                output["tdm_slots"][i + 2] = pkt[4 + j] >> 2 & 3;
                output["tdm_slots"][i + 3] = pkt[4 + j] & 3;
            }

            output["services"] = get_services_short(is_8);
            output["uplink_channel_mhz"] = uplink_mhz;
        }

        void STDPacketParser::parse_pkt_7d(uint8_t *pkt, nlohmann::json &output)
        {
            int network_version = pkt[1];
            int frame_number = pkt[2] << 8 | pkt[3];
            double timestamp_seconds_d = frame_number * 8.64;
            int timestamp_hours = floor(timestamp_seconds_d / 3600.0);
            int timestamp_min = floor((((int)timestamp_seconds_d) % 3600) / 60.0);
            int timestamp_sec = ((int)timestamp_seconds_d) % 60;
            int timestamp_msec = (((int)(timestamp_seconds_d * 1000)) % 1000);
            int signalling_channel = pkt[4] >> 2;
            int count = (pkt[5] >> 4 & 0x0F) * 0x02;
            uint8_t channel_type = pkt[6] >> 0x05;

            std::string channel_type_nName;
            if (channel_type == 1)
                channel_type_nName = "NCS";
            else if (channel_type == 2)
                channel_type_nName = "LES TDM";
            else if (channel_type == 3)
                channel_type_nName = "Joint NCS and TDM";
            else if (channel_type == 4)
                channel_type_nName = "ST-BY NCS";
            else
                channel_type_nName = "Reserved";

            int local = pkt[6] >> 2 & 0x07;
            int sat = pkt[7] >> 6 & 0x03;
            int les_id = pkt[7] & 0x3F;
            uint8_t status_b = pkt[8];
            int iss = pkt[9] << 8 | pkt[10];
            int random_interval = pkt[11];

            output["status"]["Bauds600"] = std::to_string((status_b & 0x80) >> 7 == 1);
            output["status"]["Operational"] = std::to_string((status_b & 0x40) >> 6 == 1);
            output["status"]["InService"] = std::to_string((status_b & 0x20) >> 5 == 1);
            output["status"]["Clear"] = std::to_string((status_b & 0x10) >> 4 == 1);
            output["status"]["LinksOpen"] = std::to_string((status_b & 0x08) >> 3 == 1);

            output["network_version"] = network_version;
            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = les_id;
            output["les_name"] = get_les_name(sat, les_id);
            output["signalling_channel"] = signalling_channel;
            output["count"] = count;
            output["channel_type"] = channel_type;
            output["channel_type_name"] = channel_type_nName;
            output["local"] = local;
            output["services"] = get_services(iss);
            output["random_interval"] = random_interval;
            output["timestamp_hour"] = timestamp_hours;
            output["timestamp_min"] = timestamp_min;
            output["timestamp_sec"] = timestamp_sec;
            output["timestamp_msec"] = timestamp_msec;
        }

        void STDPacketParser::parse_pkt_81(uint8_t *pkt, nlohmann::json &output)
        {
            int mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
            int sat = pkt[5] >> 6 & 0x03;
            int les_id = pkt[5] & 0x3F;
            int logical_channel_no = pkt[9];

            double downlink_mhz = ((pkt[6] << 8 | pkt[7]) - 8000) * 0.0025 + 15305;
            int presentation = pkt[14];

            output["mes_id"] = mes_id;
            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = sat;
            output["les_name"] = get_les_name(sat, les_id);
            output["logical_channel_no"] = logical_channel_no;
            output["downlink_channel_mhz"] = downlink_mhz;
            output["presentation"] = presentation;
            output["unknown1_hex"] = buf_to_hex_str(&pkt[8], 1);
            output["unknown2_hex"] = buf_to_hex_str(&pkt[10], 4);
            output["unknown3_hex"] = buf_to_hex_str(&pkt[15], 2);
        }

        void STDPacketParser::parse_pkt_83(uint8_t *pkt, nlohmann::json &output)
        {
            int mes_id = pkt[2] << 16 | pkt[2 + 1] << 8 | pkt[2 + 2];
            int sat = pkt[5] >> 6 & 0x03;
            int les_id = pkt[5] & 0x3F;
            uint8_t status_bits = pkt[6];
            int logical_channel_no = pkt[7];
            int frame_length = pkt[8];
            int duration = pkt[9];
            double downlink_mhz = ((pkt[10] << 8 | pkt[10 + 1]) - 8000) * 0.0025 + 1530.5;
            double uplink_mhz = ((pkt[12] << 8 | pkt[12 + 1]) - 6000) * 0.0025 + 1626.5;
            int frame_offset = pkt[14];
            uint8_t packet_descriptor_1 = pkt[15];

            output["mes_id"] = mes_id;
            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = sat;
            output["les_name"] = get_les_name(sat, les_id);
            output["status_bits"] = status_bits;
            output["logical_channel_no"] = logical_channel_no;
            output["frame_length"] = frame_length;
            output["duration"] = duration;
            output["uplink_channel_mhz"] = uplink_mhz;
            output["downlink_channel_mhz"] = downlink_mhz;
            output["frame_offset"] = frame_offset;
            output["packet_descriptor_1"] = packet_descriptor_1;
        }

        void STDPacketParser::parse_pkt_91(uint8_t *pkt, nlohmann::json &output)
        {
        }

        void STDPacketParser::parse_pkt_92(uint8_t *pkt, nlohmann::json &output)
        {
            int login_ack_length = pkt[1];
            double downlink_mhz = ((pkt[5] << 8 | pkt[5 + 1]) - 8000) * 0.0025 + 1530.5;

            output["login_ack_length"] = login_ack_length;
            output["downlink_channel_mhz"] = downlink_mhz;
            output["les"] = buf_to_hex_str(&pkt[2], 3);
            output["station_start_hex"] = buf_to_hex_str(&pkt[7], 1);

            if (login_ack_length > 7)
            {
                int station_count = pkt[8];
                std::string stations = get_stations(&pkt[9], station_count);
                output["station_count"] = station_count;
                output["stations"] = stations;
            }
        }

        void STDPacketParser::parse_pkt_9a(uint8_t *pkt, nlohmann::json &output)
        {
        }

        void STDPacketParser::parse_pkt_a0(uint8_t *pkt, nlohmann::json &output)
        {
        }

        void STDPacketParser::parse_pkt_a3(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            int mes_id = pkt[2] << 16 | pkt[2 + 1] << 8 | pkt[2 + 2];
            int sat = pkt[5] >> 6 & 0x03;
            int les_id = pkt[5] & 0x3F;

            std::string unknown1_hex;

            if (pkt_len >= 38)
            {
                int j = 13;
                std::string short_message;
                for (int i = 0; j < pkt_len - 2; i++)
                {
                    short_message += (char)pkt[j] & 0x7F; // x-IA5 encoding
                    j++;
                }
                output["short_message"] = short_message;
                unknown1_hex = buf_to_hex_str(&pkt[6], 6);
            }
            else
            {
                unknown1_hex = buf_to_hex_str(&pkt[6], 6);
            }

            output["mes_id"] = mes_id;
            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = les_id;
            output["les_name"] = get_les_name(sat, les_id);
            output["unknown1_hex"] = unknown1_hex;
        }

        void STDPacketParser::parse_pkt_a8(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            int mes_id = pkt[2] << 16 | pkt[2 + 1] << 8 | pkt[2 + 2];
            int sat = pkt[5] >> 6 & 0x03;
            int les_id = pkt[5] & 0x3F;
            int short_message_length = pkt[9];
            std::string unknown1_hex = buf_to_hex_str(&pkt[6], 3);
            std::string unknown2_hex = buf_to_hex_str(&pkt[10], 1);

            if (short_message_length > 2)
            {
                std::string short_message;
                int j = 11;
                for (int i = 0; j < pkt_len - 2; i++)
                {
                    short_message += (char)pkt[j] & 0x7F; // x-IA5 encoding
                    j++;
                }
                output["short_message"] = short_message;
            }

            output["mes_id"] = mes_id;
            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = les_id;
            output["les_name"] = get_les_name(sat, les_id);
            output["unknown1_hex"] = unknown1_hex;
            output["unknown2_hex"] = unknown2_hex;
        }

        void STDPacketParser::parse_pkt_aa(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            int sat = pkt[2] >> 6 & 0x03;
            int les_id = pkt[2] & 0x3F;
            int logical_channel_no = pkt[3];
            int packet_no = pkt[4];

            std::vector<uint8_t> payload(pkt_len - 2 - 4);
            memcpy(payload.data(), &pkt[5], pkt_len - 2 - 5);

            int payload_presentation = is_binary(payload, true) ? 7 : 0;

            output["sat"] = sat;
            output["sat_name"] = get_sat_name(sat);
            output["les_id"] = sat;
            output["les_name"] = get_les_name(sat, les_id);
            output["logical_channel_no"] = logical_channel_no;
            output["packet_no"] = packet_no;
            output["message"] = message_to_string(payload, payload_presentation);
            // output["payload"] = payload;
            // output["payload_presentation"] = wip_payload_presentation;
        }

        void STDPacketParser::parse_pkt_ab(uint8_t *pkt, nlohmann::json &output)
        {
            int les_list_length = pkt[1];
            int station_count = pkt[3];
            std::string stations = get_stations(&pkt[4], station_count);
            output["les_list_length"] = les_list_length;
            output["station_start_hex"] = buf_to_hex_str(&pkt[2], 1);
            output["stations"] = stations;
            output["station_count"] = station_count;
        }

        void STDPacketParser::parse_pkt_ac(uint8_t *pkt, nlohmann::json &output)
        {
        }

        void STDPacketParser::parse_pkt_ad(uint8_t *pkt, nlohmann::json &output)
        {
        }

        void STDPacketParser::parse_pkt_b1(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            int message_type = pkt[2];
            int continuation = (pkt[3] & 0x80) >> 7;
            int priority = (pkt[3] & 0x60) >> 5;
            bool is_distress = priority == 3;
            int repetition = pkt[3] & 0x1F;
            int message_id = pkt[4] << 8 | pkt[5];
            int packet_no = pkt[6];
            bool is_new_payload = packet_no == 1;
            bool presentation = pkt[7];

            output["message_type"] = message_type;
            output["service_code_and_address_name"] = get_service_code_and_address_name(message_type);
            output["continuation"] = continuation;
            output["priority"] = priority;
            output["priority_text"] = get_priority(priority);
            output["is_distress"] = is_distress;
            output["repetition"] = repetition;
            output["message_id"] = message_id;
            output["packet_no"] = packet_no;
            output["is_new_payload"] = is_new_payload;

            int address_length = get_address_length(message_type);

            // NAV/MET coordinator... area... TODO
            if (8 + address_length >= pkt_len)
                return;

            output["address_hex"] = buf_to_hex_str(&pkt[8], address_length);

            int payload_length = pkt_len - 2 - 8 - address_length;
            int k = 8 + address_length;
            std::vector<uint8_t> payload;
            for (int i = 0; k < 8 + address_length + payload_length; i++)
            {
                payload.push_back(pkt[k]);
                k++;
            }

            output["message"] = message_to_string(payload, presentation);
        }

        void STDPacketParser::parse_pkt_b2(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            int message_type = pkt[2];
            int continuation = (pkt[3] & 0x80) >> 7;
            int priority = (pkt[3] & 0x60) >> 5;
            bool is_distress = priority == 3;
            int repetition = pkt[3] & 0x1F;
            int message_id = pkt[4] << 8 | pkt[5];
            int packet_no = pkt[6];
            bool is_new_payload = packet_no == 1;
            bool presentation = pkt[7];

            output["message_type"] = message_type;
            output["service_code_and_address_name"] = get_service_code_and_address_name(message_type);
            output["continuation"] = continuation;
            output["priority"] = priority;
            output["priority_text"] = get_priority(priority);
            output["is_distress"] = is_distress;
            output["repetition"] = repetition;
            output["message_id"] = message_id;
            output["packet_no"] = packet_no;
            output["is_new_payload"] = is_new_payload;

            int address_length = get_address_length(message_type);

            // NAV/MET coordinator... area... TODO
            if (8 + address_length >= pkt_len)
                return;

            output["address_hex"] = buf_to_hex_str(&pkt[8], address_length);

            int payload_length = pkt_len - 2 - 8 - address_length;
            int k = 8 + address_length;
            std::vector<uint8_t> payload;
            for (int i = 0; k < 8 + address_length + payload_length; i++)
            {
                payload.push_back(pkt[k]);
                k++;
            }

            output["message"] = message_to_string(payload, presentation);
        }

        void STDPacketParser::parse_pkt_bd(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            uint8_t mid = pkt[2] & 0xFF;

            int mpkt_len = 0;
            if (mid >> 7 == 0)
                mpkt_len = (mid & 0x0F) + 1;
            else if (mid >> 6 == 0x02)
                mpkt_len = pkt[3] + 2;

            wip_multi_frame_pkt.clear();
            wip_multi_frame_pkt.resize(mpkt_len, 0);

            wip_multi_frame_gotten_size = pkt_len - 2 - 2;
            memcpy(&wip_multi_frame_pkt[0], &pkt[2], wip_multi_frame_gotten_size);

            wip_multi_frame_has_start = true;
        }

        void STDPacketParser::parse_pkt_be(uint8_t *pkt, int pkt_len, nlohmann::json &output)
        {
            if (!wip_multi_frame_has_start)
                return;
            int actual_length = pkt_len - 2 - 2;
            memcpy(&wip_multi_frame_pkt[wip_multi_frame_gotten_size], &pkt[2], actual_length);
            wip_multi_frame_gotten_size += actual_length;
        }

        void STDPacketParser::parse_main_pkt(uint8_t *main_pkt, int main_pkt_len)
        {
            double timestamp = time(0);

            int pos = 0;
            while (pos < main_pkt_len)
            {
                // Setup utils vars
                uint8_t *pkt = &main_pkt[pos];
                int pkt_len_max = main_pkt_len - pos;

                // Parse pkt
                uint8_t id = pkt[0];
                int pkt_len = pkt_len_max;

                if (id == 0x00) // No more packets
                    return;

                // Compute pkt length
                if (id >> 7 == 0)
                    pkt_len = (id & 0x0F) + 1;
                else if (id >> 6 == 0x02)
                    pkt_len = pkt[1] + 2;
                else
                    pkt_len = pkt_len_max;

                // Avoid going out of bounds
                if (pkt_len > pkt_len_max) // No more packets, corrupted...
                    throw std::runtime_error("Invalid PKT length!");

                // Check CRC
                uint16_t sent_crc = (pkt[pkt_len - 2] << 8) | pkt[pkt_len - 1];
                uint16_t comp_crc = compute_crc(pkt, pkt_len);
                bool crc_valid = sent_crc == 0 || sent_crc == comp_crc;

                if (!crc_valid) // No more packets, corrupted...
                    throw std::runtime_error("Invalid CRC! Skipping.");

                // logger->warn("Packet 0x{:X} ({:s}) : ", id, get_id_name(id).c_str());

                // Clear output
                output_meta.clear();

                output_meta["pkt_id"] = id;
                output_meta["pkt_type"] = get_id_name(id);

                switch (id)
                {
                case 0x27:
                    parse_pkt_27(pkt, output_meta);
                    break;
                case 0x2a:
                    parse_pkt_2a(pkt, output_meta);
                    break;
                case 0x08:
                    parse_pkt_08(pkt, output_meta);
                    break;
                case 0x6c:
                    parse_pkt_6c(pkt, output_meta);
                    break;
                case 0x7d:
                    parse_pkt_7d(pkt, output_meta);
                    break;
                case 0x81:
                    parse_pkt_81(pkt, output_meta);
                    break;
                case 0x83:
                    parse_pkt_83(pkt, output_meta);
                    break;
                case 0x91:
                    parse_pkt_91(pkt, output_meta);
                    break;
                case 0x92:
                    parse_pkt_92(pkt, output_meta);
                    break;
                case 0x9a:
                    parse_pkt_9a(pkt, output_meta);
                    break;
                case 0xa0:
                    parse_pkt_a0(pkt, output_meta);
                    break;
                case 0xa3:
                    parse_pkt_a3(pkt, pkt_len, output_meta);
                    break;
                case 0xa8:
                    parse_pkt_a8(pkt, pkt_len, output_meta);
                    break;
                case 0xaa: // Message
                    parse_pkt_aa(pkt, pkt_len, output_meta);
                    break;
                case 0xab:
                    parse_pkt_ab(pkt, output_meta);
                    break;
                case 0xac:
                    parse_pkt_ac(pkt, output_meta);
                    break;
                case 0xad:
                    parse_pkt_ad(pkt, output_meta);
                    break;
                case 0xb1:
                    parse_pkt_b1(pkt, pkt_len, output_meta);
                    break;
                case 0xb2:
                    parse_pkt_b2(pkt, pkt_len, output_meta);
                    break;
                case 0xbd: // Multiframe start
                    parse_pkt_bd(pkt, pkt_len, output_meta);
                    break;
                case 0xbe: // Multiframe end
                    parse_pkt_be(pkt, pkt_len, output_meta);

                    // Check packet is usable
                    if (wip_multi_frame_has_start && (wip_multi_frame_gotten_size == (int)wip_multi_frame_pkt.size() - 2))
                    {
                        STDPacketParser dec;
                        dec.on_packet = on_packet;
                        dec.parse_main_pkt(wip_multi_frame_pkt.data(), wip_multi_frame_pkt.size());
                    }

                    wip_multi_frame_has_start = false;
                    wip_multi_frame_gotten_size = 0;
                    wip_multi_frame_pkt.clear();

                    break;
                }

                // If this is 0x7D and first frame, use better timestamp
                if (id == 0x7D && pos == 0)
                {
                    time_t currentDay = time(0);
                    time_t dayValue = currentDay - (currentDay % 86400);
                    int hour = output_meta["timestamp_hour"];
                    int min = output_meta["timestamp_min"];
                    int sec = output_meta["timestamp_sec"];
                    int msec = output_meta["timestamp_msec"];

                    timestamp = dayValue + hour * 3600.0 + min * 60.0 + sec + double(msec / 1000.0);
                }

                // Timestamp packet
                output_meta["timestamp"] = timestamp + ((double)pos / (double)main_pkt_len) * 8.64;

                // Not a multiframe PKT, process it now.
                if (id != 0xbd && id != 0xbe)
                {
                    on_packet(output_meta);
                    output_meta.clear();
                }

                pos += pkt_len;
            }
        }
    }
}