#include "common/codings/ldpc/labrador/encoder.h"
#include "common/codings/ldpc/labrador/codes/parity.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    using namespace satdump::labrador;

    std::vector<uint8_t> randomBytes(int count, uint32_t seed)
    {
        std::vector<uint8_t> v(count);
        uint32_t state = seed;
        for (auto &b : v)
        {
            state = state * 1664525u + 1013904223u;
            b = state >> 24;
        }
        return v;
    }

    int bitAt(const std::vector<uint8_t> &bytes, uint64_t i) { return (bytes[i / 8] >> (7 - i % 8)) & 1; }

    // Syndrome check of an unpunctured codeword against the parity iterator's H
    bool satisfiesParity(ldpc_code_t code, const std::vector<uint8_t> &cw)
    {
        code_params_t c = get_code_params(code);
        std::vector<uint8_t> syndrome(c.n - c.k, 0);
        parity_iter_t it;
        parity_iter_init(&it, code);
        uint64_t check, var;
        while (parity_iter_next(&it, &check, &var))
            syndrome[check] ^= bitAt(cw, var);
        for (auto s : syndrome)
            if (s)
                return false;
        return true;
    }
}

TEST_CASE("labrador encode_parity_u8 produces valid systematic TC codewords", "[common][codings][ldpc][labrador]")
{
    for (ldpc_code_t code : {TC128, TC256, TC512})
    {
        INFO("code " << int(code));
        code_params_t c = get_code_params(code);
        for (uint32_t seed = 1; seed <= 3; seed++)
        {
            std::vector<uint8_t> cw = randomBytes(c.n / 8, seed);
            std::vector<uint8_t> data(cw.begin(), cw.begin() + c.k / 8);
            encode_parity_u8(c, cw.data());
            CHECK(std::equal(data.begin(), data.end(), cw.begin()));
            CHECK(satisfiesParity(code, cw));
        }
    }
}

TEST_CASE("labrador encode_parity_u8 and encode_parity_u32 agree", "[common][codings][ldpc][labrador]")
{
    for (ldpc_code_t code : {TC128, TC256, TC512, TM1280, TM1536, TM2048, TM5120, TM6144, TM8192})
    {
        INFO("code " << int(code));
        code_params_t c = get_code_params(code);
        std::vector<uint8_t> a = randomBytes(c.n / 8, int(code) + 10);
        std::vector<uint8_t> b = a;
        encode_parity_u8(c, a.data());
        encode_parity_u32(c, b.data());
        CHECK(a == b);
    }
}

TEST_CASE("labrador encoder is linear and maps zero to zero", "[common][codings][ldpc][labrador]")
{
    for (ldpc_code_t code : {TC256, TM2048, TM1280})
    {
        INFO("code " << int(code));
        code_params_t c = get_code_params(code);
        std::vector<uint8_t> zero(c.n / 8, 0xFF);
        std::fill(zero.begin(), zero.begin() + c.k / 8, 0);
        encode_parity_u8(c, zero.data());
        CHECK(zero == std::vector<uint8_t>(c.n / 8, 0));

        std::vector<uint8_t> a = randomBytes(c.n / 8, 1), b = randomBytes(c.n / 8, 2), x(c.n / 8);
        for (size_t i = 0; i < x.size(); i++)
            x[i] = a[i] ^ b[i];
        encode_parity_u8(c, a.data());
        encode_parity_u8(c, b.data());
        encode_parity_u8(c, x.data());
        for (size_t i = 0; i < x.size(); i++)
            REQUIRE(x[i] == (a[i] ^ b[i]));
    }
}
