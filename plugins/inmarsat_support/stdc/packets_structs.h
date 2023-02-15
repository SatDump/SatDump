#pragma once

#include <cstdint>
#include <vector>
#include "nlohmann/json.hpp"

namespace inmarsat
{
    namespace stdc
    {
        std::string get_id_name(uint8_t id);
        std::string get_sat_name(int sat);
        std::string get_les_name(int sat, int lesId);

        nlohmann::json get_services_short(uint8_t is8);
        nlohmann::json get_services(int iss);
        nlohmann::json get_stations(uint8_t *data, int stationCount);

        std::string buf_to_hex_str(uint8_t *buf, int len);
        bool is_binary(std::vector<uint8_t> data, bool checkAll);
        std::string message_to_string(std::vector<uint8_t> buf, int presentation, bool egc);
        std::string get_service_code_and_address_name(int code);
        std::string get_priority(int priority);
        int get_address_length(int messageType);
        std::string string_from_ia5(uint8_t *buf, int len);

        double parse_uplink_freq_mhz(uint8_t *b);
        double parse_downlink_freq_mhz(uint8_t *b);

        std::string service4_name(uint8_t service_b);
        std::string direction2_name(uint8_t direction_b);

        int get_packet_frm_id(nlohmann::json pkt);

        namespace pkts
        {
            using namespace ::inmarsat::stdc;

            // Common descriptor to all packets
            struct PacketDescriptor
            {
                bool is_short;  // 1 byte
                bool is_medium; // 2 bytes
                bool is_long;   // 3 bytes

                uint8_t type;
                uint16_t length;

                PacketDescriptor() {}
                PacketDescriptor(uint8_t *pkt)
                {
                    if (pkt[0] >> 7 == 0) // Short
                    {
                        type = (pkt[0] >> 4) & 0b111;
                        length = (pkt[0] & 0xF) + 1;
                        is_short = true;
                        is_medium = false;
                        is_long = false;
                    }
                    else if (pkt[0] >> 6 == 2) // Medium
                    {
                        type = pkt[0] & 0b111111;
                        length = pkt[1] + 2;
                        is_short = false;
                        is_medium = true;
                        is_long = false;
                    }
                    else if (pkt[0] >> 6 == 3) // Long
                    {
                        type = pkt[0] & 0b111111;
                        length = (pkt[1] << 8 | pkt[2]) + 3;
                        is_short = false;
                        is_medium = true;
                        is_long = false;
                    }
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketDescriptor, is_short, is_medium, is_long, type, length)
            };

            // Common packet format :
            // - Descriptor
            // - Data field
            // - Checksum
            struct PacketBase
            {
                PacketDescriptor descriptor;

                uint16_t compute_crc(uint8_t *buf, int size)
                {
                    short C0 = 0, C1 = 0;
                    uint8_t CB1, CB2, B;
                    int i = 0;
                    while (i < size)
                    {
                        if (i < size - 2)
                            B = buf[i];
                        else
                            B = 0;
                        C0 += B;
                        C1 += C0;
                        i++;
                    }
                    CB1 = (uint8_t)(C0 - C1);
                    CB2 = (uint8_t)(C1 - 2 * C0);
                    return (CB1 << 8) | CB2;
                }

                PacketBase() {}
                PacketBase(uint8_t *pkt, int len_max)
                {
                    descriptor = PacketDescriptor(pkt);

                    // Check length is valid
                    if (descriptor.length > len_max)
                        throw std::runtime_error("Invalid PKT length!");

                    // Check CRC
                    uint16_t sent_crc = (pkt[descriptor.length - 2] << 8) | pkt[descriptor.length - 1];
                    uint16_t comp_crc = compute_crc(pkt, descriptor.length);
                    bool crc_valid = sent_crc == 0 || sent_crc == comp_crc;

                    if (!crc_valid)
                        throw std::runtime_error("Invalid CRC!");
                }

                // NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketBase, descriptor)
            };

            // Bulletin Board packet
            struct PacketBulletinBoard : public PacketBase
            {
                static const uint8_t FRM_ID = 0x07;

                // Frame Descriptor
                uint8_t network_version;
                uint16_t frame_number;
                uint8_t signalling_channels;
                uint8_t frame_2_count;
                bool empty_frame;

                double seconds_of_day;

                // TDM Descriptor
                uint8_t channel_type;
                uint8_t local_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint8_t status_b;
                uint16_t services_b;
                uint8_t randomizing_interval;

