#include "dsp/channel_model/model/antenna/dipole.h"
#include "dsp/channel_model/model/antenna/helix.h"
#include "dsp/channel_model/model/antenna/parabolic_reflector.h"
#include "dsp/channel_model/model/antenna/quadrafilar_helix.h"
#include "dsp/channel_model/model/attenuation/atmospheric_gases_itu.h"
#include "dsp/channel_model/model/attenuation/atmospheric_gases_regression.h"
#include "dsp/channel_model/model/attenuation/free_space_path_loss.h"
#include "dsp/channel_model/model/new/channel_model_leo.h"
#include "dsp/channel_model/model/new/link_margin.h"
#include "helpers/guards.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <memory>
#include <stdexcept>

using Catch::Approx;

namespace
{
    constexpr double kC = 299792458.0;

    // Antennas bump a global ID counter on construction, the tests put it back afterwards with a
    // test_helpers::ValueGuard on antenna::GenericAntenna::base_unique_id

    std::shared_ptr<antenna::GenericAntenna> dipole(double f) { return std::make_shared<antenna::DipoleAntenna>(antenna::GenericAntenna::DIPOLE, f, RHCP, 0); }
} // namespace

TEST_CASE("ndsp GenericAntenna common accessors", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    antenna::DipoleAntenna a(antenna::GenericAntenna::DIPOLE, 437e6, LHCP, 30.0);
    antenna::DipoleAntenna b(antenna::GenericAntenna::DIPOLE, 145.8e6, RHCP, 0.0);
    CHECK(a.get_frequency() == 437e6);
    CHECK(a.get_wavelength() == Approx(kC / 437e6));
    CHECK(a.get_polarization() == LHCP);
    // Pointing error is given in degrees and stored in radians
    CHECK(a.get_pointing_error() == Approx(30.0 * M_PI / 180.0));
    a.set_pointing_error(45);
    CHECK(a.get_pointing_error() == Approx(M_PI / 4));
    // Unique, increasing IDs
    CHECK(b.unique_id() == a.unique_id() + 1);
    CHECK(a.d_type == antenna::GenericAntenna::DIPOLE);
}

TEST_CASE("ndsp DipoleAntenna gain and pointing loss", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    antenna::DipoleAntenna a(antenna::GenericAntenna::DIPOLE, 437e6, RHCP, 0.0);
    CHECK(a.get_gain() == Approx(2.15));
    CHECK(a.get_beamwidth() == Approx(156.2));
    CHECK(a.get_gain_rolloff() == 0);
    // cos pattern : 3 dB at 60 degrees, symmetric beyond 90 degrees
    a.set_pointing_error(60);
    CHECK(a.get_gain_rolloff() == Approx(10 * log10(2)).epsilon(1e-6));
    a.set_pointing_error(120);
    CHECK(a.get_gain_rolloff() == Approx(10 * log10(2)).epsilon(1e-6));
}

TEST_CASE("ndsp HelixAntenna gain, beamwidth and pointing loss", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    // 10 turns, spacing 0.25 wavelength, circumference 1 wavelength (Kraus)
    antenna::HelixAntenna h(antenna::GenericAntenna::HELIX, 2.4e9, RHCP, 0.0, 10, 0.25, 1.0);
    CHECK(h.get_gain() == Approx(10 * log10(15 * 1.0 * 10 * 0.25)));
    CHECK(h.get_beamwidth() == Approx(115.0 / std::sqrt(10 * 0.25)));
    CHECK(h.get_gain_rolloff() == 0);
    // Half power at half the beamwidth
    h.set_pointing_error(h.get_beamwidth() / 2);
    CHECK(h.get_gain_rolloff() == Approx(3.01).margin(0.02));
    // Continuous toward 0
    h.set_pointing_error(1e-4);
    CHECK(std::fabs(h.get_gain_rolloff()) < 1e-4);
}

