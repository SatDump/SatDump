#include "common/ccsds/ccsds_tm/vcdu.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace
{
    // TM Transfer Frame primary header (CCSDS 132.0-B) after a 4-byte ASM:
    // version(2) SCID(10) VCID(3) OCF(1) | MC count(8) | VC count(8) | data field status(16)
    void writeTmHeader(uint8_t *cadu, uint8_t version, uint16_t scid, uint8_t vcid, bool ocf, uint8_t mcfc, uint8_t vcfc, uint16_t status)
    {
        cadu[0] = 0x1A;
        cadu[1] = 0xCF;
        cadu[2] = 0xFC;
        cadu[3] = 0x1D;
        cadu[4] = (version & 0b11) << 6 | ((scid >> 4) & 0x3F);
        cadu[5] = (scid & 0xF) << 4 | (vcid & 0b111) << 1 | ocf;
        cadu[6] = mcfc;
        cadu[7] = vcfc;
        cadu[8] = status >> 8;
        cadu[9] = status & 0xFF;
    }
} // namespace

TEST_CASE("ccsds_tm parseVCDU decodes version, 10-bit SCID and 3-bit VCID", "[common][ccsds][tm]")
{
    uint8_t cadu[16] = {};
    writeTmHeader(cadu, 0, 0x2A5, 6, true, 0x12, 0x34, 0x1800 | 0x123);

    auto vcdu = ccsds::ccsds_tm::parseVCDU(cadu);
    CHECK(vcdu.version == 0);
    CHECK(vcdu.spacecraft_id == 0x2A5);
    CHECK(vcdu.vcid == 6);

    // TM has no 24-bit frame counter or replay flag: these follow the AOS byte
    // layout, so they mix the MC/VC counts with the data field status word.
    CHECK(vcdu.vcdu_counter == (0x12u << 16 | 0x34u << 8 | 0x19u));
    CHECK(vcdu.replay_flag == false);
}

TEST_CASE("ccsds_tm parseVCDU keeps SCID and VCID apart from the OCF flag", "[common][ccsds][tm]")
{
    uint8_t cadu[16] = {};
    for (int vcid = 0; vcid < 8; vcid++)
    {
        for (uint16_t scid : {0, 1, 0x155, 0x3FF})
        {
            for (bool ocf : {false, true})
            {
                writeTmHeader(cadu, 1, scid, vcid, ocf, 0, 0, 0);
                auto vcdu = ccsds::ccsds_tm::parseVCDU(cadu);
                INFO("vcid " << vcid << " scid " << scid << " ocf " << ocf);
                REQUIRE(vcdu.version == 1);
                REQUIRE(vcdu.spacecraft_id == scid);
                REQUIRE(vcdu.vcid == vcid);
            }
        }
    }
}

TEST_CASE("ccsds_tm parseVCDU reads the version from the top two bits", "[common][ccsds][tm]")
{
    uint8_t cadu[16] = {};
    writeTmHeader(cadu, 3, 0, 0, false, 0, 0, 0);
    CHECK(ccsds::ccsds_tm::parseVCDU(cadu).version == 3);
    CHECK(ccsds::ccsds_tm::parseVCDU(cadu).spacecraft_id == 0);
}
