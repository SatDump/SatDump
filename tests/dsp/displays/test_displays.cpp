#include "../ndsp_test_helpers.h"
#include "dsp/displays/const_disp.h"
#include "dsp/displays/hist_disp.h"
#include "dsp/displays/level_helper.h"
#include "dsp/displays/time_disp.h"
#include <catch2/catch_test_macros.hpp>

using namespace satdump::ndsp;

// Only the non-rendering logic (sample ingestion and configuration) is tested here

TEST_CASE("ndsp TimeDisplayBlock keeps the latest samples in a sliding history", "[dsp][displays][time_disp]")
{
    auto b = std::make_shared<TimeDisplayBlock>();
    CHECK(b->get_cfg("size") == 1024);
    REQUIRE(ndsp_test::configure(*b, {{"size", 8}}) == Block::RES_OK);
    CHECK(b->get_cfg("size") == 8);
    CHECK(b->time_history == std::vector<float>(8, 0.0f));

    auto r = ndsp_test::runSimple<float, float>(b, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, 3);
    REQUIRE(r.completed);
    CHECK(r.outputs.empty());
    CHECK(b->time_history == std::vector<float>{4, 5, 6, 7, 8, 9, 10, 11});
}

TEST_CASE("ndsp TimeDisplayBlock shifts in fewer samples than its size", "[dsp][displays][time_disp]")
{
    auto b = std::make_shared<TimeDisplayBlock>();
    b->set_cfg("size", 5);
    auto r = ndsp_test::runSimple<float, float>(b, {1.5f, -2.5f}, 8);
    REQUIRE(r.completed);
    CHECK(b->time_history == std::vector<float>{0, 0, 0, 1.5f, -2.5f});
}

TEST_CASE("ndsp TimeDisplayBlock config", "[dsp][displays][time_disp]")
{
    TimeDisplayBlock b;
    CHECK(b.d_id == "time_disp_f");
    CHECK(b.get_inputs()[0].type == DSP_SAMPLE_TYPE_F32);
    CHECK(b.get_outputs().empty());
    // Sizes of 1 or less are ignored
    b.set_cfg("size", 1);
    CHECK(b.get_cfg("size") == 1024);
    b.set_cfg("size", -5);
    CHECK(b.get_cfg("size") == 1024);
    b.set_cfg("size", 2);
    CHECK(b.get_cfg("size") == 2);
    CHECK(b.get_cfg_list()["size"]["type"] == "int");
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

TEST_CASE("ndsp LevelHelperDisplayBlock stores 20 log10(1 + |x|)", "[dsp][displays][level_helper]")
{
    auto b = std::make_shared<LevelHelperDisplayBlock>();
    b->set_cfg("size", 4);
    std::vector<complex_t> in = {{0, 0}, {3, 4}, {0, -9}, {0.5, 0}, {1, 0}};
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, in, 2);
    REQUIRE(r.completed);
    REQUIRE(b->time_history.size() == 4);
    // The first sample was shifted out
    CHECK(std::fabs(b->time_history[0] - 20 * log10(6.0)) < 1e-4);
    CHECK(std::fabs(b->time_history[1] - 20 * log10(10.0)) < 1e-4);
    CHECK(std::fabs(b->time_history[2] - 20 * log10(1.5)) < 1e-4);
    CHECK(std::fabs(b->time_history[3] - 20 * log10(2.0)) < 1e-4);
}

TEST_CASE("ndsp LevelHelperDisplayBlock config", "[dsp][displays][level_helper]")
{
    LevelHelperDisplayBlock b;
    // The id says float, but the input is complex
    CHECK(b.d_id == "level_helper_disp_f");
    CHECK(b.get_inputs()[0].type == DSP_SAMPLE_TYPE_CF32);
    CHECK(b.get_cfg("size") == 1024);
    b.set_cfg("size", 16);
    CHECK(b.get_cfg("size") == 16);
    b.set_cfg("size", 0);
    CHECK(b.get_cfg("size") == 16);
    CHECK_THROWS(b.get_cfg("x"));
    CHECK_THROWS(b.set_cfg("x", 1));
}

TEST_CASE("ndsp ConstellationDisplayBlock consumes samples until a terminator", "[dsp][displays][const_disp]")
{
    auto b = std::make_shared<ConstellationDisplayBlock>();
    CHECK(b->d_id == "const_disp_c");
    CHECK(b->get_outputs().empty());
    // More than the internal 2048 sample history, in large and small buffers
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, ndsp_test::randomComplex(10000, 81), 3000);
    REQUIRE(r.completed);
    auto r2 = ndsp_test::runSimple<complex_t, complex_t>(b, ndsp_test::randomComplex(100, 82), 7, 2);
    REQUIRE(r2.completed);
    CHECK(b->get_cfg_list().empty());
    CHECK_THROWS(b->get_cfg("x"));
    CHECK_THROWS(b->set_cfg("x", 1));
}

TEST_CASE("ndsp HistogramDisplayBlock consumes samples until a terminator", "[dsp][displays][hist_disp]")
{
    auto b = std::make_shared<HistogramDisplayBlock>();
    CHECK(b->d_id == "hist_disp_c");
    CHECK(b->get_outputs().empty());
    auto r = ndsp_test::runSimple<complex_t, complex_t>(b, ndsp_test::randomComplex(10000, 83), 4096);
    REQUIRE(r.completed);
    auto r2 = ndsp_test::runSimple<complex_t, complex_t>(b, ndsp_test::randomComplex(10, 84), 3, 2);
    REQUIRE(r2.completed);
    CHECK_THROWS(b->get_cfg("x"));
    CHECK_THROWS(b->set_cfg("x", 1));
}
