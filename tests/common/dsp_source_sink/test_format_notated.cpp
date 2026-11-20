#include "common/dsp_source_sink/format_notated.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("format_notated picks SI prefixes with units", "[common][dsp_source_sink][format_notated]")
{
    CHECK(format_notated(137.1e6, "Hz") == "137.1 MHz");
    CHECK(format_notated(1.5e9, "Hz") == "1.5 GHz");
    CHECK(format_notated(2.5e3, "Hz") == "2.5 kHz");
    CHECK(format_notated(3e12, "Hz") == "3 THz");
    CHECK(format_notated(4e15, "Hz") == "4 PHz");
    CHECK(format_notated(500.0, "Hz") == "500 Hz");
    CHECK(format_notated(999.0, "Hz") == "999 Hz");
    CHECK(format_notated(1000.0, "Hz") == "1 kHz");
}

TEST_CASE("format_notated supports integer types", "[common][dsp_source_sink][format_notated]")
{
    CHECK(format_notated<uint64_t>(2400000, "sps") == "2.4 Msps");
    CHECK(format_notated<int64_t>(-2500, "Hz") == "-2.5 kHz");
    CHECK(format_notated<int>(500, "Hz") == "500 Hz");
    // Integer types never use sub-unit prefixes
    CHECK(format_notated<int>(0, "Hz") == "0 Hz");
    CHECK(format_notated<uint64_t>(0, "Hz") == "0 Hz");
}

TEST_CASE("format_notated uses sub-unit prefixes for small values", "[common][dsp_source_sink][format_notated]")
{
    CHECK(format_notated(0.0015, "s") == "1.5 ms");
    CHECK(format_notated(2.5e-6, "s") == "2.5 µs");
    CHECK(format_notated(3e-9, "s") == "3 ns");
    CHECK(format_notated(-0.25, "V") == "-250 mV");
    CHECK(format_notated(0.5f, "V") == "500 mV");
    // Zero falls into the smallest bucket
    CHECK(format_notated(0.0, "Hz") == "0 nHz");
}

TEST_CASE("format_notated can stay at or above one unit", "[common][dsp_source_sink][format_notated]")
{
    CHECK(format_notated(0.5, "Hz", -1, false) == "0.5 Hz");
    CHECK(format_notated(0.0, "Hz", -1, false) == "0 Hz");
}

TEST_CASE("format_notated honours the number of decimals", "[common][dsp_source_sink][format_notated]")
{
    CHECK(format_notated(137.1e6, "Hz", 3) == "137.100 MHz");
    CHECK(format_notated(137.1e6, "Hz", 0) == "137 MHz");
    CHECK(format_notated<uint64_t>(1234, "B", 2) == "1.23 kB");
    // Default precision is 6 significant digits
    CHECK(format_notated(1234567.0, "Hz") == "1.23457 MHz");
}

TEST_CASE("format_notated without units uses wider ranges and no spacing", "[common][dsp_source_sink][format_notated]")
{
    CHECK(format_notated(1234567.0) == "1234.57k");
    CHECK(format_notated(5e9) == "5000M");
    CHECK(format_notated(2e10) == "20G");
    CHECK(format_notated(0.002) == "2m");
    // The plain range always appends a space, even without units
    CHECK(format_notated(500.0) == "500 ");
}
