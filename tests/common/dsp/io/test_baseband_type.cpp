#include "common/dsp/io/baseband_type.h"
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("BasebandType defaults to cf32", "[common][dsp][baseband_type]")
{
    dsp::BasebandType t;
    CHECK(t == dsp::CF_32);
    CHECK((std::string)t == "cf32");
}

TEST_CASE("BasebandType string round trip", "[common][dsp][baseband_type]")
{
    struct Case
    {
        dsp::BasebandTypeEnum e;
        const char *name;
    };
    const Case cases[] = {
        {dsp::CF_32, "cf32"},
        {dsp::CS_32, "cs32"},
        {dsp::CS_16, "cs16"},
        {dsp::CS_8, "cs8"},
        {dsp::CU_8, "cu8"},
        {dsp::WAV_16, "wav"},
#ifdef BUILD_ZIQ
        {dsp::ZIQ, "ziq"},
#endif
    };

    for (const auto &c : cases)
    {
        INFO(c.name);
        dsp::BasebandType from_enum(c.e);
        CHECK((std::string)from_enum == c.name);

        dsp::BasebandType from_str(std::string(c.name));
        CHECK(from_str == c.e);

        dsp::BasebandType from_cstr(c.name);
        CHECK(from_cstr == c.e);
    }
}

TEST_CASE("BasebandType accepts the short aliases", "[common][dsp][baseband_type]")
{
    CHECK(dsp::BasebandType("f32") == dsp::CF_32);
    CHECK(dsp::BasebandType("s32") == dsp::CS_32);
    CHECK(dsp::BasebandType("s16") == dsp::CS_16);
    CHECK(dsp::BasebandType("s8") == dsp::CS_8);
    CHECK(dsp::BasebandType("u8") == dsp::CU_8);
    CHECK(dsp::BasebandType("w16") == dsp::WAV_16);
}

TEST_CASE("BasebandType assignment operators", "[common][dsp][baseband_type]")
{
    dsp::BasebandType t;
    t = dsp::CS_16;
    CHECK(t == dsp::CS_16);
    t = std::string("cu8");
    CHECK(t == dsp::CU_8);
    t = "cs8";
    CHECK(t == dsp::CS_8);
}

TEST_CASE("BasebandType rejects unknown names", "[common][dsp][baseband_type]")
{
    CHECK_THROWS(dsp::BasebandType("cf64"));
    CHECK_THROWS(dsp::BasebandType(""));
    CHECK_THROWS(dsp::BasebandType("CF32")); // Case sensitive

    dsp::BasebandType t;
    CHECK_THROWS(t = "nope");
}
