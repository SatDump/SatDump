#include "common/ccsds/ccsds_aos/mpdu.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

TEST_CASE("ccsds_aos parseMPDU reads the 11-bit first header pointer after the primary header", "[common][ccsds][aos]")
{
    std::vector<uint8_t> cadu(32, 0);

    SECTION("regular value, spare bits set are ignored")
    {
        cadu[10] = 0xF8 | 0x03; // 5 spare bits + FHP high bits
        cadu[11] = 0x74;
        auto mpdu = ccsds::ccsds_aos::parseMPDU(cadu.data());
        CHECK(mpdu.first_header_pointer == 0x374);
        CHECK(mpdu.data == &cadu[12]);
    }

    SECTION("no header pointer")
    {
        cadu[10] = 0x07;
        cadu[11] = 0xFF;
        CHECK(ccsds::ccsds_aos::parseMPDU(cadu.data()).first_header_pointer == 2047);
    }

    SECTION("zero")
    {
        CHECK(ccsds::ccsds_aos::parseMPDU(cadu.data()).first_header_pointer == 0);
    }
}

TEST_CASE("ccsds_aos parseMPDU skips the insert zone when present", "[common][ccsds][aos]")
{
    std::vector<uint8_t> cadu(32, 0xFF);
    cadu[10] = 0x00; // insert zone, must not be read as the FHP
    cadu[11] = 0x00;
    cadu[12] = 0x00;
    cadu[13] = 0x00;
    cadu[14] = 0x01;
    cadu[15] = 0x23;

    auto mpdu = ccsds::ccsds_aos::parseMPDU(cadu.data(), true, 4);
    CHECK(mpdu.first_header_pointer == 0x123);
    CHECK(mpdu.data == &cadu[16]);

    // Default insert zone size is 2
    cadu[12] = 0x02;
    cadu[13] = 0x22;
    auto def = ccsds::ccsds_aos::parseMPDU(cadu.data(), true);
    CHECK(def.first_header_pointer == 0x222);
    CHECK(def.data == &cadu[14]);

    // Insert zone flag off ignores the size
    auto off = ccsds::ccsds_aos::parseMPDU(cadu.data(), false, 4);
    CHECK(off.first_header_pointer == 0);
    CHECK(off.data == &cadu[12]);
}
