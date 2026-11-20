#include "common/mpeg_ts/ts_header.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("mpeg_ts::TSHeader parses all header fields", "[common][mpeg_ts][ts_header]")
{
    // sync, TEI=1 PUSI=0 TP=1 PID=0x1ABC, TSC=0b10 AFC=0b11 CC=0x5
    uint8_t pkt[4] = {0x47, 0b10111010, 0xBC, 0b10110101};
    mpeg_ts::TSHeader h;
    h.parse(pkt);

    CHECK(h.sync == 0x47);
    CHECK(h.tei == true);
    CHECK(h.pusi == false);
    CHECK(h.tp == true);
    CHECK(h.pid == 0x1ABC);
    CHECK(h.tsc == 0b10);
    CHECK(h.afc == 0b11);
    CHECK(h.cont == 0x5);
}

TEST_CASE("mpeg_ts::TSHeader parses PUSI, null PID and boundaries", "[common][mpeg_ts][ts_header]")
{
    uint8_t pkt[4] = {0x47, 0x5F, 0xFF, 0x1F};
    mpeg_ts::TSHeader h;
    h.parse(pkt);

    CHECK(h.tei == false);
    CHECK(h.pusi == true);
    CHECK(h.tp == false);
    CHECK(h.pid == 0x1FFF);
    CHECK(h.tsc == 0);
    CHECK(h.afc == 0b01);
    CHECK(h.cont == 0xF);

    uint8_t zero[4] = {0, 0, 0, 0};
    h.parse(zero);
    CHECK(h.sync == 0);
    CHECK(h.pid == 0);
    CHECK(h.afc == 0);
    CHECK(h.cont == 0);
}
