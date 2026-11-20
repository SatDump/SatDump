#include "projection/raytrace/timestamp_filtering.h"
#include <catch2/catch_test_macros.hpp>

using satdump::timestamp_filtering::filter_timestamps_width_cfg;

namespace
{
    nlohmann::json simpleFilter(double scan_time, double max_diff) { return {{"type", "simple"}, {"scan_time", scan_time}, {"max_diff", max_diff}}; }

    std::vector<double> ramp(int n, double start = 1000, double step = 1)
    {
        std::vector<double> v;
        for (int i = 0; i < n; i++)
            v.push_back(start + i * step);
        return v;
    }
} // namespace

TEST_CASE("filter_timestamps_width_cfg passes timestamps through for unknown filter types", "[projection][raytrace][timestamp_filtering]")
{
    std::vector<double> t = {5, 1, -1, 1e9};
    CHECK(filter_timestamps_width_cfg(t, {{"type", "none"}}) == t);
}

TEST_CASE("filter_timestamps_width_cfg simple filter removes outliers far from the median", "[projection][raytrace][timestamp_filtering]")
{
    auto t = ramp(10);
    t[4] = 99999;  // Far outside 10 * 1s * (0.5 + 1.5) around the median
    t[7] = -50000; // Same, below
    auto f = filter_timestamps_width_cfg(t, simpleFilter(1, 3));
    REQUIRE(f.size() == t.size());
    CHECK(f[4] == -1);
    CHECK(f[7] == -1);
    for (int i : {1, 2, 3, 9})
        CHECK(f[i] == t[i]);
}

TEST_CASE("filter_timestamps_width_cfg simple filter removes backwards steps and jumps", "[projection][raytrace][timestamp_filtering]")
{
    auto t = ramp(10);
    t[5] = 1003.5; // Goes backwards relative to 1004
    auto f = filter_timestamps_width_cfg(t, simpleFilter(1, 3));
    CHECK(f[5] == -1);
    CHECK(f[4] == 1004);
    CHECK(f[6] == 1006);

    t = ramp(10);
    for (int i = 6; i < 10; i++)
        t[i] += 4; // A 5s gap between line 5 and line 6, above max_diff = 3
    f = filter_timestamps_width_cfg(t, simpleFilter(1, 3));
    CHECK(f[5] == 1005);
    CHECK(f[6] == -1);
    CHECK(f[7] == t[7]); // The next line is compared against the dropped one, which is close
}

// Bug: projection/raytrace/timestamp_filtering.cpp:21 and :28 call unqualified abs() on doubles, which resolves
// to the integer ::abs here, so differences are truncated: a 3.5s gap counts as 3s and
// passes max_diff = 3, and the median-distance check is truncated the same way.
// Whether the double overload is visible depends on the standard library headers: the integer
// ::abs is picked with libstdc++, but most likely not with libc++ or MSVC, where the test passes.
TEST_CASE("filter_timestamps_width_cfg simple filter compares fractional gaps exactly", "[projection][raytrace][timestamp_filtering][!mayfail]")
{
    auto t = ramp(10);
    for (int i = 6; i < 10; i++)
        t[i] += 2.5; // A 3.5s gap between line 5 and line 6, above max_diff = 3
    auto f = filter_timestamps_width_cfg(t, simpleFilter(1, 3));
    CHECK(f[5] == 1005);
    CHECK(f[6] == -1);
}

TEST_CASE("filter_timestamps_width_cfg simple filter keeps invalid markers", "[projection][raytrace][timestamp_filtering]")
{
    auto t = ramp(8);
    t[3] = -1;
    auto f = filter_timestamps_width_cfg(t, simpleFilter(1, 3));
    CHECK(f[3] == -1);
    CHECK(f[4] == 1004);
    CHECK(f[5] == 1005);
}

TEST_CASE("filter_timestamps_width_cfg simple filter honours the margin", "[projection][raytrace][timestamp_filtering]")
{
    auto t = ramp(10);
    t[8] = 1030; // ~25s from the median: rejected with margin 1.5 (range 20)...
    CHECK(filter_timestamps_width_cfg(t, simpleFilter(1, 100))[8] == -1);

    // ...but kept with margin 3 (range 35)
    nlohmann::json cfg = simpleFilter(1, 100);
    cfg["margin"] = 3.0;
    CHECK(filter_timestamps_width_cfg(t, cfg)[8] == 1030);
}

// Bug: timestamp_filtering.cpp:13 starts with last = -1, so the first valid timestamp is
// always checked against -1 and rejected by `abs(last - v) > max_diff` (for any realistic
// UNIX timestamp). A perfectly regular series loses its first line.
TEST_CASE("filter_timestamps_width_cfg simple filter keeps a perfectly regular series", "[projection][raytrace][timestamp_filtering][!shouldfail]")
{
    auto t = ramp(10);
    CHECK(filter_timestamps_width_cfg(t, simpleFilter(1, 3)) == t);
}
