#include "common/dsp/demod/constellation.h"
#include "../dsp_test_helpers.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <set>

using Catch::Approx;

namespace
{
    struct ConstCase
    {
        dsp::constellation_type_t type;
        int bits;
        const char *name;
    };

    const ConstCase all_types[] = {
        {dsp::BPSK, 1, "BPSK"},
        {dsp::QPSK, 2, "QPSK"},
        {dsp::OQPSK, 2, "OQPSK"},
        {dsp::PSK8, 3, "8PSK"},
        {dsp::APSK16, 4, "16APSK"},
        {dsp::APSK32, 5, "32APSK"},
    };

    // Soft bits are output MSB first, positive meaning 1
    int softToSymbol(const int8_t *bits, int nbits)
    {
        int s = 0;
        for (int i = 0; i < nbits; i++)
            s = (s << 1) | (bits[i] > 0 ? 1 : 0);
        return s;
    }

    complex_t rotate(complex_t c, float a) { return c * complex_t(cosf(a), sinf(a)); }
}

TEST_CASE("constellation_t bit counts", "[common][dsp][constellation]")
{
    for (const auto &c : all_types)
    {
        INFO(c.name);
        dsp::constellation_t cst(c.type);
        CHECK(cst.getBitsCnt() == c.bits);
    }
}

TEST_CASE("constellation_t rejects unknown types", "[common][dsp][constellation]")
{
    CHECK_THROWS(dsp::constellation_t((dsp::constellation_type_t)42));
}

TEST_CASE("constellation_t points are distinct", "[common][dsp][constellation]")
{
    for (const auto &c : all_types)
    {
        INFO(c.name);
        dsp::constellation_t cst(c.type);
        int n = 1 << c.bits;
        for (int a = 0; a < n; a++)
            for (int b = a + 1; b < n; b++)
                REQUIRE(dsp_test::absDiff(cst.mod(a), cst.mod(b)) > 0.05f);
    }
}

TEST_CASE("constellation_t BPSK and 8PSK points are on the unit circle", "[common][dsp][constellation]")
{
    dsp::constellation_t bpsk(dsp::BPSK);
    CHECK(bpsk.mod(0).real == -1.0f);
    CHECK(bpsk.mod(1).real == 1.0f);
    CHECK(bpsk.mod(0).imag == 0.0f);

    dsp::constellation_t psk8(dsp::PSK8);
    std::set<int> angles;
    for (int s = 0; s < 8; s++)
    {
        complex_t p = psk8.mod(s);
        CHECK(p.norm() == Approx(1.0f).epsilon(1e-5));
        // 8 points spread every 45 degrees
        angles.insert((int)std::lround((p.arg() + M_PI) / (M_PI / 4)) % 8);
    }
    CHECK(angles.size() == 8);
}

TEST_CASE("constellation_t QPSK points are on the diagonals", "[common][dsp][constellation]")
{
    dsp::constellation_t qpsk(dsp::QPSK);
    for (int s = 0; s < 4; s++)
    {
        complex_t p = qpsk.mod(s);
        CHECK(fabs(p.real) == Approx(fabs(p.imag)));
        // Mapping: bit 0 = sign of I, bit 1 = sign of Q
        CHECK((p.real > 0) == (bool)(s & 1));
        CHECK((p.imag > 0) == (bool)(s & 2));
    }
}

TEST_CASE("constellation_t APSK rings use the gamma ratios", "[common][dsp][constellation]")
{
    // 16APSK: 4 + 12 points, outer / inner radius = gamma
    dsp::constellation_t apsk16(dsp::APSK16, 3.15f);
    std::vector<float> radii;
    for (int s = 0; s < 16; s++)
        radii.push_back(apsk16.mod(s).norm());
    std::sort(radii.begin(), radii.end());
    CHECK(radii[0] == Approx(radii[3]));
    CHECK(radii[4] == Approx(radii[15]));
    CHECK(radii[15] / radii[0] == Approx(3.15f).epsilon(1e-4));

    // 32APSK: 4 + 12 + 16 points
    dsp::constellation_t apsk32(dsp::APSK32, 2.64f, 4.64f);
    radii.clear();
    for (int s = 0; s < 32; s++)
        radii.push_back(apsk32.mod(s).norm());
    std::sort(radii.begin(), radii.end());
    CHECK(radii[3] == Approx(radii[0]));
    CHECK(radii[4] == Approx(radii[15]));
    CHECK(radii[16] == Approx(radii[31]));
    CHECK(radii[4] / radii[0] == Approx(2.64f).epsilon(1e-4));
    CHECK(radii[16] / radii[0] == Approx(4.64f).epsilon(1e-4));

    // Default gammas when 0 is passed
    dsp::constellation_t def16(dsp::APSK16);
    float rmin = 1e9, rmax = 0;
    for (int s = 0; s < 16; s++)
    {
        rmin = std::min(rmin, def16.mod(s).norm());
        rmax = std::max(rmax, def16.mod(s).norm());
    }
    CHECK(rmax / rmin == Approx(2.57f).epsilon(1e-4));
}

TEST_CASE("constellation_t hard demod inverts mod for BPSK, QPSK and OQPSK", "[common][dsp][constellation]")
{
    for (auto type : {dsp::BPSK, dsp::QPSK, dsp::OQPSK})
    {
        dsp::constellation_t cst(type);
        int n = 1 << cst.getBitsCnt();
        for (int s = 0; s < n; s++)
        {
            INFO("type " << (int)type << " symbol " << s);
            CHECK(cst.demod(cst.mod(s)) == s);
            // Small noise and phase rotation do not change the decision
            CHECK(cst.demod(rotate(cst.mod(s), 0.3f) * 0.6f) == s);
        }
    }

    // Types without a hard slicer return 0
    dsp::constellation_t psk8(dsp::PSK8);
    CHECK(psk8.demod(psk8.mod(5)) == 0);
}

