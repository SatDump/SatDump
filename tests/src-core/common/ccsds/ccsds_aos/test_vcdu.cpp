#include "common/ccsds/ccsds_aos/vcdu.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace
{
    // AOS Transfer Frame primary header (CCSDS 732.0-B) after a 4-byte ASM:
    // version(2) SCID(8) VCID(6) | VC frame count(24) | replay(1) usage(1) spare(2) cycle(4)
    void writeAosHeader(uint8_t *cadu, uint8_t version, uint8_t scid, uint8_t vcid, uint32_t counter, bool replay, uint8_t lowSignalling = 0)
    {
        cadu[0] = 0x1A;
        cadu[1] = 0xCF;
        cadu[2] = 0xFC;
        cadu[3] = 0x1D;
        cadu[4] = (version & 0b11) << 6 | scid >> 2;
        cadu[5] = (scid & 0b11) << 6 | (vcid & 0x3F);
        cadu[6] = counter >> 16;
        cadu[7] = counter >> 8;
        cadu[8] = counter;
        cadu[9] = replay << 7 | (lowSignalling & 0x7F);
    }
} // namespace

TEST_CASE("ccsds_aos parseVCDU decodes the AOS primary header", "[common][ccsds][aos]")
{
    uint8_t cadu[16] = {};
    writeAosHeader(cadu, 1, 0x9D, 63, 0xABCDEF, true);

    ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);
    CHECK(vcdu.version == 1);
    CHECK(vcdu.spacecraft_id == 0x9D);
    CHECK(vcdu.vcid == 63);
    CHECK(vcdu.vcdu_counter == 0xABCDEF);
    CHECK(vcdu.replay_flag == true);
}

TEST_CASE("ccsds_aos parseVCDU keeps fields independent", "[common][ccsds][aos]")
{
    uint8_t cadu[16] = {};

    SECTION("only the SCID")
    {
        writeAosHeader(cadu, 0, 0xFF, 0, 0, false);
        auto vcdu = ccsds::ccsds_aos::parseVCDU(cadu);
        CHECK(vcdu.version == 0);
        CHECK(vcdu.spacecraft_id == 0xFF);
        CHECK(vcdu.vcid == 0);
        CHECK(vcdu.vcdu_counter == 0);
        CHECK(vcdu.replay_flag == false);
    }

    SECTION("only the VCID")
    {
        writeAosHeader(cadu, 0, 0, 0x2A, 0, false);
        auto vcdu = ccsds::ccsds_aos::parseVCDU(cadu);
        CHECK(vcdu.spacecraft_id == 0);
        CHECK(vcdu.vcid == 0x2A);
    }

    SECTION("only the counter, max value")
    {
        writeAosHeader(cadu, 0, 0, 0, 0xFFFFFF, false);
        auto vcdu = ccsds::ccsds_aos::parseVCDU(cadu);
        CHECK(vcdu.vcdu_counter == 0xFFFFFF);
        CHECK(vcdu.vcid == 0);
        CHECK(vcdu.replay_flag == false);
    }

    SECTION("signalling bits other than replay are ignored")
    {
        writeAosHeader(cadu, 1, 0x10, 5, 1, false, 0x7F);
        auto vcdu = ccsds::ccsds_aos::parseVCDU(cadu);
        CHECK(vcdu.replay_flag == false);
        CHECK(vcdu.vcdu_counter == 1);
    }

    SECTION("version 3")
    {
        writeAosHeader(cadu, 3, 0, 0, 0, false);
        CHECK(ccsds::ccsds_aos::parseVCDU(cadu).version == 3);
        CHECK(ccsds::ccsds_aos::parseVCDU(cadu).spacecraft_id == 0);
    }
}

TEST_CASE("ccsds_aos parseVCDU handles every VCID", "[common][ccsds][aos]")
{
    uint8_t cadu[16] = {};
    for (int vcid = 0; vcid < 64; vcid++)
    {
        writeAosHeader(cadu, 1, 0x55, vcid, vcid * 1000, vcid & 1);
        auto vcdu = ccsds::ccsds_aos::parseVCDU(cadu);
        INFO("vcid " << vcid);
        REQUIRE(vcdu.vcid == vcid);
        REQUIRE(vcdu.spacecraft_id == 0x55);
        REQUIRE(vcdu.vcdu_counter == (uint32_t)vcid * 1000);
        REQUIRE(vcdu.replay_flag == (bool)(vcid & 1));
    }
}
