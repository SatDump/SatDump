#include "common/widgets/datetime.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using satdump::widgets::DateTimePicker;

// Only get()/set() are tested; draw() needs an ImGui context

namespace
{
    const double T_2024_01_02_030405 = 1704164645; // 2024-01-02 03:04:05 UTC
} // namespace

TEST_CASE("DateTimePicker round-trips whole second timestamps", "[common][widgets][datetime]")
{
    DateTimePicker picker("##dt", T_2024_01_02_030405);
    CHECK(picker.get() == T_2024_01_02_030405);

    picker.set(0);
    CHECK(picker.get() == 0);

    picker.set(951782400); // 2000-02-29, leap day
    CHECK(picker.get() == 951782400);
}

TEST_CASE("DateTimePicker keeps fractional seconds", "[common][widgets][datetime]")
{
    DateTimePicker picker("##dt", T_2024_01_02_030405 + 0.5);
    CHECK(picker.get() == Catch::Approx(T_2024_01_02_030405 + 0.5).epsilon(0).margin(1e-6));

    picker.set(T_2024_01_02_030405 + 0.25);
    CHECK(picker.get() == Catch::Approx(T_2024_01_02_030405 + 0.25).epsilon(0).margin(1e-6));

    picker.set(T_2024_01_02_030405 + 0.123);
    CHECK(picker.get() == Catch::Approx(T_2024_01_02_030405 + 0.123).epsilon(0).margin(1e-5));
}

TEST_CASE("DateTimePicker uses -1 for automatic time", "[common][widgets][datetime]")
{
    DateTimePicker picker("##dt", -1);
    CHECK(picker.get() == -1.0);

    picker.set(T_2024_01_02_030405);
    CHECK(picker.get() == T_2024_01_02_030405);

    picker.set(-1);
    CHECK(picker.get() == -1.0);
}

// Bug: datetime.cpp:37-48 stores the fraction as the integer digits after the
// decimal point and rescales by its digit count, dropping leading zeros, so
// .05 comes back as .5 (and .001 as .1).
TEST_CASE("DateTimePicker keeps fractional seconds with leading zeros", "[common][widgets][datetime][!shouldfail]")
{
    DateTimePicker picker("##dt", T_2024_01_02_030405 + 0.05);
    CHECK(picker.get() == Catch::Approx(T_2024_01_02_030405 + 0.05).epsilon(0).margin(1e-5));
}
