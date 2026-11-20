#include "common/mpeg_ts/dvb_mpe.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("mpeg_ts::MPEHeader parses a DSM-CC datagram section header", "[common][mpeg_ts][dvb_mpe]")
{
    uint8_t frm[12] = {
        0x3E,                         // table_id
        0b10110001,                   // SSI=1, private=0, reserved, length high nibble = 1
        0x23,                         // length low
        0x66,       0x55,             // MAC 6, 5
        0b11100111,                   // reserved, payload scrambling=0b10, address scrambling=0b01, LLC=1, current_next=1
        0x02,       0x03,             // section numbers
        0x44,       0x33, 0x22, 0x11, // MAC 4..1
    };
    mpeg_ts::MPEHeader h;
    h.parse(frm);

    CHECK(h.table_id == 0x3E);
    CHECK(h.section_syntax_indicator == true);
    CHECK(h.private_indicator == false);
    CHECK(h.section_length == 0x123);
    CHECK(h.payload_scrambling_control == 0b10);
    CHECK(h.address_scrambling_control == 0b01);
    CHECK(h.llc_snap_flag == true);
    CHECK(h.current_next_indicator == true);
    CHECK(h.section_number == 2);
    CHECK(h.last_section_number == 3);
    CHECK(h.mac_address_1 == 0x11);
    CHECK(h.mac_address_2 == 0x22);
    CHECK(h.mac_address_3 == 0x33);
    CHECK(h.mac_address_4 == 0x44);
    CHECK(h.mac_address_5 == 0x55);
    CHECK(h.mac_address_6 == 0x66);
}

namespace
{
    // Standard 20 byte IPv4 header (plus padding, the parser reads byte 22)
    // 45 00 0054 1c46 4000 40 01 b1e6 c0a80001 c0a800c7
    uint8_t ipv4_frame[24] = {0x45, 0x00, 0x00, 0x54, 0x1c, 0x46, 0x40, 0x00, 0x40, 0x01, 0xb1, 0xe6, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7, 0x00, 0x00, 0x00, 0x00};
} // namespace

TEST_CASE("mpeg_ts::IPv4Header parses version and addresses", "[common][mpeg_ts][dvb_mpe]")
{
    mpeg_ts::IPv4Header h;
    h.parse(ipv4_frame);

    CHECK(h.version == 4);
    CHECK(h.source_ip_1 == 192);
    CHECK(h.source_ip_2 == 168);
    CHECK(h.source_ip_3 == 0);
    CHECK(h.source_ip_4 == 1);
    CHECK(h.target_ip_1 == 192);
    CHECK(h.target_ip_2 == 168);
    CHECK(h.target_ip_3 == 0);
    CHECK(h.target_ip_4 == 199);
}

// Bug: dvb_mpe.cpp:28-38 - IPv4Header::parse reads IHL..protocol one byte too
// late (e.g. IHL from byte 1, total length from bytes 3-4) and the checksum
// from bytes 22 and 11 instead of bytes 10-11.
TEST_CASE("mpeg_ts::IPv4Header parses the remaining header fields", "[common][mpeg_ts][dvb_mpe][!shouldfail]")
{
    mpeg_ts::IPv4Header h;
    h.parse(ipv4_frame);

    CHECK(h.ihl == 5);
    CHECK(h.dscp == 0);
    CHECK(h.ecn == 0);
    CHECK(h.total_length == 0x54);
    CHECK(h.idenditication == 0x1c46);
    CHECK(h.flags == 0b010);
    CHECK(h.fragment_offset == 0);
    CHECK(h.time_to_live == 64);
    CHECK(h.protocol == 1);
    CHECK(h.header_checksum == 0xb1e6);
}
