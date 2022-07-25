#pragma once

#include <cstdint>

namespace mpeg_ts
{
    struct MPEHeader
    {
        uint8_t table_id;
        bool section_syntax_indicator;
        bool private_indicator;
        uint16_t section_length;
        uint8_t mac_address_6;
        uint8_t mac_address_5;
        uint8_t payload_scrambling_control;
        uint8_t address_scrambling_control;
        bool llc_snap_flag;
        bool current_next_indicator;
        uint8_t section_number;
        uint8_t last_section_number;
        uint8_t mac_address_4;
        uint8_t mac_address_3;
        uint8_t mac_address_2;
        uint8_t mac_address_1;

        void parse(uint8_t *frm);
    };

    struct IPv4Header
    {
        uint8_t version;
        uint8_t ihl;
        uint8_t dscp;
        uint8_t ecn;
        uint16_t total_length;
        uint16_t idenditication;
        uint8_t flags;
        uint16_t fragment_offset;
        uint8_t time_to_live;
        uint8_t protocol;
        uint16_t header_checksum;
        uint8_t source_ip_1;
        uint8_t source_ip_2;
        uint8_t source_ip_3;
        uint8_t source_ip_4;
        uint8_t target_ip_1;
        uint8_t target_ip_2;
        uint8_t target_ip_3;
        uint8_t target_ip_4;

        void parse(uint8_t *frm);
    };
};