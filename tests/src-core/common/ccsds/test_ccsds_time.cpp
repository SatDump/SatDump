#include "common/ccsds/ccsds_time.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    // CCSDS Day Segmented (CDS) time code: 16-bit day, 32-bit ms of day, 16-bit us of ms
    std::vector<uint8_t> makeCds(uint16_t days, uint32_t ms, uint16_t us)
    {
        return {(uint8_t)(days >> 8), (uint8_t)days, (uint8_t)(ms >> 24), (uint8_t)(ms >> 16), (uint8_t)(ms >> 8), (uint8_t)ms, (uint8_t)(us >> 8), (uint8_t)us};
    }

    ccsds::CCSDSPacket packetWithPayload(const std::vector<uint8_t> &payload)
    {
        ccsds::CCSDSPacket pkt;
        pkt.payload = payload;
        return pkt;
    }

    // Days between the CCSDS epoch (1958-01-01) and the Unix epoch, as passed by decoders
    const int EPOCH_1958_TO_UNIX = -4383;
} // namespace

TEST_CASE("parseCCSDSTime converts CDS days and milliseconds to seconds", "[common][ccsds][time]")
{
    SECTION("zero time with zero offset")
    {
        ccsds::CCSDSPacket pkt = packetWithPayload(makeCds(0, 0, 0));
        CHECK(ccsds::parseCCSDSTime(pkt) == 0);
    }

    SECTION("noon of day 1")
    {
        ccsds::CCSDSPacket pkt = packetWithPayload(makeCds(1, 43200000, 0));
        CHECK(ccsds::parseCCSDSTime(pkt) == 86400 + 43200);
    }

    SECTION("milliseconds are truncated to whole seconds")
    {
        ccsds::CCSDSPacket pkt = packetWithPayload(makeCds(0, 1999, 999));
        CHECK(ccsds::parseCCSDSTime(pkt) == 1);
    }

    SECTION("1958 epoch offset gives Unix time")
    {
        // 2020-01-01T00:00:00Z is Unix 1577836800 = 18262 days, so 18262 + 4383 days since 1958
        ccsds::CCSDSPacket pkt = packetWithPayload(makeCds(18262 + 4383, 0, 0));
        CHECK(ccsds::parseCCSDSTime(pkt, EPOCH_1958_TO_UNIX) == 1577836800);
    }

    SECTION("custom ms scale, e.g. a counter of 1/256 s units")
    {
        ccsds::CCSDSPacket pkt = packetWithPayload(makeCds(2, 256 * 10, 0));
        CHECK(ccsds::parseCCSDSTime(pkt, 0, 256) == 2 * 86400 + 10);
    }
}

TEST_CASE("parseCCSDSTimeFull includes the microsecond field", "[common][ccsds][time]")
{
    ccsds::CCSDSPacket pkt = packetWithPayload(makeCds(10, 3723456, 789)); // 01:02:03.456789
    double t = ccsds::parseCCSDSTimeFull(pkt, 0);
    CHECK(t == Catch::Approx(10 * 86400.0 + 3723.456789).margin(1e-7));

    SECTION("offset is applied in days")
    {
        double withOffset = ccsds::parseCCSDSTimeFull(pkt, -10);
        CHECK(withOffset == Catch::Approx(3723.456789).margin(1e-7));
    }

    SECTION("custom scales")
    {
        // ms field in units of 1/10 s, sub-field in units of 1/1000 s
        ccsds::CCSDSPacket scaled = packetWithPayload(makeCds(0, 25, 500));
        CHECK(ccsds::parseCCSDSTimeFull(scaled, 0, 10, 1000) == Catch::Approx(2.5 + 0.5));
    }

    SECTION("maximum field values do not overflow")
    {
        ccsds::CCSDSPacket maxed = packetWithPayload(makeCds(0xFFFF, 0xFFFFFFFF, 0xFFFF));
        double expected = 65535.0 * 86400.0 + 4294967295.0 / 1000.0 + 65535.0 / 1000000.0;
        CHECK(ccsds::parseCCSDSTimeFull(maxed, 0) == Catch::Approx(expected));
    }

    SECTION("dates after 2038 are representable")
    {
        // 2040-01-01 is Unix 2208988800 = 25567 days
        ccsds::CCSDSPacket late = packetWithPayload(makeCds(25567 + 4383, 0, 0));
        CHECK(ccsds::parseCCSDSTimeFull(late, EPOCH_1958_TO_UNIX) == Catch::Approx(2208988800.0));
    }
}

TEST_CASE("parseCCSDSTimeFullRaw matches parseCCSDSTimeFull on the same bytes", "[common][ccsds][time]")
{
    std::vector<uint8_t> raw = makeCds(21000, 86399999, 999);
    ccsds::CCSDSPacket pkt = packetWithPayload(raw);

    CHECK(ccsds::parseCCSDSTimeFullRaw(raw.data(), EPOCH_1958_TO_UNIX) == ccsds::parseCCSDSTimeFull(pkt, EPOCH_1958_TO_UNIX));
    CHECK(ccsds::parseCCSDSTimeFullRaw(raw.data(), 0) == Catch::Approx(21000 * 86400.0 + 86399.999999).margin(1e-6));
    CHECK(ccsds::parseCCSDSTimeFullRaw(raw.data(), 0, 100, 10000) == Catch::Approx(21000 * 86400.0 + 863999.99 + 0.0999).margin(1e-6));
}

TEST_CASE("parseCCSDSTimeFullRawUnsegmented subtracts the leap second field", "[common][ccsds][time]")
{
    // byte 1 low 7 bits: seconds to subtract, bytes 2-5: seconds, bytes 6-7: subseconds
    uint8_t data[8] = {0xAA, 0x80 | 37, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00};
    double t = ccsds::parseCCSDSTimeFullRawUnsegmented(data, 0, 1.0 / 65536.0);
    CHECK(t == Catch::Approx(65536.0 - 37.0 + 0.5));

    SECTION("byte 0 and the top bit of byte 1 are ignored")
    {
        uint8_t other[8] = {0x00, 37, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00};
        CHECK(ccsds::parseCCSDSTimeFullRawUnsegmented(other, 0, 1.0 / 65536.0) == t);
    }

    SECTION("offset is in days") { CHECK(ccsds::parseCCSDSTimeFullRawUnsegmented(data, 2, 1.0 / 65536.0) == Catch::Approx(t + 2 * 86400.0)); }

    SECTION("zero")
    {
        uint8_t zero[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        CHECK(ccsds::parseCCSDSTimeFullRawUnsegmented(zero, 0, 1e-3) == 0.0);
    }
}