TEST_CASE("ndsp ParabolicReflectorAntenna gain and beamwidth", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    // 3 m dish at 10 GHz, 55 % efficiency (given in percent)
    antenna::ParabolicReflectorAntenna p(antenna::GenericAntenna::PARABOLIC_REFLECTOR, 10e9, RHCP, 0.0, 3.0, 55);
    double lambda = kC / 10e9;
    CHECK(p.get_gain() == Approx(10 * log10(0.55 * std::pow(M_PI * 3.0 / lambda, 2))).margin(0.02));
    CHECK(p.get_beamwidth() == Approx(21.0 / (10 * 3.0)));
    CHECK(p.get_gain_rolloff() == 0);
    p.set_pointing_error(p.get_beamwidth() / 2);
    CHECK(p.get_gain_rolloff() == Approx(3.01).margin(0.02));
}

// Bug: parabolic_reflector.cpp uses 3282.1 instead of (180 / pi)^2 = 3282.81 (as the
// helix antenna does) in the pointing loss, so the loss does not go to 0 for small errors.
TEST_CASE("ndsp ParabolicReflectorAntenna pointing loss is continuous at 0", "[dsp][channel_model][!shouldfail]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    antenna::ParabolicReflectorAntenna p(antenna::GenericAntenna::PARABOLIC_REFLECTOR, 10e9, RHCP, 1e-4, 3.0, 55);
    CHECK(std::fabs(p.get_gain_rolloff()) < 1e-4);
}

TEST_CASE("ndsp QuadrifilarHelixAntenna gain and pointing loss", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    antenna::QuadrifilarHelixAntenna q(antenna::GenericAntenna::QUADRIFILAR_HELIX, 137e6, RHCP, 0.0, 1.0);
    CHECK(q.get_gain() == 4);
    CHECK(q.get_beamwidth() == 150);
    CHECK(q.get_gain_rolloff() == Approx(0).margin(0.01));
    q.set_pointing_error(90);
    CHECK(q.get_gain_rolloff() == Approx(-1.5 * (-4 + 10 * log10(1.256))).epsilon(1e-6));
}

TEST_CASE("ndsp FreeSpacePathLossAttenuation matches 20 log10(4 pi d f / c)", "[dsp][channel_model]")
{
    FreeSpacePathLossAttenuation fspl;
    for (double f : {137e6, 2.4e9, 8e9})
    {
        for (double d_km : {500.0, 2000.0, 36000.0})
        {
            fspl.set_frequency(f);
            fspl.set_slant_range(d_km);
            double expected = 20 * log10(4 * M_PI * d_km * 1e3 * f / kC);
            CHECK(fspl.get_attenuation() == Approx(expected).margin(0.01));
        }
    }
}

namespace
{
    // Ippolito Table 3-1 / 3-2, 1 GHz row
    double zenithAt1GHz(double rho, double t) { return 0.0334 + 0.00000276 * rho - 0.000112 * t; }
} // namespace

// Bug: atmospheric_gases_regression.cpp:84 (and :89) uses (ha * zenitha) where the slant
// attenuation should be (ha * gammaa) / sin(elevation), i.e. zenitha / sin(elevation)
// since ha = zenitha / gammaa. The result is zenitha^2 / gammaa instead.
TEST_CASE("ndsp AtmosphericGasesRegression at zenith equals the zenith attenuation", "[dsp][channel_model][!shouldfail]")
{
    AtmosphericGasesRegression a(7.5, 15);
    a.set_frequency(1e9);
    a.set_elevation_angle(M_PI / 2);
    CHECK(a.get_attenuation() == Approx(zenithAt1GHz(7.5, 15)).epsilon(1e-3));
}

