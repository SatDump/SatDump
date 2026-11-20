#pragma once

/*
Deterministic pseudo-random test data, shared by the whole suite.
Everything is inline and lives in the test_helpers namespace, as every test
file is linked into the same executable.

Lcg is the Numerical Recipes 32-bit LCG (state = state * 1664525 + 1013904223)
that the tests have always used, so failures are reproducible. Every
extraction method below is bit-identical to the local copies it replaces:
- next()      : the new 32-bit state
- bit()       : top bit (next() >> 31)
- byte()      : top byte (next() >> 24)
- below(n)    : (next() >> 8) % n
- uniform()   : 24-bit uniform in [0, 1)
- symmetric() : uniform in [-1, 1), as 2 * uniform() - 1 (exact in float too)
- sign()      : +1.0f / -1.0f from the top bit
- gauss()     : standard normal, Box-Muller on two uniform() draws
*/

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace test_helpers
{
    // One LCG step on a bare state variable, for code that passes the state around
    constexpr uint32_t lcgNext(uint32_t &state)
    {
        state = state * 1664525u + 1013904223u;
        return state;
    }

    // Deterministic LCG, so failures are reproducible
    struct Lcg
    {
        uint32_t state;

        constexpr explicit Lcg(uint32_t seed = 0x12345678) : state(seed) {}

        constexpr uint32_t next() { return lcgNext(state); }
        constexpr uint8_t bit() { return next() >> 31; }
        constexpr uint8_t byte() { return next() >> 24; }
        constexpr int below(int n) { return (next() >> 8) % n; }
        // Uniform in [0, 1)
        constexpr double uniform() { return (next() >> 8) * (1.0 / 16777216.0); }
        // Uniform in [-1, 1). The value is exactly representable as a float
        constexpr double symmetric() { return 2.0 * uniform() - 1.0; }
        // Random +1 / -1
        constexpr float sign() { return (next() >> 31) ? 1.0f : -1.0f; }
        // Standard normal (Box-Muller)
        double gauss()
        {
            double u1 = std::max(uniform(), 1e-12), u2 = uniform();
            return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
        }
    };

    // count random bits (0 or 1, one per byte), from Lcg(seed).bit()
    inline std::vector<uint8_t> randomBits(size_t count, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<uint8_t> bits(count);
        for (auto &b : bits)
            b = rng.bit();
        return bits;
    }

    // count random bytes, from Lcg(seed).byte()
    inline std::vector<uint8_t> randomBytes(size_t count, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<uint8_t> bytes(count);
        for (auto &b : bytes)
            b = rng.byte();
        return bytes;
    }
} // namespace test_helpers