                std::string channel_type_name;
                std::string sat_name;
                std::string les_name;
                nlohmann::json status;
                nlohmann::json services;

                PacketBulletinBoard() {}
                PacketBulletinBoard(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    // Parse frame desc.
                    network_version = pkt[1];
                    frame_number = pkt[2] << 8 | pkt[3];
                    signalling_channels = pkt[4] >> 2;
                    frame_2_count = (pkt[5] >> 4 & 0x0F) * 2;
                    empty_frame = (pkt[5] >> 3) & 1;

                    seconds_of_day = frame_number * 8.64;

                    // Parse TDM desc.
                    channel_type = pkt[6] >> 0x05;
                    local_id = pkt[6] >> 2 & 0x07;
                    sat_id = pkt[7] >> 6 & 0x03;
                    les_id = pkt[7] & 0x3F;
                    status_b = pkt[8];
                    services_b = pkt[9] << 8 | pkt[10];
                    randomizing_interval = pkt[11];

                    if (channel_type == 1)
                        channel_type_name = "NCS";
                    else if (channel_type == 2)
                        channel_type_name = "LES TDM";
                    else if (channel_type == 3)
                        channel_type_name = "Joint NCS and TDM";
                    else if (channel_type == 4)
                        channel_type_name = "ST-BY NCS";
                    else
                        channel_type_name = "Reserved";

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);

                    status["return_link_speed"] = (status_b & 0x80) >> 7 == 1 ? 600 : 300;
                    status["operational_sat"] = (status_b & 0x40) >> 6 == 1;
                    status["in_service"] = (status_b & 0x20) >> 5 == 1;
                    status["clear"] = (status_b & 0x10) >> 4 == 1;
                    status["links_open"] = (status_b & 0x08) >> 3 == 1;
                    status["covert_alerting"] = (status_b & 1) == 1;

                    services = get_services(services_b);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketBulletinBoard, descriptor,
                                               network_version, frame_number, signalling_channels, frame_2_count, empty_frame, seconds_of_day,
                                               channel_type, local_id, sat_id, les_id, status_b, services_b, randomizing_interval, channel_type_name, sat_name, les_name, status, services)
            };

            // Signalling Channel packet
            struct PacketSignallingChannel : public PacketBase
            {
                static const uint8_t FRM_ID = 0x06;

                uint8_t services_b;
                double uplink_freq_mhz;
                std::vector<int> tdm_slots;

                nlohmann::json services;

                PacketSignallingChannel() {}
                PacketSignallingChannel(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    services_b = pkt[1];
                    uplink_freq_mhz = parse_uplink_freq_mhz(&pkt[2]);

                    tdm_slots.resize(28);
                    for (int i = 0, j = 0; i < 28; i += 4, j++)
                    {
                        tdm_slots[i] = pkt[4 + j] >> 6;
                        tdm_slots[i + 1] = pkt[4 + j] >> 4 & 3;
                        tdm_slots[i + 2] = pkt[4 + j] >> 2 & 3;
                        tdm_slots[i + 3] = pkt[4 + j] & 3;
                    }

                    services = get_services_short(services_b);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketSignallingChannel, descriptor,
                                               uplink_freq_mhz, tdm_slots, services_b, services)
            };

            // Acknowledgement packet
            struct PacketAcknowledgement : public PacketBase
            {
                static const uint8_t FRM_ID = 0x10;

                uint8_t sat_id;
                uint8_t les_id;
                uint8_t logical_channel_number;
                uint8_t frame_length;
                uint8_t duration;
                uint16_t message_channel;
                uint8_t frame_offset;
                bool am_pm_bit;
                uint8_t slot_number;
                std::vector<int> errored_packet_numbers;

                std::string sat_name;
                std::string les_name;

                PacketAcknowledgement() {}
                PacketAcknowledgement(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    sat_id = pkt[2] >> 6 & 0x03;
                    les_id = pkt[2] & 0x3F;
                    logical_channel_number = pkt[3];
                    frame_length = pkt[4];
                    duration = pkt[5];
                    message_channel = pkt[6] << 8 | pkt[7];
                    frame_offset = pkt[8];
                    am_pm_bit = pkt[9] >> 7;
                    slot_number = pkt[9] & 0b11111;

                    for (int i = 0; i < descriptor.length - 10 - 2; i++)
                        errored_packet_numbers.push_back(pkt[9 + i]);

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketAcknowledgement, descriptor,
                                               sat_id, les_id, logical_channel_number, frame_length, duration, message_channel, frame_offset, am_pm_bit, slot_number, errored_packet_numbers,
                                               sat_name, les_name)
            };