// Bug: atmospheric_gases_regression.cpp:11-20, the coefficients are looked up in the
// constructor, from the frequency member which is still 0 at that point. set_frequency()
// therefore has no effect, every frequency uses the 1 GHz coefficients.
TEST_CASE("ndsp AtmosphericGasesRegression depends on the frequency", "[dsp][channel_model][!shouldfail]")
{
    AtmosphericGasesRegression a(7.5, 15);
    a.set_elevation_angle(M_PI / 4);
    a.set_frequency(1e9);
    double at1 = a.get_attenuation();
    a.set_frequency(12e9);
    double at12 = a.get_attenuation();
    // Gaseous absorption is way higher at 12 GHz than at 1 GHz
    CHECK(at12 > at1 * 1.2);
}

TEST_CASE("ndsp AtmosphericGasesRegression increases toward the horizon", "[dsp][channel_model]")
{
    AtmosphericGasesRegression a(7.5, 15);
    a.set_frequency(1e9);
    double prev = 0;
    for (double el_deg : {90.0, 45.0, 20.0, 10.0, 5.0, 2.0})
    {
        INFO("elevation " << el_deg);
        a.set_elevation_angle(el_deg * M_PI / 180);
        double v = a.get_attenuation();
        CHECK(std::isfinite(v));
        CHECK(v > prev);
        prev = v;
    }
    // Above 10 degrees, cosecant law
    a.set_elevation_angle(M_PI / 2);
    double z = a.get_attenuation();
    a.set_elevation_angle(30 * M_PI / 180);
    CHECK(a.get_attenuation() == Approx(z / std::sin(30 * M_PI / 180)).epsilon(1e-9));
}

TEST_CASE("ndsp AtmosphericGasesITU gives a sane zenith attenuation", "[dsp][channel_model]")
{
    AtmosphericGasesITU a(7.5);
    a.set_frequency(10e9);
    // Not valid below 1 degree, returns 0
    a.set_elevation_angle(0.5 * M_PI / 180);
    CHECK(a.get_attenuation() == 0);

    a.set_elevation_angle(M_PI / 2);
    double zenith = a.get_attenuation();
    CHECK(std::isfinite(zenith));
    // A few hundredths of a dB at 10 GHz for a standard atmosphere
    CHECK(zenith > 0.01);
    CHECK(zenith < 1.0);
}

// Bug: the ray-tracing of AtmosphericGasesITU (atmospheric_gases_itu.cpp:248-288, a() / alpha() /
// beta()) loses the elevation : the slant path attenuation at 10 degrees comes out about
// the same as at zenith, while a stratified atmosphere gives roughly zenith / sin(10 deg).
TEST_CASE("ndsp AtmosphericGasesITU slant attenuation grows at low elevation", "[dsp][channel_model][!shouldfail]")
{
    AtmosphericGasesITU a(7.5);
    a.set_frequency(10e9);
    a.set_elevation_angle(M_PI / 2);
    double zenith = a.get_attenuation();
    a.set_elevation_angle(10 * M_PI / 180);
    double low = a.get_attenuation();
    CHECK(low > 4 * zenith);
}

TEST_CASE("ndsp LinkMargin is the received power over the noise floor", "[dsp][channel_model]")
{
    LinkMargin lm;
    CHECK(lm.calc_link_margin(150, 2, 10, 0, -130) == Approx(0 - 150 + 2 + 10 + 130));
    CHECK(lm.calc_link_margin(100, 0, 0, -10, -140) == Approx(30));
}

