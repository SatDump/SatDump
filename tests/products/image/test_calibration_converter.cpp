#include "common/calibration.h"
#include "products/image/calibration_converter.h"
#include "products/image/calibration_units.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using satdump::calibration::UnitConverter;

namespace
{
    // Planck's law in wavenumber form: L in mW/(m^2 sr cm^-1), v in cm^-1, T in K
    const double C1 = 1.191042e-5, C2 = 1.4387752;
    double planckRadiance(double t, double v) { return C1 * v * v * v / (std::exp(C2 * v / t) - 1); }

    // 2024-03-20 12:00:00 UTC, the March equinox at noon on the prime meridian
    const double EQUINOX_NOON = 1710936000;

    nlohmann::json globeCfgAt(double timestamp)
    {
        return {{"type", "equirec"}, {"offset_x", -180.0}, {"offset_y", 90.0}, {"scalar_x", 1.0},
                {"scalar_y", -1.0},  {"width", 360},       {"height", 180},    {"proj_timestamp", timestamp}};
    }

    bool contains(const std::vector<std::string> &v, const std::string &s) { return std::find(v.begin(), v.end(), s) != v.end(); }
} // namespace

TEST_CASE("getUnitInfo describes the built-in calibration units", "[products][calibration]")
{
    auto bt = satdump::calibration::getUnitInfo(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
    CHECK(bt.name == "Brightness Temperature");
    CHECK(bt.unit == "°K");
    CHECK(bt.getNiceUnits() == "Brightness Temperature (°K)");
    CHECK(satdump::calibration::getUnitInfo(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS).unit == "°C");
    CHECK(satdump::calibration::getUnitInfo(CALIBRATION_ID_SUN_ANGLE).unit == "Deg");
    CHECK(satdump::calibration::getUnitInfo(CALIBRATION_ID_ALBEDO).name == "Albedo");
    CHECK(satdump::calibration::getUnitInfo(CALIBRATION_ID_EMISSIVE_RADIANCE).name == "Emissive Radiance");

    auto unknown = satdump::calibration::getUnitInfo("not_a_unit");
    CHECK(unknown.unit == "unknown");
    CHECK(unknown.name == "Unknown Unit");
}

TEST_CASE("getAvailableConversions lists reachable units", "[products][calibration]")
{
    auto em = satdump::calibration::getAvailableConversions(CALIBRATION_ID_EMISSIVE_RADIANCE);
    CHECK(em.front() == CALIBRATION_ID_EMISSIVE_RADIANCE); // The input unit itself comes first
    CHECK(contains(em, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE));
    CHECK(contains(em, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS));
    CHECK(contains(em, CALIBRATION_ID_SUN_ANGLE));

    auto bt = satdump::calibration::getAvailableConversions(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
    CHECK(contains(bt, CALIBRATION_ID_EMISSIVE_RADIANCE));
    CHECK(contains(bt, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS));

    CHECK(contains(satdump::calibration::getAvailableConversions(CALIBRATION_ID_ALBEDO), CALIBRATION_ID_SUN_ANGLE_COMPENSATED_ALBEDO));
    CHECK(contains(satdump::calibration::getAvailableConversions(CALIBRATION_ID_REFLECTIVE_RADIANCE), CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE));

    // Uncalibrated data can only produce a sun angle
    CHECK(satdump::calibration::getAvailableConversions("") == std::vector<std::string>{CALIBRATION_ID_SUN_ANGLE});
}

TEST_CASE("getAvailableConverters returns one converter per supported pair", "[products][calibration]")
{
    using satdump::calibration::getAvailableConverters;
    CHECK(getAvailableConverters(CALIBRATION_ID_EMISSIVE_RADIANCE, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE).size() == 1);
    CHECK(getAvailableConverters(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE, CALIBRATION_ID_EMISSIVE_RADIANCE).size() == 1);
    CHECK(getAvailableConverters(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS).size() == 1);
    CHECK(getAvailableConverters("anything", CALIBRATION_ID_SUN_ANGLE).size() == 1);
    CHECK(getAvailableConverters(CALIBRATION_ID_ALBEDO, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE).empty());
}

TEST_CASE("UnitConverter passes values through when units are equal", "[products][calibration]")
{
    UnitConverter c;
    c.set_conversion("x", "x");
    CHECK(c.convert(0, 0, 12.5) == 12.5);
    double mn = -3, mx = 7;
    CHECK(c.convert_range(mn, mx));
    CHECK(mn == -3);
    CHECK(mx == 7);
}

TEST_CASE("UnitConverter without a valid converter returns invalid values", "[products][calibration]")
{
    UnitConverter c;
    c.set_conversion(CALIBRATION_ID_ALBEDO, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
    CHECK(c.convert(0, 0, 0.5) == CALIBRATION_INVALID_VALUE);
    double mn = 0, mx = 1;
    CHECK_FALSE(c.convert_range(mn, mx));
}

TEST_CASE("UnitConverter Kelvin <=> Celsius", "[products][calibration]")
{
    UnitConverter k2c, c2k;
    k2c.set_conversion(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS);
    c2k.set_conversion(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);

    CHECK_THAT(k2c.convert(0, 0, 273.15), WithinAbs(0, 1e-12));
    CHECK_THAT(k2c.convert(0, 0, 300), WithinAbs(26.85, 1e-12));
    CHECK_THAT(c2k.convert(0, 0, -40), WithinAbs(233.15, 1e-12));
    CHECK(k2c.convert(0, 0, CALIBRATION_INVALID_VALUE) == CALIBRATION_INVALID_VALUE);

    double mn = 200, mx = 320;
    REQUIRE(k2c.convert_range(mn, mx));
    CHECK_THAT(mn, WithinAbs(-73.15, 1e-12));
    CHECK_THAT(mx, WithinAbs(46.85, 1e-12));
}

TEST_CASE("UnitConverter radiance <=> brightness temperature follows Planck's law", "[products][calibration]")
{
    const double wn = 930; // ~10.75 um
    UnitConverter r2t, r2c, t2r;
    r2t.set_wavenumber(wn);
    r2c.set_wavenumber(wn);
    t2r.set_wavenumber(wn);
    r2t.set_conversion(CALIBRATION_ID_EMISSIVE_RADIANCE, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
    r2c.set_conversion(CALIBRATION_ID_EMISSIVE_RADIANCE, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE_CELSIUS);
    t2r.set_conversion(CALIBRATION_ID_BRIGHTNESS_TEMPERATURE, CALIBRATION_ID_EMISSIVE_RADIANCE);

    for (double t : {200.0, 250.0, 300.0, 330.0})
    {
        const double l = planckRadiance(t, wn);
        CHECK_THAT(r2t.convert(0, 0, l), WithinAbs(t, 1e-3));
        CHECK_THAT(r2c.convert(0, 0, l), WithinAbs(t - 273.15, 1e-3));
        CHECK_THAT(t2r.convert(0, 0, t), WithinRel(l, 1e-6));
    }

    double mn = planckRadiance(220, wn), mx = planckRadiance(310, wn);
    REQUIRE(r2t.convert_range(mn, mx));
    CHECK_THAT(mn, WithinAbs(220, 1e-3));
    CHECK_THAT(mx, WithinAbs(310, 1e-3));

    CHECK(r2t.convert(0, 0, CALIBRATION_INVALID_VALUE) == CALIBRATION_INVALID_VALUE);

    // Without a wavenumber the conversion is impossible
    UnitConverter nowl;
    nowl.set_conversion(CALIBRATION_ID_EMISSIVE_RADIANCE, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
    CHECK(nowl.convert(0, 0, 100) == CALIBRATION_INVALID_VALUE);
}

TEST_CASE("UnitConverter sun angle needs a projection with a timestamp", "[products][calibration]")
{
    UnitConverter noproj;
    noproj.set_conversion("", CALIBRATION_ID_SUN_ANGLE);
    CHECK(noproj.convert(10, 10, 1) == CALIBRATION_INVALID_VALUE);
    double mn = -5, mx = 5;
    REQUIRE(noproj.convert_range(mn, mx));
    CHECK(mn == 0);
    CHECK(mx == 90);

    UnitConverter notime;
    nlohmann::json cfg = globeCfgAt(0);
    cfg.erase("proj_timestamp");
    notime.set_proj(cfg);
    REQUIRE(notime.proj_valid);
    notime.set_conversion("", CALIBRATION_ID_SUN_ANGLE);
    CHECK(notime.convert(180, 90, 1) == CALIBRATION_INVALID_VALUE);
}

TEST_CASE("UnitConverter sun angle is the solar elevation at the pixel", "[products][calibration]")
{
    UnitConverter c;
    c.set_proj(globeCfgAt(EQUINOX_NOON));
    REQUIRE(c.proj_valid);
    c.set_conversion(CALIBRATION_ID_ALBEDO, CALIBRATION_ID_SUN_ANGLE);

    // Pixel (180, 90) is 0N 0E: sun nearly overhead (equation of time and declination keep it ~2 deg off)
    CHECK(c.convert(180, 90, 0) > 85);
    // 90 degrees of longitude away the sun is on the horizon
    CHECK_THAT(c.convert(270, 90, 0), WithinAbs(0, 3));
    // On the antimeridian it is midnight
    CHECK(c.convert(0, 90, 0) < -85);
    // At 60N the noon elevation is ~30 degrees at the equinox
    CHECK_THAT(c.convert(180, 30, 0), WithinAbs(30, 3));
}

TEST_CASE("UnitConverter reflective radiance sun compensation", "[products][calibration]")
{
    UnitConverter c;
    c.set_proj(globeCfgAt(EQUINOX_NOON));
    c.set_conversion(CALIBRATION_ID_REFLECTIVE_RADIANCE, CALIBRATION_ID_SUN_ANGLE_COMPENSATED_REFLECTIVE_RADIANCE);

    // At 60N, local noon on the equinox, the sun zenith angle is ~60 degrees: radiance / cos(60) ~ doubles
    const double v = c.convert(180, 30, 10);
    CHECK_THAT(v, WithinAbs(compensate_radiance_for_sun(10, (time_t)EQUINOX_NOON, 60, 0), 1e-6));
    CHECK_THAT(v, WithinRel(20, 0.1));

    // Range conversion is a no-op for this converter
    double mn = 0, mx = 50;
    REQUIRE(c.convert_range(mn, mx));
    CHECK(mn == 0);
    CHECK(mx == 50);

    CHECK(c.convert(180, 30, CALIBRATION_INVALID_VALUE) == CALIBRATION_INVALID_VALUE);
}
