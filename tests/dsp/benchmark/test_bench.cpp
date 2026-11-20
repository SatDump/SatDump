#include "dsp/benchmark/bench.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <set>

using namespace satdump::ndsp;

TEST_CASE("ndsp getBenchCategories lists unique benchmark categories", "[dsp][benchmark]")
{
    auto cats = getBenchCategories();
    REQUIRE(cats.size() == 10);
    CHECK(std::set<std::string>(cats.begin(), cats.end()).size() == cats.size());
    for (auto c : {"fft_ddc", "simple_gardner", "simple_zero_crossing", "agc", "costas", "rrc", "mm_recovery", "splitter", "freq_shift", "resamplers"})
        CHECK(std::find(cats.begin(), cats.end(), c) != cats.end());
}

TEST_CASE("ndsp runBenchmarks with an unknown category runs nothing", "[dsp][benchmark]")
{
    // Only the category filter is exercised : no benchmark matches, so this returns at once
    CHECK(runBenchmarks({"no_such_category"}).empty());
}