            // Acknowledgement Request packet
            struct PacketAcknowledgementRequest : public PacketBase
            {
                static const uint8_t FRM_ID = 0x00;

                uint8_t sat_id;
                uint8_t les_id;
                uint8_t logical_channel_number;
                double uplink_freq_mhz;
                uint8_t frame_offset;
                bool am_pm_bit;
                uint8_t slot_number;

                std::string sat_name;
                std::string les_name;

                PacketAcknowledgementRequest() {}
                PacketAcknowledgementRequest(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    sat_id = pkt[1] >> 6 & 0x03;
                    les_id = pkt[1] & 0x3F;
                    logical_channel_number = pkt[2];
                    uplink_freq_mhz = parse_uplink_freq_mhz(&pkt[3]);
                    frame_offset = pkt[5];
                    am_pm_bit = pkt[6] >> 7;
                    slot_number = pkt[6] & 0b11111;

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketAcknowledgementRequest, descriptor,
                                               sat_id, les_id, logical_channel_number, uplink_freq_mhz, frame_offset, am_pm_bit, slot_number,
                                               sat_name, les_name)
            };

            // Announcement packet
            struct PacketAnnouncement : public PacketBase
            {
                static const uint8_t FRM_ID = 0x01;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                double downlink_freq_mhz;
                uint8_t service_b;
                uint8_t direction_b;
                uint8_t priority_b;

                uint8_t logical_channel_number;
                uint32_t message_reference_number;
                uint8_t sub_address;
                uint8_t presentation;
                uint8_t number_of_packets;
                uint8_t last_count;

                std::string sat_name;
                std::string les_name;
                std::string service;
                std::string direction;
                std::string priority;

                PacketAnnouncement() {}
                PacketAnnouncement(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    downlink_freq_mhz = parse_downlink_freq_mhz(&pkt[6]);
                    service_b = pkt[8] >> 4;
                    direction_b = (pkt[8] >> 2) & 0b11;
                    priority_b = pkt[8] & 0b11;

                    if (direction_b == 0)
                    {
                        logical_channel_number = pkt[9];
                        message_reference_number = pkt[10] << 16 | pkt[11] << 8 | pkt[12];
                        sub_address = pkt[13];
                        presentation = pkt[14];
                        number_of_packets = pkt[15];
                        last_count = pkt[16];
                    }

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);

                    service = service4_name(service_b);
                    direction = direction2_name(direction_b);

                    if (priority_b == 0)
                        priority = "Routine";
                    else if (priority_b == 3)
                        priority = "Distress";
                    else
                        priority = "Unknown";
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketAnnouncement, descriptor,
                                               mes_id, sat_id, les_id, downlink_freq_mhz, service_b, direction_b, priority_b,
                                               logical_channel_number, message_reference_number, sub_address, presentation, number_of_packets, last_count,
                                               sat_name, les_name, service, direction, priority)
            };

            // LES Forced Clear packet
            struct PacketLESForcedClear : public PacketBase
            {
                static const uint8_t FRM_ID = 0x19;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint8_t logical_channel_number;
                uint8_t reason_for_clear_b;

                std::string sat_name;
                std::string les_name;
                std::string reason_for_clear;

                PacketLESForcedClear() {}
                PacketLESForcedClear(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    logical_channel_number = pkt[6];
                    reason_for_clear_b = pkt[7];

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);

