#include "image/histogram_utils.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace hist = satdump::image::histogram;

TEST_CASE("image::histogram::get_histogram counts occurrences", "[image][histogram_utils]")
{
    std::vector<int> h = hist::get_histogram({0, 1, 1, 3, 3, 3, 7}, 8);
    CHECK(h == std::vector<int>{1, 2, 0, 3, 0, 0, 0, 1});

    // Empty input gives an all-zero histogram of the requested length
    CHECK(hist::get_histogram({}, 4) == std::vector<int>{0, 0, 0, 0});
}

TEST_CASE("image::histogram::equalize_histogram is the cumulative sum", "[image][histogram_utils]")
{
    CHECK(hist::equalize_histogram({1, 2, 0, 3, 0, 1}) == std::vector<int>{1, 3, 3, 6, 6, 7});
    CHECK(hist::equalize_histogram({5}) == std::vector<int>{5});
    CHECK(hist::equalize_histogram({0, 0, 0}) == std::vector<int>{0, 0, 0});
}

TEST_CASE("image::histogram::make_hist_match_table with identical histograms is the identity", "[image][histogram_utils]")
{
    // Strictly increasing cumulative histogram: each value is found at its own index
    std::vector<int> cdf = {1, 3, 6, 10, 15};
    std::vector<int> table = hist::make_hist_match_table(cdf, cdf, 0);
    CHECK(table == std::vector<int>{0, 1, 2, 3, 4});
}

TEST_CASE("image::histogram::make_hist_match_table maps to matching cumulative counts", "[image][histogram_utils]")
{
    // input cdf value at i is looked up in the target cdf
    std::vector<int> input = {2, 4, 6, 8, 8, 8};
    std::vector<int> target = {0, 2, 4, 6, 8, 8};
    std::vector<int> table = hist::make_hist_match_table(input, target, 0);
    REQUIRE(table.size() == target.size());
    CHECK(table[0] == 0); // Always forced to 0
    CHECK(table[1] == 2); // 4 is at target index 2
    CHECK(table[2] == 3); // 6 at index 3
    CHECK(table[3] == 4); // 8 first found at index 4
    CHECK(table[4] == 4);
    CHECK(table[5] == 4);
}

TEST_CASE("image::histogram::make_hist_match_table searches neighbouring values", "[image][histogram_utils]")
{
    // 5 is missing from the target, 6 (one higher) is found first
    std::vector<int> input = {0, 5, 9};
    std::vector<int> target = {0, 6, 9};
    std::vector<int> table = hist::make_hist_match_table(input, target, 0);
    CHECK(table[0] == 0);
    CHECK(table[1] == 1);
    CHECK(table[2] == 2);
}

TEST_CASE("image::histogram::make_hist_match_table zero matches fall back to identity", "[image][histogram_utils]")
{
    // A match at index 0 for i != 0 is replaced by i
    std::vector<int> input = {0, 0, 0};
    std::vector<int> target = {0, 1, 2};
    std::vector<int> table = hist::make_hist_match_table(input, target, 0);
    CHECK(table == std::vector<int>{0, 1, 2});
}
