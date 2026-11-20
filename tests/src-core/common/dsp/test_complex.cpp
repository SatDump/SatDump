#include "common/dsp/complex.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <type_traits>

using Catch::Approx;

TEST_CASE("complex_t constructors set real and imaginary parts", "[common][dsp][complex]")
{
    complex_t zero;
    CHECK(zero.real == 0.0f);
    CHECK(zero.imag == 0.0f);

    complex_t re(3.5f);
    CHECK(re.real == 3.5f);
    CHECK(re.imag == 0.0f);

    complex_t both(1.5f, -2.5f);
    CHECK(both.real == 1.5f);
    CHECK(both.imag == -2.5f);

    complex_t from_std(std::complex<float>(4.0f, 5.0f));
    CHECK(from_std.real == 4.0f);
    CHECK(from_std.imag == 5.0f);
}

TEST_CASE("complex_t converts to std::complex losslessly", "[common][dsp][complex]")
{
    complex_t c(-1.25f, 7.0f);
    std::complex<float> s = c;
    CHECK(s.real() == -1.25f);
    CHECK(s.imag() == 7.0f);
}

TEST_CASE("complex_t is layout compatible with two floats", "[common][dsp][complex]")
{
    // VOLK and FFTW calls throughout dsp/ reinterpret complex_t buffers as lv_32fc_t / fftwf_complex
    STATIC_REQUIRE(sizeof(complex_t) == 2 * sizeof(float));
    STATIC_REQUIRE(std::is_trivially_copyable_v<complex_t>);

    complex_t arr[2] = {complex_t(1, 2), complex_t(3, 4)};
    float *f = (float *)arr;
    CHECK(f[0] == 1.0f);
    CHECK(f[1] == 2.0f);
    CHECK(f[2] == 3.0f);
    CHECK(f[3] == 4.0f);
}

TEST_CASE("complex_t conj, norm and arg", "[common][dsp][complex]")
{
    complex_t c(3.0f, 4.0f);

    complex_t cj = c.conj();
    CHECK(cj.real == 3.0f);
    CHECK(cj.imag == -4.0f);

    CHECK(c.norm() == Approx(5.0f));
    CHECK(c.arg() == Approx(atan2f(4.0f, 3.0f)));

    CHECK(complex_t(0, 1).arg() == Approx(M_PI / 2));
    CHECK(complex_t(-1, 0).arg() == Approx(M_PI));
    CHECK(complex_t(0, -2).arg() == Approx(-M_PI / 2));
    CHECK(complex_t(0, 0).norm() == 0.0f);
}

TEST_CASE("complex_t addition and subtraction", "[common][dsp][complex]")
{
    complex_t a(1.0f, 2.0f);
    complex_t b(-3.0f, 0.5f);

    complex_t sum = a + b;
    CHECK(sum.real == Approx(-2.0f));
    CHECK(sum.imag == Approx(2.5f));

    complex_t diff = a - b;
    CHECK(diff.real == Approx(4.0f));
    CHECK(diff.imag == Approx(1.5f));

    complex_t acc(0, 0);
    acc += a;
    acc += b;
    CHECK(acc.real == Approx(-2.0f));
    CHECK(acc.imag == Approx(2.5f));

    acc -= b;
    CHECK(acc.real == Approx(1.0f));
    CHECK(acc.imag == Approx(2.0f));
}

TEST_CASE("complex_t multiplication matches std::complex", "[common][dsp][complex]")
{
    const float vals[] = {-2.5f, -1.0f, 0.0f, 0.5f, 3.0f};
    for (float ar : vals)
        for (float ai : vals)
            for (float br : vals)
                for (float bi : vals)
                {
                    complex_t a(ar, ai), b(br, bi);
                    complex_t r = a * b;
                    std::complex<float> e = std::complex<float>(ar, ai) * std::complex<float>(br, bi);
                    CHECK(r.real == Approx(e.real()).margin(1e-6));
                    CHECK(r.imag == Approx(e.imag()).margin(1e-6));
                }

    // j * j = -1
    complex_t jj = complex_t(0, 1) * complex_t(0, 1);
    CHECK(jj.real == Approx(-1.0f));
    CHECK(jj.imag == Approx(0.0f).margin(1e-7));

    // Multiplying by the conjugate gives |z|^2 on the real axis
    complex_t z(2.0f, -3.0f);
    complex_t p = z * z.conj();
    CHECK(p.real == Approx(13.0f));
    CHECK(p.imag == Approx(0.0f).margin(1e-6));
}

TEST_CASE("complex_t scalar operations", "[common][dsp][complex]")
{
    complex_t a(2.0f, -4.0f);

    complex_t m = a * 0.5f;
    CHECK(m.real == Approx(1.0f));
    CHECK(m.imag == Approx(-2.0f));

    complex_t d = a / 4.0f;
    CHECK(d.real == Approx(0.5f));
    CHECK(d.imag == Approx(-1.0f));

    a *= 3.0f;
    CHECK(a.real == Approx(6.0f));
    CHECK(a.imag == Approx(-12.0f));
}

TEST_CASE("complex_t assignment from scalars clears the imaginary part", "[common][dsp][complex]")
{
    complex_t c(5.0f, 6.0f);
    c = 2.5f;
    CHECK(c.real == 2.5f);
    CHECK(c.imag == 0.0f);

    complex_t i(5.0f, 6.0f);
    i = 7;
    CHECK(i.real == 7.0f);
    CHECK(i.imag == 0.0f);
}

TEST_CASE("complex_t rotation by unit phasors preserves magnitude", "[common][dsp][complex]")
{
    complex_t z(0.6f, 0.8f);
    complex_t rot(cosf(0.1f), sinf(0.1f));
    float start_arg = z.arg();
    for (int i = 0; i < 10; i++)
        z = z * rot;
    CHECK(z.norm() == Approx(1.0f).epsilon(1e-5));
    CHECK(z.arg() == Approx(start_arg + 1.0f).epsilon(1e-5));
}