TEST_CASE("constellation_t soft_demod slices interleaved I/Q softs", "[common][dsp][constellation]")
{
    dsp::constellation_t qpsk(dsp::QPSK);
    int8_t softs[8] = {50, 50, -50, 50, 50, -50, -10, -127};
    CHECK(qpsk.soft_demod(&softs[0]) == 3);
    CHECK(qpsk.soft_demod(&softs[2]) == 2);
    CHECK(qpsk.soft_demod(&softs[4]) == 1);
    CHECK(qpsk.soft_demod(&softs[6]) == 0);

    uint8_t bits[4];
    qpsk.soft_demod(softs, 8, bits);
    CHECK(bits[0] == 3);
    CHECK(bits[1] == 2);
    CHECK(bits[2] == 1);
    CHECK(bits[3] == 0);

    dsp::constellation_t bpsk(dsp::BPSK);
    int8_t b1 = 20, b0 = -20;
    CHECK(bpsk.soft_demod(&b1) == 1);
    CHECK(bpsk.soft_demod(&b0) == 0);

    dsp::constellation_t psk8(dsp::PSK8);
    CHECK(psk8.soft_demod(&softs[0]) == 0);
}

TEST_CASE("constellation_t demod_soft_calc recovers every symbol", "[common][dsp][constellation]")
{
    for (const auto &c : all_types)
    {
        INFO(c.name);
        dsp::constellation_t cst(c.type);
        int n = 1 << c.bits;
        for (int s = 0; s < n; s++)
        {
            INFO("symbol " << s);
            int8_t bits[5] = {0};
            float perr = 99;
            cst.demod_soft_calc(cst.mod(s), bits, &perr);
            CHECK(softToSymbol(bits, c.bits) == s);
            // Exactly on a constellation point: no phase error
            CHECK(perr == Approx(0.0f).margin(1e-4));
        }
    }
}

TEST_CASE("constellation_t demod_soft_calc phase error follows a small rotation", "[common][dsp][constellation]")
{
    for (auto type : {dsp::BPSK, dsp::QPSK, dsp::PSK8})
    {
        dsp::constellation_t cst(type);
        for (float a : {-0.2f, -0.05f, 0.05f, 0.2f})
        {
            INFO("type " << (int)type << " angle " << a);
            float perr = 0;
            int8_t bits[3];
            cst.demod_soft_calc(rotate(cst.mod(1), a), bits, &perr);
            CHECK(perr == Approx(a).margin(1e-4));
            CHECK(softToSymbol(bits, cst.getBitsCnt()) == 1);
        }
    }

    // Null outputs are allowed
    dsp::constellation_t qpsk(dsp::QPSK);
    CHECK_NOTHROW(qpsk.demod_soft_calc(qpsk.mod(2), nullptr, nullptr));
}

TEST_CASE("constellation_t soft confidence grows with distance from the decision boundary", "[common][dsp][constellation]")
{
    dsp::constellation_t bpsk(dsp::BPSK);
    int8_t weak, strong;
    bpsk.demod_soft_calc(complex_t(0.05f, 0), &weak);
    bpsk.demod_soft_calc(complex_t(0.5f, 0), &strong);
    CHECK(weak > 0);
    CHECK(strong > weak);

    int8_t neg;
    bpsk.demod_soft_calc(complex_t(-0.5f, 0), &neg);
    CHECK(neg == -strong);
}

TEST_CASE("constellation_t demod_soft_lut matches demod_soft_calc on the LUT grid", "[common][dsp][constellation]")
{
    const int res = 64;
    for (auto type : {dsp::QPSK, dsp::PSK8, dsp::APSK16})
    {
        dsp::constellation_t cst(type);
        cst.make_lut(res);
        int nbits = cst.getBitsCnt();

        for (int x = 0; x < res; x += 3)
            for (int y = 0; y < res; y += 5)
            {
                INFO("type " << (int)type << " x " << x << " y " << y);
                float gx = (float(x - res / 2) / float(res)) * 1.5f;
                float gy = (float(y - res / 2) / float(res)) * 1.5f;
                // Middle of the LUT cell, so the index rounds back to (x, y)
                float cx = gx + 0.5f * 1.5f / res;
                float cy = gy + 0.5f * 1.5f / res;

                int8_t lut_bits[5], calc_bits[5];
                float lut_perr, calc_perr;
                cst.demod_soft_lut(complex_t(cx, cy), lut_bits, &lut_perr);
                cst.demod_soft_calc(complex_t(gx, gy), calc_bits, &calc_perr);
                for (int b = 0; b < nbits; b++)
                    REQUIRE(lut_bits[b] == calc_bits[b]);
                REQUIRE(lut_perr == calc_perr);
            }

        // Out-of-range samples are clamped to the LUT edges rather than overflowing
        int8_t bits[5];
        CHECK_NOTHROW(cst.demod_soft_lut(complex_t(100.0f, -100.0f), bits));
    }

    // 32APSK bypasses the LUT entirely
    dsp::constellation_t apsk32(dsp::APSK32);
    int8_t a[5], b[5];
    apsk32.demod_soft_lut(apsk32.mod(17), a);
    apsk32.demod_soft_calc(apsk32.mod(17), b);
    for (int i = 0; i < 5; i++)
        CHECK(a[i] == b[i]);
}
