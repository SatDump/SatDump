#include "common/codings/ldpc/labrador/decoder.h"
#include "common/codings/ldpc/labrador/encoder.h"
#include "common/codings/ldpc/labrador/codes/parity.h"
#include "common/codings/ldpc/ccsds_ldpc.h"
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>

namespace
{
    using namespace satdump::labrador;

    struct Lcg
    {
        uint32_t state;
        explicit Lcg(uint32_t seed) : state(seed) {}
        uint32_t next()
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }
        int below(int n) { return (next() >> 8) % n; }
    };

    int bitAt(const std::vector<uint8_t> &bytes, uint64_t i) { return (bytes[i / 8] >> (7 - i % 8)) & 1; }

    std::vector<uint8_t> makeCodeword(const code_params_t &c, uint32_t seed)
    {
        Lcg rng(seed);
        std::vector<uint8_t> cw(c.n / 8);
        for (auto &b : cw)
            b = rng.next() >> 24;
        encode_parity_u8(c, cw.data());
        return cw;
    }

    // Labrador LLR convention: negative = 1
    template <typename T>
    std::vector<T> toLlrs(const code_params_t &c, const std::vector<uint8_t> &cw, T amplitude)
    {
        std::vector<T> llrs(c.n);
        for (uint64_t i = 0; i < c.n; i++)
            llrs[i] = bitAt(cw, i) ? -amplitude : amplitude;
        return llrs;
    }

    template <typename T>
    struct Decoder
    {
        code_params_t c;
        std::vector<T> working;
        std::vector<uint8_t> workingU8;
        std::vector<uint8_t> output;
        uint64_t iters = 12345;

        explicit Decoder(ldpc_code_t code)
            : c(get_code_params(code)), working(c.decode_ms_working_len), workingU8(c.decode_ms_working_u8_len), output(c.output_len)
        {
        }

        bool run(std::vector<T> llrs, uint64_t maxiters)
        {
            return decode_ms<T>(c, llrs.data(), output.data(), working.data(), workingU8.data(), maxiters, &iters);
        }
    };

    // Full (n + punctured) output satisfies every parity check
    bool outputSatisfiesParity(ldpc_code_t code, const std::vector<uint8_t> &out)
    {
        code_params_t c = get_code_params(code);
        std::vector<uint8_t> syndrome(c.n + c.punctured_bits - c.k, 0);
        parity_iter_t it;
        parity_iter_init(&it, code);
        uint64_t check, var;
        while (parity_iter_next(&it, &check, &var))
            syndrome[check] ^= bitAt(out, var);
        for (auto s : syndrome)
            if (s)
                return false;
        return true;
    }

    template <typename T>
    void checkCorrection(ldpc_code_t code, T amplitude, T weak)
    {
        Decoder<T> dec(code);
        std::vector<uint8_t> cw = makeCodeword(dec.c, int(code) * 3 + 1);

        SECTION("clean codeword")
        {
            REQUIRE(dec.run(toLlrs<T>(dec.c, cw, amplitude), 20));
            CHECK(dec.iters < 20);
            CHECK(std::equal(cw.begin(), cw.end(), dec.output.begin()));
            CHECK(outputSatisfiesParity(code, dec.output));
        }

        SECTION("weak errors are corrected")
        {
            std::vector<T> llrs = toLlrs<T>(dec.c, cw, amplitude);
            Lcg rng(int(code) + 99);
            std::set<int> flips;
            while (flips.size() < dec.c.n / 100)
                flips.insert(rng.below(dec.c.n));
            for (int i : flips)
                llrs[i] = llrs[i] < 0 ? weak : T(-weak);

            REQUIRE(dec.run(llrs, 30));
            CHECK(dec.iters > 0);
            CHECK(std::equal(cw.begin(), cw.end(), dec.output.begin()));
            CHECK(outputSatisfiesParity(code, dec.output));
        }

        SECTION("random LLRs fail after maxiters")
        {
            std::vector<T> llrs(dec.c.n);
            Lcg rng(int(code) + 7);
            for (auto &l : llrs)
                l = (rng.next() >> 31) ? amplitude : T(-amplitude);
            CHECK_FALSE(dec.run(llrs, 5));
            CHECK(dec.iters == 5);
        }
    }
}

TEST_CASE("labrador decode_ms<int8_t> corrects TC and TM codewords", "[common][codings][ldpc][labrador]")
{
    for (ldpc_code_t code : {TC128, TC512, TM1280, TM1536, TM2048})
    {
        DYNAMIC_SECTION("code " << int(code))
        {
            checkCorrection<int8_t>(code, 16, 2);
        }
    }
}

TEST_CASE("labrador decode_ms<int16_t> corrects TC and TM codewords", "[common][codings][ldpc][labrador]")
{
    for (ldpc_code_t code : {TC256, TM2048, TM5120})
    {
        DYNAMIC_SECTION("code " << int(code))
        {
            checkCorrection<int16_t>(code, 400, 50);
        }
    }
}

TEST_CASE("labrador TM2048 codewords decode with CCSDSLDPC rate 1/2 k=1024", "[common][codings][ldpc][labrador]")
{
    // Cross-check between the two in-tree LDPC implementations (CCSDSLDPC soft convention: positive = 1)
    code_params_t c = get_code_params(TM2048);
    std::vector<uint8_t> cw = makeCodeword(c, 2024);

    codings::ldpc::CCSDSLDPC ldpc(codings::ldpc::RATE_1_2, 1024);
    REQUIRE(ldpc.frame_length() == int(c.n));
    std::vector<int8_t> soft(c.n);
    for (uint64_t i = 0; i < c.n; i++)
        soft[i] = bitAt(cw, i) ? 30 : -30;
    // A few weak errors in the data part
    for (int i = 5; i < 1000; i += 97)
        soft[i] = soft[i] > 0 ? -5 : 5;

    std::vector<uint8_t> out(c.n);
    ldpc.decode(soft.data(), out.data(), 20);
    for (uint64_t i = 0; i < c.n; i++)
        REQUIRE(out[i] == bitAt(cw, i));
}
