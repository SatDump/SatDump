#include "common/codings/rotation.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace
{
    std::vector<int8_t> makeSoft(int count, uint32_t seed)
    {
        std::vector<int8_t> soft(count);
        for (auto &s : soft)
        {
            seed = seed * 1664525u + 1013904223u;
            s = (int8_t)((seed >> 24) % 255 - 127); // -127..127, no -128
        }
        return soft;
    }
} // namespace

TEST_CASE("rotate_soft PHASE_0 without swap leaves symbols untouched", "[common][codings][rotation]")
{
    std::vector<int8_t> soft = makeSoft(64, 1);
    std::vector<int8_t> original = soft;
    rotate_soft(soft.data(), soft.size(), PHASE_0, false);
    CHECK(soft == original);
}

TEST_CASE("rotate_soft maps each (I, Q) pair as documented", "[common][codings][rotation]")
{
    std::vector<int8_t> base = {10, 20, -30, 40, 0, -5, 127, -127};

    SECTION("PHASE_90: (I, Q) -> (Q, -I)")
    {
        std::vector<int8_t> s = base;
        rotate_soft(s.data(), s.size(), PHASE_90, false);
        CHECK(s == std::vector<int8_t>{20, -10, 40, 30, -5, 0, -127, -127});
    }

    SECTION("PHASE_180: (I, Q) -> (-I, -Q)")
    {
        std::vector<int8_t> s = base;
        rotate_soft(s.data(), s.size(), PHASE_180, false);
        CHECK(s == std::vector<int8_t>{-10, -20, 30, -40, 0, 5, -127, 127});
    }

    SECTION("PHASE_270: (I, Q) -> (-Q, I)")
    {
        std::vector<int8_t> s = base;
        rotate_soft(s.data(), s.size(), PHASE_270, false);
        CHECK(s == std::vector<int8_t>{-20, 10, -40, -30, 5, 0, 127, 127});
    }

    SECTION("IQ swap happens before the rotation")
    {
        std::vector<int8_t> s = base;
        rotate_soft(s.data(), s.size(), PHASE_0, true);
        CHECK(s == std::vector<int8_t>{20, 10, 40, -30, -5, 0, -127, 127});

        std::vector<int8_t> r = base;
        rotate_soft(r.data(), r.size(), PHASE_90, true);
        // swap -> (Q, I), then (x, y) -> (y, -x) gives (I, -Q)
        CHECK(r == std::vector<int8_t>{10, -20, -30, -40, 0, 5, 127, 127});
    }
}

TEST_CASE("rotate_soft rotations compose like a cyclic group", "[common][codings][rotation]")
{
    std::vector<int8_t> original = makeSoft(512, 2);

    SECTION("four 90 degree steps are the identity")
    {
        std::vector<int8_t> s = original;
        for (int i = 0; i < 4; i++)
            rotate_soft(s.data(), s.size(), PHASE_90, false);
        CHECK(s == original);
    }

    SECTION("90 then 270 is the identity")
    {
        std::vector<int8_t> s = original;
        rotate_soft(s.data(), s.size(), PHASE_90, false);
        rotate_soft(s.data(), s.size(), PHASE_270, false);
        CHECK(s == original);
    }

    SECTION("two 90 degree steps equal one 180 step")
    {
        std::vector<int8_t> a = original, b = original;
        rotate_soft(a.data(), a.size(), PHASE_90, false);
        rotate_soft(a.data(), a.size(), PHASE_90, false);
        rotate_soft(b.data(), b.size(), PHASE_180, false);
        CHECK(a == b);
    }

    SECTION("swapping twice is the identity")
    {
        std::vector<int8_t> s = original;
        rotate_soft(s.data(), s.size(), PHASE_0, true);
        rotate_soft(s.data(), s.size(), PHASE_0, true);
        CHECK(s == original);
    }
}

TEST_CASE("rotate_soft clamps -128 to -127 so negation cannot overflow", "[common][codings][rotation]")
{
    std::vector<int8_t> s = {-128, -128, 5, -128};
    rotate_soft(s.data(), s.size(), PHASE_180, false);
    CHECK(s == std::vector<int8_t>{127, 127, -5, 127});

    std::vector<int8_t> z = {-128, 3};
    rotate_soft(z.data(), z.size(), PHASE_0, false);
    CHECK(z == std::vector<int8_t>{-127, 3});
}

TEST_CASE("rotate_soft with zero size is a no-op", "[common][codings][rotation]")
{
    int8_t s[2] = {1, 2};
    rotate_soft(s, 0, PHASE_90, true);
    CHECK(s[0] == 1);
    CHECK(s[1] == 2);
}

TEST_CASE("rotate_soft_arbitrary with zero phase is the identity", "[common][codings][rotation]")
{
    std::vector<int8_t> original = makeSoft(256, 3);
    std::vector<int8_t> s = original;
    rotate_soft_arbitrary(s.data(), s.size(), 0.0f);
    CHECK(s == original);
}

TEST_CASE("rotate_soft_arbitrary multiplies each pair by exp(j*phase)", "[common][codings][rotation]")
{
    std::vector<int8_t> original = makeSoft(256, 4);
    for (float phase : {0.3f, 1.0f, (float)M_PI / 2, (float)M_PI, -2.0f})
    {
        INFO("phase " << phase);
        std::vector<int8_t> s = original;
        rotate_soft_arbitrary(s.data(), s.size(), phase);
        for (size_t i = 0; i < s.size(); i += 2)
        {
            float re = original[i] * cosf(phase) - original[i + 1] * sinf(phase);
            float im = original[i + 1] * cosf(phase) + original[i] * sinf(phase);
            re = std::fmax(-127.0f, std::fmin(127.0f, re));
            im = std::fmax(-127.0f, std::fmin(127.0f, im));
            // Results are truncated, not rounded
            REQUIRE(std::fabs(s[i] - re) < 1.0001f);
            REQUIRE(std::fabs(s[i + 1] - im) < 1.0001f);
        }
    }
}

TEST_CASE("rotate_soft_arbitrary saturates at the int8 range", "[common][codings][rotation]")
{
    // |127 + 127j| = 179.6, rotated by 45 degrees it lies on the imaginary axis
    std::vector<int8_t> s = {127, 127, -127, -127};
    rotate_soft_arbitrary(s.data(), s.size(), (float)M_PI / 4);
    CHECK(std::abs(s[0]) <= 1);
    CHECK(s[1] == 127);
    CHECK(std::abs(s[2]) <= 1);
    CHECK(s[3] == -127);
}

TEST_CASE("rotate_soft_arbitrary ignores a trailing odd symbol", "[common][codings][rotation]")
{
    std::vector<int8_t> s = {10, 20, 30};
    rotate_soft_arbitrary(s.data(), s.size(), (float)M_PI);
    CHECK(s[2] == 30);
}
