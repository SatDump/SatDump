#include "dsp/block_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace satdump::ndsp;

TEST_CASE("ndsp getShortTypeName maps every sample type", "[dsp][block_helpers]")
{
    CHECK(getShortTypeName<complex_t>() == "c");
    CHECK(getShortTypeName<float>() == "f");
    CHECK(getShortTypeName<int16_t>() == "s");
    CHECK(getShortTypeName<int8_t>() == "h");
    CHECK(getShortTypeName<uint8_t>() == "b");
    CHECK_THROWS_AS(getShortTypeName<double>(), satdump::satdump_exception_t);
    CHECK_THROWS_AS(getShortTypeName<int32_t>(), satdump::satdump_exception_t);
}

TEST_CASE("ndsp getTypeSampleType maps every sample type", "[dsp][block_helpers]")
{
    CHECK(getTypeSampleType<complex_t>() == DSP_SAMPLE_TYPE_CF32);
    CHECK(getTypeSampleType<float>() == DSP_SAMPLE_TYPE_F32);
    CHECK(getTypeSampleType<int16_t>() == DSP_SAMPLE_TYPE_S16);
    CHECK(getTypeSampleType<int8_t>() == DSP_SAMPLE_TYPE_S8);
    CHECK(getTypeSampleType<uint8_t>() == DSP_SAMPLE_TYPE_U8);
    CHECK_THROWS_AS(getTypeSampleType<double>(), satdump::satdump_exception_t);
}

TEST_CASE("ndsp getColorFromDSPType gives distinct colors and red for unknown", "[dsp][block_helpers]")
{
    auto toU32 = [](ImColor c) { return (ImU32)c; };

    std::vector<ImU32> colors = {
        toU32(getColorFromDSPType(DSP_SAMPLE_TYPE_CF32)), toU32(getColorFromDSPType(DSP_SAMPLE_TYPE_F32)), toU32(getColorFromDSPType(DSP_SAMPLE_TYPE_S16)),
        toU32(getColorFromDSPType(DSP_SAMPLE_TYPE_S8)),   toU32(getColorFromDSPType(DSP_SAMPLE_TYPE_U8)),
    };
    for (size_t i = 0; i < colors.size(); i++)
        for (size_t j = i + 1; j < colors.size(); j++)
            CHECK(colors[i] != colors[j]);

    CHECK(toU32(getColorFromDSPType(DSP_SAMPLE_TYPE_CF32)) == toU32(ImColor(5, 150, 255)));

    ImColor unknown = getColorFromDSPType((BlockIOType)42);
    CHECK(unknown.Value.x == 1.0f);
    CHECK(unknown.Value.y == 0.0f);
    CHECK(unknown.Value.z == 0.0f);
}
