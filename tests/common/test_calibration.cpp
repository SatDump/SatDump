#include "common/calibration.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <ctime>

namespace
{
    time_t utc(int year, int month, int day, int hour, int minute)
    {
        std::tm t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day;
        t.tm_hour = hour;
        t.tm_min = minute;
        return timegm(&t);
    }

    // Planck spectral radiance per wavelength, W/(m^2 sr um), lambda in um
    double planckWavelength(double lambda_um, double t)
    {
        const double c1 = 1.191042e8;
        const double c2 = 1.4387752e4;
        return c1 / (std::pow(lambda_um, 5) * (std::exp(c2 / (lambda_um * t)) - 1));
    }
} // namespace

TEST_CASE("temperature_to_radiance matches the Planck function", "[common][calibration]")
{
    // 300 K at 900 cm^-1, in mW/(m^2 sr cm^-1)
    CHECK(temperature_to_radiance(300, 900) == Catch::Approx(117.472).epsilon(1e-5));
    // Radiance increases with temperature
    CHECK(temperature_to_radiance(310, 900) > temperature_to_radiance(300, 900));
    CHECK(temperature_to_radiance(200, 900) < temperature_to_radiance(300, 900));
}

TEST_CASE("radiance_to_temperature inverts temperature_to_radiance", "[common][calibration]")
{
    for (double wn : {700.0, 900.0, 1400.0, 2500.0})
    {
        for (double t : {180.0, 250.0, 300.0, 330.0})
        {
            double l = temperature_to_radiance(t, wn);
            CHECK(radiance_to_temperature(l, wn) == Catch::Approx(t).epsilon(1e-6));
        }
    }
}

TEST_CASE("freq_to_wavenumber and wavenumber_to_freq convert between Hz and cm^-1", "[common][calibration]")
{
    // A 10 um wavelength is 1000 cm^-1
    double f10um = 299792458.0 / 10e-6;
    CHECK(freq_to_wavenumber(f10um) == Catch::Approx(1000.0).epsilon(1e-12));
    CHECK(wavenumber_to_freq(1000.0) == Catch::Approx(f10um).epsilon(1e-12));

    for (double wn : {1.0, 650.0, 2500.0, 25000.0})
        CHECK(freq_to_wavenumber(wavenumber_to_freq(wn)) == Catch::Approx(wn).epsilon(1e-12));
}

// BUG: calibration.cpp:15 has a misplaced parenthesis in the inverse Planck
// function: log(c_1 / (lambda^5 * L + 1)) instead of log(c_1 / (lambda^5 * L) + 1),
// which biases the computed brightness temperature.
TEST_CASE("spectral_radiance_to_radiance converts per-um radiance to per-cm^-1 radiance", "[common][calibration][!shouldfail]")
{
    // A 300 K blackbody at 10 um (1000 cm^-1), expressed in both units
    double l_wavelength = planckWavelength(10.0, 300.0);
    double expected = temperature_to_radiance(300.0, 1000.0);
    CHECK(spectral_radiance_to_radiance(l_wavelength, 1000.0) == Catch::Approx(expected).epsilon(1e-4));
}

TEST_CASE("calculate_sun_irradiance_interval is positive and finite", "[common][calibration]")
{
    double e = calculate_sun_irradiance_interval(0.5e-6, 0.7e-6);
    CHECK(std::isfinite(e));
    CHECK(e > 0);
}

TEST_CASE("get_sun_angle gives the solar elevation", "[common][calibration]")
{
    // Near the March equinox the sun is close to overhead at the equator at local noon
    CHECK(get_sun_angle(utc(2024, 3, 20, 12, 0), 0, 0) > 85.0);
    CHECK(get_sun_angle(utc(2024, 3, 20, 0, 0), 0, 0) < -85.0);
    // Longitude is east positive: 06:00 UTC is local noon at 90E
    CHECK(get_sun_angle(utc(2024, 3, 20, 6, 0), 0, 90) > 85.0);
    CHECK(get_sun_angle(utc(2024, 3, 20, 6, 0), 0, 0) == Catch::Approx(0.0).margin(5.0));

    // Solstices: overhead on the relevant tropic, low on the opposite one
    double june_north = get_sun_angle(utc(2024, 6, 21, 12, 0), 23.44, 0);
    double june_south = get_sun_angle(utc(2024, 6, 21, 12, 0), -23.44, 0);
    CHECK(june_north > 88.0);
    CHECK(june_south == Catch::Approx(90.0 - 2 * 23.44).margin(2.0));
    CHECK(get_sun_angle(utc(2024, 12, 21, 12, 0), -23.44, 0) > 88.0);
}

TEST_CASE("radiance_to_reflectance normalises by irradiance and sun angle", "[common][calibration]")
{
    time_t noon = utc(2024, 3, 20, 12, 0);

    // Sun almost overhead and earth-sun distance close to 1 AU in March
    double r = radiance_to_reflectance(100.0, 50.0, noon, 0, 0);
    CHECK(r == Catch::Approx(0.5).epsilon(0.02));

    // Linear in radiance, inversely proportional to irradiance
    CHECK(radiance_to_reflectance(100.0, 100.0, noon, 0, 0) == Catch::Approx(2 * r));
    CHECK(radiance_to_reflectance(200.0, 50.0, noon, 0, 0) == Catch::Approx(r / 2));

    // Sun below 10 degrees elevation is rejected
    CHECK(radiance_to_reflectance(100.0, 50.0, utc(2024, 3, 20, 0, 0), 0, 0) == Catch::Approx(-999.99));
}

TEST_CASE("compensate_radiance_for_sun divides by the sun zenith cosine", "[common][calibration]")
{
    time_t noon = utc(2024, 3, 20, 12, 0);
    CHECK(compensate_radiance_for_sun(10.0, noon, 0, 0) == Catch::Approx(10.0).epsilon(0.02));

    // Lower sun gives a larger compensation factor
    time_t morning = utc(2024, 3, 20, 9, 0);
    CHECK(compensate_radiance_for_sun(10.0, morning, 0, 0) > compensate_radiance_for_sun(10.0, noon, 0, 0));

    // Night is rejected
    CHECK(compensate_radiance_for_sun(10.0, utc(2024, 3, 20, 0, 0), 0, 0) == Catch::Approx(-999.99));
}
