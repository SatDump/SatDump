#include "common/codings/ldpc/labrador/codes/codes.h"
#include "common/codings/ldpc/labrador/codes/parity.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
    using namespace satdump::labrador;

    struct Expected
    {
        ldpc_code_t code;
        uint64_t n, k, p;
    };

    // CCSDS 231.0-B (TC) and 131.0-B (TM AR4JA) code sizes
    const Expected CODES[] = {
        {TC128, 128, 64, 0},       {TC256, 256, 128, 0},      {TC512, 512, 256, 0},       {TM1280, 1280, 1024, 128},  {TM1536, 1536, 1024, 256},
        {TM2048, 2048, 1024, 512}, {TM5120, 5120, 4096, 512}, {TM6144, 6144, 4096, 1024}, {TM8192, 8192, 4096, 2048},
    };
} // namespace

TEST_CASE("labrador get_code_params sizes and derived lengths", "[common][codings][ldpc][labrador]")
{
    for (auto &e : CODES)
    {
        INFO("code " << int(e.code));
        code_params_t c = get_code_params(e.code);
        CHECK(c.c == e.code);
        CHECK(c.n == e.n);
        CHECK(c.k == e.k);
        CHECK(c.punctured_bits == e.p);
        // Relations documented in codes.h
        CHECK(c.decode_bf_working_len == c.n + c.punctured_bits);
        CHECK(c.decode_ms_working_len == 2 * c.paritycheck_sum + 3 * c.n + 3 * c.punctured_bits - 2 * c.k);
        CHECK(c.decode_ms_working_u8_len == (c.n + c.punctured_bits - c.k) / 8);
        CHECK(c.output_len == (c.n + c.punctured_bits) / 8);
        CHECK(c.ptr_compact_h != nullptr);
        CHECK(c.ptr_compact_g != nullptr);
        // TM codes use the AR4JA permutations, TC codes only circulant shifts
        CHECK((c.ptr_phi_j_k != nullptr) == (e.p != 0));
        if (e.p != 0)
            CHECK(c.punctured_bits == c.submatrix_size);
    }
}

TEST_CASE("labrador get_code_params rejects unknown codes", "[common][codings][ldpc][labrador]") { CHECK_THROWS(get_code_params(ldpc_code_t(42))); }

TEST_CASE("labrador parity_iter yields paritycheck_sum in-range edges", "[common][codings][ldpc][labrador]")
{
    for (auto &e : CODES)
    {
        INFO("code " << int(e.code));
        code_params_t c = get_code_params(e.code);
        parity_iter_t it;
        parity_iter_init(&it, e.code);

        uint64_t check, var, edges = 0;
        std::vector<int> checkDegree(c.n + c.punctured_bits - c.k, 0);
        std::vector<int> varDegree(c.n + c.punctured_bits, 0);
        while (parity_iter_next(&it, &check, &var))
        {
            REQUIRE(check < checkDegree.size());
            REQUIRE(var < varDegree.size());
            checkDegree[check]++;
            varDegree[var]++;
            edges++;
        }
        CHECK(edges == c.paritycheck_sum);
        // Every check and every variable is connected
        {
            size_t unconnected = 0, first = 0;
            for (size_t i = 0; i < checkDegree.size(); i++)
                if (!(checkDegree[i] > 0) && unconnected++ == 0)
                    first = i;
            INFO("first unconnected check " << first);
            REQUIRE(unconnected == 0);
        }
        {
            size_t unconnected = 0, first = 0;
            for (size_t i = 0; i < varDegree.size(); i++)
                if (!(varDegree[i] > 0) && unconnected++ == 0)
                    first = i;
            INFO("first unconnected variable " << first);
            REQUIRE(unconnected == 0);
        }
    }
}
