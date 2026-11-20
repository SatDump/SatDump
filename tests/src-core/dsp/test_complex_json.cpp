#include "dsp/complex_json.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("ndsp complex_json serializes complex_t as real/imag", "[dsp][complex_json]")
{
    nlohmann::json j = complex_t(1.5f, -2.25f);
    REQUIRE(j.is_object());
    CHECK(j.size() == 2);
    CHECK(j["real"].get<float>() == 1.5f);
    CHECK(j["imag"].get<float>() == -2.25f);
}

TEST_CASE("ndsp complex_json round-trips complex_t and vectors of it", "[dsp][complex_json]")
{
    std::vector<complex_t> v = {{0, 0}, {1, -1}, {3.25f, 1e-6f}, {-1e10f, 7}};
    nlohmann::json j = v;
    auto back = j.get<std::vector<complex_t>>();
    REQUIRE(back.size() == v.size());
    for (size_t i = 0; i < v.size(); i++)
    {
        CHECK(back[i].real == v[i].real);
        CHECK(back[i].imag == v[i].imag);
    }

    // Through a text dump too
    auto c = nlohmann::json::parse(nlohmann::json(complex_t(0.5f, 4)).dump()).get<complex_t>();
    CHECK(c.real == 0.5f);
    CHECK(c.imag == 4.0f);
}

TEST_CASE("ndsp complex_json rejects incomplete objects", "[dsp][complex_json]")
{
    nlohmann::json j = {{"real", 1.0}};
    CHECK_THROWS(j.get<complex_t>());
}