TEST_CASE("ndsp ChannelModelLEO doppler, path loss, noise floor and link margin", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    const double f = 437e6, ptx_dbm = 30, nf = 2, temp = 290, bw = 50e3;
    ChannelModelLEO m(false, FREE_SPACE_PATH_LOSS, IMPAIRMENT_NONE, DOPPLER_SHIFT, IMPAIRMENT_NONE, IMPAIRMENT_NONE, true, 7.5, 15, 0, //
                      dipole(f), dipole(f), f, ptx_dbm, nf, temp, bw);
    CHECK(m.get_frequency() == f);
    CHECK(m.get_tx_power_dbm() == ptx_dbm);
    CHECK(m.get_noise_figure() == nf);
    CHECK(m.get_noise_temperature() == temp);
    CHECK(m.get_receiver_bandwidth() == bw);
    CHECK(m.get_polarization() == RHCP);

    // 1500 km, 20 degrees, receding at 6 km/s
    m.set_sat_info(1500, 20, 6);
    m.update();
    CHECK(m.get_slant_range() == 1500);
    CHECK(m.get_elevation() == 20);
    // Receding : negative Doppler shift
    CHECK(m.get_doppler_freq() == Approx(-6e3 * f / kC).epsilon(1e-9));
    double fspl = 20 * log10(4 * M_PI * 1500e3 * f / kC);
    CHECK(m.get_pathloss_attenuation() == Approx(fspl).margin(0.01));
    CHECK(m.get_total_attenuation() == Approx(fspl).margin(0.01));
    // kTB, in dBW
    double noise = 10 * log10(1.38e-23 * temp * bw) + nf;
    CHECK(m.calculate_noise_floor() == Approx(noise).epsilon(1e-9));
    CHECK(m.get_noise_floor() == Approx(noise).epsilon(1e-9));
    double margin = (ptx_dbm - 30) - fspl + 2.15 + 2.15 - noise;
    CHECK(m.get_link_margin() == Approx(margin).margin(0.01));

    // Approaching : positive shift
    m.set_sat_info(800, 60, -3);
    m.update();
    CHECK(m.get_doppler_freq() == Approx(3e3 * f / kC).epsilon(1e-9));
    CHECK(m.get_pathloss_attenuation() < fspl);
}

TEST_CASE("ndsp ChannelModelLEO adds the atmospheric attenuation to the total", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    const double f = 1e9;
    ChannelModelLEO m(false, FREE_SPACE_PATH_LOSS, IMPAIRMENT_NONE, IMPAIRMENT_NONE, ATMO_GASES_REGRESSION, IMPAIRMENT_NONE, false, 7.5, 15, 0, //
                      dipole(f), dipole(f), f, 30, 2, 290, 50e3);
    m.set_sat_info(1000, 30, 0);
    m.update();
    CHECK(m.get_atmo_attenuation() > 0);
    CHECK(m.get_total_attenuation() == Approx(m.get_atmo_attenuation() + m.get_pathloss_attenuation()).epsilon(1e-12));
    CHECK(m.get_doppler_freq() == 0);
}

TEST_CASE("ndsp ChannelModelLEO rejects invalid impairment selections", "[dsp][channel_model]")
{
    test_helpers::ValueGuard guard(antenna::GenericAntenna::base_unique_id);
    auto mk = [&](impairment_enum_t fspl, impairment_enum_t point, impairment_enum_t dop, impairment_enum_t atmo, impairment_enum_t prec)
    { ChannelModelLEO m(false, fspl, point, dop, atmo, prec, false, 7.5, 15, 0, dipole(1e9), dipole(1e9), 1e9, 30, 2, 290, 50e3); };
    CHECK_NOTHROW(mk(IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE));
    CHECK_NOTHROW(mk(FREE_SPACE_PATH_LOSS, ANTENNA_POINTING_LOSS, DOPPLER_SHIFT, ATMO_GASES_ITU, PRECIPITATION_ITU));
    CHECK_THROWS_AS(mk(IMPAIRMENT_NONE, IMPAIRMENT_NONE, FREE_SPACE_PATH_LOSS, IMPAIRMENT_NONE, IMPAIRMENT_NONE), std::runtime_error);
    CHECK_THROWS_AS(mk(DOPPLER_SHIFT, IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE), std::runtime_error);
    CHECK_THROWS_AS(mk(IMPAIRMENT_NONE, DOPPLER_SHIFT, IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE), std::runtime_error);
    CHECK_THROWS_AS(mk(IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE, DOPPLER_SHIFT, IMPAIRMENT_NONE), std::runtime_error);
    CHECK_THROWS_AS(mk(IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE, IMPAIRMENT_NONE, DOPPLER_SHIFT), std::runtime_error);
}
