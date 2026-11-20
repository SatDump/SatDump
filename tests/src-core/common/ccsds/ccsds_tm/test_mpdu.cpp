#include "common/ccsds/ccsds_tm/mpdu.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

TEST_CASE("ccsds_tm parseMPDU reads the FHP from the data field status word", "[common][ccsds][tm]")
{
    std::vector<uint8_t> cadu(32, 0);

    SECTION("flag bits above the FHP are ignored")
    {
        cadu[8] = 0xF8 | 0x05; // sec hdr, sync, order, segment length id + FHP high bits
        cadu[9] = 0x5A;
        auto mpdu = ccsds::ccsds_tm::parseMPDU(cadu.data());
        CHECK(mpdu.first_header_pointer == 0x55A);
        CHECK(mpdu.data == &cadu[10]);
    }

    SECTION("idle and no-header markers")
    {
        cadu[8] = 0x07;
        cadu[9] = 0xFE;
        CHECK(ccsds::ccsds_tm::parseMPDU(cadu.data()).first_header_pointer == 2046);
        cadu[9] = 0xFF;
        CHECK(ccsds::ccsds_tm::parseMPDU(cadu.data()).first_header_pointer == 2047);
    }
}

TEST_CASE("ccsds_tm parseMPDU handles the secondary header and M_PDU insert zones", "[common][ccsds][tm]")
{
    std::vector<uint8_t> cadu(40, 0);
    cadu[8] = 0xFF; // would be read as the FHP if the zone were not skipped
    cadu[9] = 0xFF;
    cadu[12] = 0x01;
    cadu[13] = 0x02;

    SECTION("frame secondary header shifts both FHP and data")
    {
        auto mpdu = ccsds::ccsds_tm::parseMPDU(cadu.data(), true, 4);
        CHECK(mpdu.first_header_pointer == 0x102);
        CHECK(mpdu.data == &cadu[14]);
    }

    SECTION("M_PDU insert zone shifts only the data")
    {
        auto mpdu = ccsds::ccsds_tm::parseMPDU(cadu.data(), true, 4, 6);
        CHECK(mpdu.first_header_pointer == 0x102);
        CHECK(mpdu.data == &cadu[20]);

        auto plain = ccsds::ccsds_tm::parseMPDU(cadu.data(), false, 4, 3);
        CHECK(plain.first_header_pointer == 0x7FF);
        CHECK(plain.data == &cadu[13]);
    }

    SECTION("default insert zone size is 2")
    {
        cadu[10] = 0x03;
        cadu[11] = 0x33;
        auto mpdu = ccsds::ccsds_tm::parseMPDU(cadu.data(), true);
        CHECK(mpdu.first_header_pointer == 0x333);
        CHECK(mpdu.data == &cadu[12]);
    }
}
