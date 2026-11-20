#include "common/big_endian.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>

TEST_CASE("big endian types have packed sizes", "[common][big_endian]")
{
    CHECK(sizeof(be_uint16_t) == 2);
    CHECK(sizeof(be_uint32_t) == 4);
    CHECK(sizeof(be_uint48_t) == 6);
    CHECK(sizeof(be_uint64_t) == 8);
}

TEST_CASE("be_uint16_t stores big endian and converts back", "[common][big_endian]")
{
    be_uint16_t v = uint16_t(0x1234);
    uint8_t raw[2];
    memcpy(raw, &v, 2);
    CHECK(raw[0] == 0x12);
    CHECK(raw[1] == 0x34);
    CHECK(uint16_t(v) == 0x1234);

    be_uint16_t def;
    CHECK(uint16_t(def) == 0);

    // Reading from a raw big endian buffer
    const uint8_t buf[2] = {0xBE, 0xEF};
    be_uint16_t r;
    memcpy(&r, buf, 2);
    CHECK(uint16_t(r) == 0xBEEF);
}

TEST_CASE("be_uint32_t stores big endian and converts back", "[common][big_endian]")
{
    be_uint32_t v = uint32_t(0xDEADBEEF);
    uint8_t raw[4];
    memcpy(raw, &v, 4);
    CHECK(raw[0] == 0xDE);
    CHECK(raw[1] == 0xAD);
    CHECK(raw[2] == 0xBE);
    CHECK(raw[3] == 0xEF);
    CHECK(uint32_t(v) == 0xDEADBEEF);

    be_uint32_t def;
    CHECK(uint32_t(def) == 0);

    const uint8_t buf[4] = {0x01, 0x02, 0x03, 0x04};
    be_uint32_t r;
    memcpy(&r, buf, 4);
    CHECK(uint32_t(r) == 0x01020304);
}

TEST_CASE("be_uint48_t decodes six big endian bytes", "[common][big_endian]")
{
    const uint8_t buf[6] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
    be_uint48_t v;
    memcpy(&v, buf, 6);
    CHECK(v.value() == 0x0123456789ABULL);
    CHECK(uint64_t(v) == 0x0123456789ABULL);

    const uint8_t maxbuf[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(&v, maxbuf, 6);
    CHECK(v.value() == 0xFFFFFFFFFFFFULL);
}

// BUG: be_uint64_t uses htonl/ntohl, which operate on 32-bit values, so the
// upper 32 bits are dropped and the in-memory layout is not big endian.
TEST_CASE("be_uint64_t stores big endian and converts back", "[common][big_endian][!shouldfail]")
{
    be_uint64_t v = uint64_t(0x0102030405060708ULL);
    CHECK(uint64_t(v) == 0x0102030405060708ULL);

    be_uint64_t small = uint64_t(5);
    uint8_t raw[8];
    memcpy(raw, &small, 8);
    const uint8_t expected[8] = {0, 0, 0, 0, 0, 0, 0, 5};
    CHECK(memcmp(raw, expected, 8) == 0);
}