                    if (reason_for_clear_b == 1)
                        reason_for_clear = "LES Timeout";
                    else if (reason_for_clear_b == 2)
                        reason_for_clear = "MES Procotol Error";
                    else if (reason_for_clear_b == 3)
                        reason_for_clear = "LES Hardware Error";
                    else if (reason_for_clear_b == 4)
                        reason_for_clear = "Operator Forced Clear";
                    else if (reason_for_clear_b == 5)
                        reason_for_clear = "MES Forced Clear";
                    else if (reason_for_clear_b == 6)
                        reason_for_clear = "LES Protocol Error";
                    else if (reason_for_clear_b == 7)
                        reason_for_clear = "MES Hardware Error";
                    else if (reason_for_clear_b == 8)
                        reason_for_clear = "MES Timeout";
                    else if (reason_for_clear_b == 9)
                        reason_for_clear = "Unknown Presentation code";
                    else if (reason_for_clear_b == 0xd)
                        reason_for_clear = "Requested Service Temporarily Unavailable";
                    else if (reason_for_clear_b == 0xe)
                        reason_for_clear = "Access To Requested Service Denied";
                    else if (reason_for_clear_b == 0xf)
                        reason_for_clear = "Invalid Service";
                    else if (reason_for_clear_b == 0x10)
                        reason_for_clear = "Invalid Address";
                    else if (reason_for_clear_b == 0x11)
                        reason_for_clear = "Destination MES Not Commissioned";
                    else if (reason_for_clear_b == 0x12)
                        reason_for_clear = "Destination MES Not Logged In";
                    else if (reason_for_clear_b == 0x13)
                        reason_for_clear = "Destination MES Barred";
                    else if (reason_for_clear_b == 0x14)
                        reason_for_clear = "Requested Service Not Provided";
                    else if (reason_for_clear_b == 0xa)
                        reason_for_clear = "Unable To Decode: Specified Dictionary Version Not Available";
                    else if (reason_for_clear_b == 0xb)
                        reason_for_clear = "IWU Number Is Invalid";
                    else if (reason_for_clear_b == 0xc)
                        reason_for_clear = "MES Has Not Subscribed To This Service";
                    else if (reason_for_clear_b == 0x15)
                        reason_for_clear = "Protocol Version Not Supported";
                    else if (reason_for_clear_b == 0x16)
                        reason_for_clear = "Unrecognized PDU Type";
                    else
                        reason_for_clear = "Unknown";
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketLESForcedClear, descriptor,
                                               mes_id, sat_id, les_id, logical_channel_number, reason_for_clear_b, reason_for_clear,
                                               sat_name, les_name)
            };

            // Clear packet
            struct PacketClear : public PacketBase
            {
                static const uint8_t FRM_ID = 0x02;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint8_t logical_channel_number;

                std::string sat_name;
                std::string les_name;

                PacketClear() {}
                PacketClear(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[1] << 16 | pkt[2] << 8 | pkt[3];
                    sat_id = pkt[4] >> 6 & 0x03;
                    les_id = pkt[4] & 0x3F;
                    logical_channel_number = pkt[5];

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketClear, descriptor,
                                               mes_id, sat_id, les_id, logical_channel_number,
                                               sat_name, les_name)
            };

            // Confirmation packet
            struct PacketConfirmation : public PacketBase
            {
                static const uint8_t FRM_ID = 0x28;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint32_t message_reference_number;

                uint8_t descriptor_length;
                bool status;
                uint8_t attempts_number;
                std::string non_delivery_code;
                std::string address_information;

                std::string sat_name;
                std::string les_name;

                PacketConfirmation() {}
                PacketConfirmation(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    message_reference_number = pkt[6] << 16 | pkt[7] << 8 | pkt[8];

                    descriptor_length = pkt[9];
                    status = pkt[10] >> 7;
                    attempts_number = pkt[10] & 0b1111111;
                    non_delivery_code = string_from_ia5(&pkt[11], 3);
                    address_information = string_from_ia5(&pkt[14], descriptor_length - 5);

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketConfirmation, descriptor,
                                               mes_id, sat_id, les_id, message_reference_number, descriptor_length, status, attempts_number, non_delivery_code, address_information,
                                               sat_name, les_name)
            };

            // DistressAlertAcknowledgement packet
            struct PacketDistressAlertAcknowledgement : public PacketBase
            {
                static const uint8_t FRM_ID = 0x11;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;

                std::string sat_name;
                std::string les_name;

                PacketDistressAlertAcknowledgement() {}
                PacketDistressAlertAcknowledgement(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketDistressAlertAcknowledgement, descriptor,
                                               mes_id, sat_id, les_id,
                                               sat_name, les_name)
            };

            // PacketDistressTestRequest packet
            struct PacketDistressTestRequest : public PacketBase
            {
                static const uint8_t FRM_ID = 0x20;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;

                std::string sat_name;
                std::string les_name;

                PacketDistressTestRequest() {}
                PacketDistressTestRequest(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketDistressTestRequest, descriptor,
                                               mes_id, sat_id, les_id,
                                               sat_name, les_name)
            };

            // PacketEGCCommon packet
            struct PacketEGCCommon : public PacketBase
            {
                uint8_t service_code_b;
                bool continuation;
                uint8_t priority_b;
                uint8_t repetition_number;
                uint16_t message_sequence_number;
                uint8_t packet_sequence_number;
                std::vector<uint8_t> address_raw;
                uint8_t presentation;
                std::vector<uint8_t> data;

                std::string service_code_and_address_name;
                std::string priority;
                std::string message;

                PacketEGCCommon() {}
                PacketEGCCommon(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    service_code_b = pkt[2];
                    continuation = (pkt[3] & 0x80) >> 7;
                    priority_b = (pkt[3] & 0x60) >> 5;
                    repetition_number = pkt[3] & 0x1F;
                    message_sequence_number = pkt[4] << 8 | pkt[5];
                    packet_sequence_number = pkt[6];
                    presentation = pkt[7];

                    service_code_and_address_name = get_service_code_and_address_name(service_code_b);
                    priority = get_priority(priority_b);

                    int address_length = get_address_length(service_code_b);

                    // NAV/MET coordinator... area... TODO
                    if (8 + address_length >= descriptor.length)
                    {
                    }
                    else
                    {
                        address_raw.resize(address_length);
                        memcpy(address_raw.data(), &pkt[8], address_length);

                        int payload_length = descriptor.length - 2 - 8 - address_length;
                        int k = 8 + address_length;
                        data.clear();
                        for (int i = 0; k < 8 + address_length + payload_length; i++)
                        {
                            data.push_back(pkt[k]);
                            k++;
                        }

                        message = message_to_string(data, presentation, true);
                    }
                }
            };

            // PacketEGCSingleHeader packet
            struct PacketEGCSingleHeader : public PacketEGCCommon
            {
                static const uint8_t FRM_ID = 0x30;

                PacketEGCSingleHeader() {}
                PacketEGCSingleHeader(uint8_t *pkt, int len_max) : PacketEGCCommon(pkt, len_max)
                {
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketEGCSingleHeader, descriptor,
                                               service_code_b, continuation, priority_b, repetition_number, message_sequence_number, packet_sequence_number, address_raw, presentation, data,
                                               service_code_and_address_name, priority, message)
            };

            // PacketEGCDoubleHeader1 packet
            struct PacketEGCDoubleHeader1 : public PacketEGCCommon
            {
                static const uint8_t FRM_ID = 0x31;

                PacketEGCDoubleHeader1() {}
                PacketEGCDoubleHeader1(uint8_t *pkt, int len_max) : PacketEGCCommon(pkt, len_max)
                {
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketEGCDoubleHeader1, descriptor,
                                               service_code_b, continuation, priority_b, repetition_number, message_sequence_number, packet_sequence_number, address_raw, presentation, data,
                                               service_code_and_address_name, priority, message)
            };

            // PacketEGCDoubleHeader2 packet
            struct PacketEGCDoubleHeader2 : public PacketEGCCommon
            {
                static const uint8_t FRM_ID = 0x32;

                PacketEGCDoubleHeader2() {}
                PacketEGCDoubleHeader2(uint8_t *pkt, int len_max) : PacketEGCCommon(pkt, len_max)
                {
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketEGCDoubleHeader2, descriptor,
                                               service_code_b, continuation, priority_b, repetition_number, message_sequence_number, packet_sequence_number, address_raw, presentation, data,
                                               service_code_and_address_name, priority, message)
            };

            // PacketLogicalChannelAssignement packet
            struct PacketLogicalChannelAssignement : public PacketBase
            {
                static const uint8_t FRM_ID = 0x03;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint8_t service_b;
                uint8_t direction_b;

                uint8_t number_of_packets;
                uint8_t last_count;
                double uplink_freq_mhz;
                uint8_t frame_offset;
                bool am_pm_bit;
                uint8_t slot_number;

                uint8_t logical_channel_number;
                uint8_t frame_length;
                uint8_t duration;
                double downlink_freq_mhz;
                uint16_t message_channel;

                std::string sat_name;
                std::string les_name;
                std::string service;
                std::string direction;

                PacketLogicalChannelAssignement() {}
                PacketLogicalChannelAssignement(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    service_b = pkt[6] >> 4;
                    direction_b = (pkt[6] >> 2) & 0b11;

                    if (direction_b == 0)
                    {
                        number_of_packets = pkt[7];
                        last_count = pkt[8];
                        uplink_freq_mhz = parse_uplink_freq_mhz(&pkt[9]);
                        frame_offset = pkt[11];
                        am_pm_bit = pkt[12] >> 7;
                        slot_number = pkt[13] & 0b11111;
                    }
                    else
                    {
                        logical_channel_number = pkt[7];
                        frame_length = pkt[8];
                        duration = pkt[9];
                        downlink_freq_mhz = parse_downlink_freq_mhz(&pkt[10]);
                        message_channel = pkt[12] << 8 | pkt[13];
                        frame_offset = pkt[14];
                        am_pm_bit = pkt[15] >> 7;
                        slot_number = pkt[16] & 0b11111;
                    }

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);

                    service = service4_name(service_b);
                    direction = direction2_name(direction_b);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketLogicalChannelAssignement, descriptor,
                                               mes_id, sat_id, les_id, service_b, direction_b,
                                               number_of_packets, last_count, uplink_freq_mhz, frame_offset, am_pm_bit, slot_number,
                                               logical_channel_number, frame_length, duration, downlink_freq_mhz, message_channel,
                                               sat_name, les_name, service, direction)
            };

            // PacketLoginAcknowledgment packet
            struct PacketLoginAcknowledgment : public PacketBase
            {
                static const uint8_t FRM_ID = 0x12;

                uint32_t mes_id;
                double downlink_freq_mhz;

                uint8_t network_version;
                uint8_t les_total;
                nlohmann::json stations;

                PacketLoginAcknowledgment() {}
                PacketLoginAcknowledgment(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    downlink_freq_mhz = parse_downlink_freq_mhz(&pkt[5]);
                    network_version = pkt[6];
                    if (descriptor.length > 7)
                    {
                        les_total = pkt[8];
                        stations = get_stations(&pkt[9], les_total);
                    }
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketLoginAcknowledgment, descriptor,
                                               mes_id, downlink_freq_mhz, network_version, les_total, stations)
            };

            // PacketLogoutAcknowledgment packet
            struct PacketLogoutAcknowledgment : public PacketBase
            {
                static const uint8_t FRM_ID = 0x13;

                uint32_t mes_id;

                PacketLogoutAcknowledgment() {}
                PacketLogoutAcknowledgment(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketLogoutAcknowledgment, descriptor, mes_id)
            };

            // Message Data packet
            struct PacketMessageData : public PacketBase
            {
                static const uint8_t FRM_ID = 0x2a;

                uint8_t sat_id;
                uint8_t les_id;
                uint8_t logical_channel_number;
                uint8_t packet_sequence_number;
                std::vector<uint8_t> data;

                std::string sat_name;
                std::string les_name;
                std::string message;

                PacketMessageData() {}
                PacketMessageData(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    sat_id = pkt[2] >> 6 & 0x03;
                    les_id = pkt[2] & 0x3F;
                    logical_channel_number = pkt[3];
                    packet_sequence_number = pkt[4];

                    data = std::vector<uint8_t>(descriptor.length - 2 - 4);
                    memcpy(data.data(), &pkt[5], descriptor.length - 2 - 5);

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                    message = message_to_string(data, is_binary(data, true) ? 7 : 0, false);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketMessageData, descriptor,
                                               sat_id, les_id, logical_channel_number, packet_sequence_number, data, sat_name, les_name, message);
            };

            // Message Status packet
            struct PacketMessageStatus : public PacketBase
            {
                static const uint8_t FRM_ID = 0x29;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint32_t message_reference_number;

                uint8_t descriptor_length;
                bool status;
                uint8_t attempts_number;
                std::string non_delivery_code;
                std::string address_information;

                std::string sat_name;
                std::string les_name;

                PacketMessageStatus() {}
                PacketMessageStatus(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    message_reference_number = pkt[6] << 16 | pkt[7] << 8 | pkt[8];

                    descriptor_length = pkt[9];
                    status = pkt[10] >> 7;
                    attempts_number = pkt[10] & 0b1111111;
                    non_delivery_code = string_from_ia5(&pkt[11], 3);
                    address_information = string_from_ia5(&pkt[14], descriptor_length - 5);

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketMessageStatus, descriptor,
                                               mes_id, sat_id, les_id, message_reference_number, descriptor_length, status, attempts_number, non_delivery_code, address_information,
                                               sat_name, les_name)
            };

            // PacketNetworkUpdate packet
            struct PacketNetworkUpdate : public PacketBase
            {
                static const uint8_t FRM_ID = 0x2B;

                uint8_t network_version;
                uint8_t les_total;
                nlohmann::json stations;

                PacketNetworkUpdate() {}
                PacketNetworkUpdate(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    network_version = pkt[2];
                    les_total = pkt[3];
                    stations = get_stations(&pkt[4], les_total);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketNetworkUpdate, descriptor,
                                               network_version, les_total, stations)
            };

            // Request Status packet
            struct PacketRequestStatus : public PacketBase
            {
                static const uint8_t FRM_ID = 0x2c;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                bool pending_reject_flag_b;
                uint8_t request_status_code_b;

                std::string sat_name;
                std::string les_name;

                std::string pending_reject_flag;
                std::string request_status_code;

                PacketRequestStatus() {}
                PacketRequestStatus(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    pending_reject_flag_b = pkt[6] >> 7;
                    request_status_code_b = pkt[6] & 0b1111111;

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);

                    if (pending_reject_flag_b == 0)
                        pending_reject_flag = "Pending";
                    else
                        pending_reject_flag = "Rejected";

                    if (request_status_code_b == 1)
                        request_status_code = "LES Message Store Full";
                    else if (request_status_code_b == 2)
                        request_status_code = "Requested Destination Not Served";
                    else if (request_status_code_b == 3)
                        request_status_code = "Satellite Congestion";
                    else if (request_status_code_b == 4)
                        request_status_code = "Terrestrial Congestion";
                    else if (request_status_code_b == 5)
                        request_status_code = "Requested Service Not Provided";
                    else if (request_status_code_b == 6)
                        request_status_code = "Request In Queue";
                    else if (request_status_code_b == 7)
                        request_status_code = "Request Barred";
                    else if (request_status_code_b == 8)
                        request_status_code = "MES Not Logged In";
                    else if (request_status_code_b == 9)
                        request_status_code = "MES Not Commissioned";
                    else if (request_status_code_b == 0xa)
                        request_status_code = "Waiting TDM Assignement";
                    else if (request_status_code_b == 0xb)
                        request_status_code = "Illegal Request";
                    else if (request_status_code_b == 0xc)
                        request_status_code = "LES Not In Service";
                    else if (request_status_code_b == 0xd)
                        request_status_code = "Requested Service Temporarily Unavailable";
                    else if (request_status_code_b == 0xe)
                        request_status_code = "Acces To Requested Service Denied";
                    else if (request_status_code_b == 0xf)
                        request_status_code = "Invalid Service";
                    else if (request_status_code_b == 0x10)
                        request_status_code = "Invalid Address";
                    else if (request_status_code_b == 0x15)
                        request_status_code = "PTSN Modem Type Not Supported";
                    else if (request_status_code_b == 0x11)
                        request_status_code = "Unable To Decode: Specified Dictionary Version Not Available";
                    else if (request_status_code_b == 0x12)
                        request_status_code = "IWU Number Is Invalid";
                    else if (request_status_code_b == 0x13)
                        request_status_code = "MES Has Not Subscribed To This Service";
                    else if (request_status_code_b == 0x14)
                        request_status_code = "Protocol Version Not Supported";
                    else if (request_status_code_b == 0x16)
                        request_status_code = "Unrecognized PDE Type";
                    else
                        request_status_code = "Unknown";
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketRequestStatus, descriptor,
                                               mes_id, sat_id, les_id, pending_reject_flag_b, request_status_code_b,
                                               sat_name, les_name, pending_reject_flag, request_status_code)
            };

            // Test Result packet
            struct PacketTestResult : public PacketBase
            {
                static const uint8_t FRM_ID = 0x2d;

                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;

                uint8_t attempts_b;
                uint8_t bber_b;
                uint8_t foward_attempts;
                uint8_t return_attempts;
                uint8_t distress_alert_test_b;
                uint8_t signal_strength_b;
                uint8_t overall_result_b;

                std::string sat_name;
                std::string les_name;

                std::string attempts;
                std::string bber;
                std::string distress_alert_test;
                std::string signal_strength;
                std::string overall_result;

                PacketTestResult() {}
                PacketTestResult(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;

                    attempts_b = pkt[6] >> 6 & 0b11;
                    bber_b = pkt[6] >> 4 & 0b11;
                    foward_attempts = pkt[6] >> 2 & 0b11;
                    return_attempts = pkt[6] & 0b11;
                    distress_alert_test_b = pkt[7] >> 4;
                    signal_strength_b = pkt[7] & 0xF;
                    overall_result_b = pkt[8] >> 4;

                    if (attempts_b == 0)
                        attempts = "Third Attempt Failed";
                    else if (attempts_b == 1)
                        attempts = "First Attempt";
                    else if (attempts_b == 2)
                        attempts = "Second Attempt";
                    else if (attempts_b == 3)
                        attempts = "Third Attempt";

                    if (bber_b == 1)
                        bber = "Pass";
                    else if (attempts_b == 2)
                        bber = "Fail Attempt";
                    else
                        bber = "Third Attempt";

                    if (distress_alert_test_b == 0)
                        distress_alert_test = "No Response";
                    else if (distress_alert_test_b == 1)
                        distress_alert_test = "Not Applicable";
                    else if (distress_alert_test_b == 2)
                        distress_alert_test = "Test OK";
                    else if (distress_alert_test_b == 3)
                        distress_alert_test = "Nature Of Distress Not Default";
                    else if (distress_alert_test_b == 4)
                        distress_alert_test = "Null Data";
                    else if (distress_alert_test_b == 5)
                        distress_alert_test = "Incorrect Protocol";
                    else if (distress_alert_test_b == 6)
                        distress_alert_test = "Invalid Data Format";
                    else if (distress_alert_test_b == 7)
                        distress_alert_test = "Automatically Activated";
                    else
                        distress_alert_test = "Unknown";

                    if (signal_strength_b == 0)
                        signal_strength = "Unreadable";
                    else if (signal_strength_b == 1)
                        signal_strength = "Less than X dB";
                    else if (signal_strength_b == 2)
                        signal_strength = "Over or at X dB";
                    else if (signal_strength_b == 3)
                        signal_strength = "Over X +3 dB";
                    else if (signal_strength_b == 4)
                        signal_strength = "Over X +6 dB";
                    else if (signal_strength_b == 5)
                        signal_strength = "Over X +10 dB";
                    else if (signal_strength_b == 6)
                        signal_strength = "Over X +13 dB";
                    else if (signal_strength_b == 7)
                        signal_strength = "Over X +16 dB";

                    if (overall_result_b == 0)
                        overall_result = "Applicable Tests Pass";
                    else if (overall_result_b == 1)
                        overall_result = "Applicable Tests Pass";
                    else if (overall_result_b == 2)
                        overall_result = "Applicable Tests Pass";
                    else if (overall_result_b == 3)
                        overall_result = "Applicable Tests Pass";
                    else if (overall_result_b == 4)
                        overall_result = "Forward Message Transfer Fail";
                    else if (overall_result_b == 5)
                        overall_result = "Return Message Transfer Fail";
                    else if (overall_result_b == 6)
                        overall_result = "Signal Unreadable";
                    else if (overall_result_b == 7)
                        overall_result = "Signal Level Excessive";
                    else if (overall_result_b == 8)
                        overall_result = "Distress Alert Test Fail";
                    else if (overall_result_b == 9)
                        overall_result = "Unspecified Fail";
                    else
                        overall_result = "Unknown";

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketTestResult, descriptor,
                                               mes_id, sat_id, les_id, attempts_b, bber_b, foward_attempts, return_attempts, distress_alert_test_b, signal_strength_b, overall_result_b,
                                               sat_name, les_name, attempts, bber, distress_alert_test, signal_strength, overall_result)
            };

            // Network Monitor packet
            struct PacketNetworkMonitor : public PacketBase
            {
                static const uint8_t FRM_ID = 0x05;

                uint8_t type;
                uint32_t mes_id;
                uint8_t sat_id;
                uint8_t les_id;
                uint8_t logical_channel_number;
                uint32_t message_reference_number;
                uint8_t number_of_message_packets;
                uint8_t presentation_control;
                uint8_t last_count;

                std::string sat_name;
                std::string les_name;

                PacketNetworkMonitor() {}
                PacketNetworkMonitor(uint8_t *pkt, int len_max) : PacketBase(pkt, len_max)
                {
                    type = pkt[1] >> 4;
                    mes_id = pkt[2] << 16 | pkt[3] << 8 | pkt[4];
                    sat_id = pkt[5] >> 6 & 0x03;
                    les_id = pkt[5] & 0x3F;
                    logical_channel_number = pkt[6];
                    message_reference_number = pkt[7] << 16 | pkt[8] << 8 | pkt[9];
                    number_of_message_packets = pkt[10];
                    presentation_control = pkt[11];
                    last_count = pkt[12];

                    sat_name = get_sat_name(sat_id);
                    les_name = get_les_name(sat_id, les_id);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketNetworkMonitor, descriptor,
                                               type, mes_id, sat_id, les_id, logical_channel_number, message_reference_number, number_of_message_packets, presentation_control, last_count,
                                               sat_name, les_name)
            };
        }
    }
}