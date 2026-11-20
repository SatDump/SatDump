#include "utils/stats.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace satdump;
using Catch::Approx;

TEST_CASE("most_common returns the most frequent element", "[utils][stats]")
{
    std::vector<int> v = {1, 2, 2, 3, 2, 1};
    CHECK(most_common(v.begin(), v.end(), 0) == 2);

    std::vector<std::string> s = {"a", "b", "b"};
    CHECK(most_common(s.begin(), s.end(), std::string("none")) == "b");

    std::vector<int> empty;
    CHECK(most_common(empty.begin(), empty.end(), -1) == -1);
}

TEST_CASE("average_common averages a range", "[utils][stats]")
{
    std::vector<double> v = {1.0, 2.0, 3.0, 6.0};
    CHECK(average_common(v.begin(), v.end()) == Approx(3.0));

    std::vector<int> i = {2, 4, 6};
    CHECK(average_common(i.begin(), i.end()) == 4);
}

TEST_CASE("percentile interpolates between elements", "[utils][stats]")
{
    float odd[5] = {1, 2, 3, 4, 5};
    CHECK(percentile(odd, 5, 50) == Approx(3.0f));

    float even[4] = {10, 20, 30, 40};
    CHECK(percentile(even, 4, 50) == Approx(25.0f));
    CHECK(percentile(even, 4, 20) == Approx(10.0f));
    CHECK(percentile(even, 4, 80) == Approx(40.0f));
}

TEST_CASE("avg_overflowless matches a plain average", "[utils][stats]")
{
    CHECK(avg_overflowless(std::vector<int>{1, 2, 3, 4}) == Approx(2.5));
    CHECK(avg_overflowless(std::vector<double>{-1.0, 1.0}) == Approx(0.0));
    CHECK(avg_overflowless(std::vector<int>{}) == 0.0);

    // Summing these directly would overflow a uint8_t
    CHECK(avg_overflowless(std::vector<uint8_t>{250, 250, 250}) == Approx(250.0));
}

TEST_CASE("get_median returns the middle element", "[utils][stats]")
{
    CHECK(get_median({3, 1, 2}) == 2);
    CHECK(get_median({5}) == 5);
    CHECK(get_median({}) == 0);

    // With an even count the upper of the two middle values is returned
    CHECK(get_median({4, 1, 3, 2}) == 3);
}
