#include "channel_model_leo.h"
#include "../attenuation/atmospheric_gases_itu.h"
#include "../attenuation/atmospheric_gases_regression.h"
#include "../attenuation/free_space_path_loss.h"
#include "common/geodetic/geodetic_coordinates.h"

ChannelModelLEO::ChannelModelLEO(bool uplink_mode,                                    //                   //
                                 impairment_enum_t fspl_attenuation_enum,             //
                                 impairment_enum_t pointing_attenuation_enum,         //
                                 impairment_enum_t doppler_shift_enum,                //
                                 impairment_enum_t atmo_gases_attenuation,            //
                                 impairment_enum_t precipitation_attenuation,         //
                                 bool enable_link_margin,                             //
                                 double surface_watervap_density,                     //
                                 double temperature,                                  //
                                 double rainfall_rate,                                //
                                 std::shared_ptr<antenna::GenericAntenna> tx_antenna, //
                                 std::shared_ptr<antenna::GenericAntenna> rx_antenna, //
                                 double frequency,                                    //
                                 double power_tx,                                     //
                                 double receiver_noise_figure,                        //
                                 double receiver_noise_temperature,                   //
                                 double receiver_bandwidth                            //
                                 )
    : d_doppler_shift_enum(doppler_shift_enum),           //
      d_uplink_mode(uplink_mode),                         //
      d_frequency(frequency),                             //
      d_tx_power_dbm(power_tx),                           //
      d_rx_noise_figure(receiver_noise_figure),           //
      d_rx_noise_temperature(receiver_noise_temperature), //
      d_rx_bandwidth(receiver_bandwidth),                 //
      rx_antenna(rx_antenna),                             //
      tx_antenna(tx_antenna)
{
    switch (d_doppler_shift_enum)
    {
    case DOPPLER_SHIFT:
    case IMPAIRMENT_NONE:
        break;
    default:
        throw std::runtime_error("Invalid Doppler shift enumeration!");
    }

    switch (enable_link_margin)
    {
    case true:
        d_link_margin = std::make_shared<LinkMargin>();
        d_link_margin_db = std::numeric_limits<double>::infinity();
        break;
    case false:
        break;
    default:
        throw std::runtime_error("Invalid atmospheric gases attenuation!");
    }

    switch (atmo_gases_attenuation)
    {
    case ATMO_GASES_ITU:
        d_atmo_gases_attenuation = std::make_shared<AtmosphericGasesITU>(surface_watervap_density);
        break;
    case ATMO_GASES_REGRESSION:
        d_atmo_gases_attenuation = std::make_shared<AtmosphericGasesRegression>(surface_watervap_density, temperature);
        break;
    case IMPAIRMENT_NONE:
        break;
    default:
        throw std::runtime_error("Invalid atmospheric gases attenuation!");
    }

    switch (precipitation_attenuation)
    {
    case PRECIPITATION_ITU:
    case PRECIPITATION_CUSTOM:
        // TODO d_precipitation_attenuation = attenuation::precipitation_itu::make(d_rainfall_rate, d_tracker->get_lontitude(), d_tracker->get_latitude(), d_tracker->get_altitude(),
        // (impairment_enum_t)precipitation_enum);
        break;
    case IMPAIRMENT_NONE:
        break;
    default:
        throw std::runtime_error("Invalid precipitation attenuation!");
    }

    switch (fspl_attenuation_enum)
    {
    case FREE_SPACE_PATH_LOSS:
        d_fspl_attenuation = std::make_shared<FreeSpacePathLossAttenuation>();
        break;
    case IMPAIRMENT_NONE:
        break;
    default:
        throw std::runtime_error("Invalid free-space path loss enumeration!");
    }

    switch (pointing_attenuation_enum)
    {
    case ANTENNA_POINTING_LOSS:
        // TODO d_pointing_loss_attenuation = attenuation::antenna_pointing_loss::make(get_tracker_antenna(), get_satellite_antenna());
        break;
    case IMPAIRMENT_NONE:
        break;
    default:
        throw std::runtime_error("Invalid antenna pointing loss enumeration!");
    }
}

ChannelModelLEO::~ChannelModelLEO() {}

double ChannelModelLEO::calculate_doppler_shift(double velocity) { return (-1e3 * velocity * get_frequency()) / LIGHT_SPEED; }

void ChannelModelLEO::update()
{
    d_doppler_shift = calculate_doppler_shift(d_sat_range_rate);

    // Update everything
    {
        double elevation_radians = d_sat_elev * DEG_TO_RAD;
        double range = d_sat_range;
        uint8_t polarization = get_polarization();

        if (d_atmo_gases_attenuation)
        {
            d_atmo_gases_attenuation->set_elevation_angle(elevation_radians);
            d_atmo_gases_attenuation->set_frequency(get_frequency());
            d_atmo_gases_attenuation->set_polarization(polarization);
            d_atmo_gases_attenuation->set_slant_range(range);
        }

        if (d_precipitation_attenuation)
        {
            d_precipitation_attenuation->set_elevation_angle(elevation_radians);
            d_precipitation_attenuation->set_frequency(get_frequency());
            d_precipitation_attenuation->set_polarization(polarization);
            d_precipitation_attenuation->set_slant_range(range);
        }

        if (d_fspl_attenuation)
        {
            d_fspl_attenuation->set_elevation_angle(elevation_radians);
            d_fspl_attenuation->set_frequency(get_frequency());
            d_fspl_attenuation->set_polarization(polarization);
            d_fspl_attenuation->set_slant_range(range);
        }

        if (d_pointing_loss_attenuation)
        {
            d_pointing_loss_attenuation->set_elevation_angle(elevation_radians);
            d_pointing_loss_attenuation->set_frequency(get_frequency());
            d_pointing_loss_attenuation->set_polarization(polarization);
            d_pointing_loss_attenuation->set_slant_range(range);
        }
    }

    calculate_total_attenuation();
}

double ChannelModelLEO::calculate_total_attenuation()
{
    d_total_attenuation = 0;

    if (d_atmo_gases_attenuation)
    {
        d_atmo_attenuation = d_atmo_gases_attenuation->get_attenuation();
        d_total_attenuation += d_atmo_attenuation;
    }
    if (d_precipitation_attenuation)
    {
        d_rainfall_attenuation = d_precipitation_attenuation->get_attenuation();
        d_total_attenuation += d_rainfall_attenuation;
    }
    if (d_fspl_attenuation)
    {
        d_pathloss_attenuation = d_fspl_attenuation->get_attenuation();
        d_total_attenuation += d_pathloss_attenuation;
    }
    if (d_pointing_loss_attenuation)
    {
        d_pointing_attenuation = d_pointing_loss_attenuation->get_attenuation();
        d_total_attenuation += d_pointing_attenuation;
    }
    if (d_link_margin)
    {
        estimate_link_margin();
    }

#if 0 // TODO
    LEO_DEBUG("Time: %s | Slant Range (km): %f | Elevation (degrees): %f | \
    Path Loss (dB): %f | Atmospheric Loss (dB): %f | Rainfall Loss\
    (dB): %f | Pointing Loss (dB): %f |  Doppler (Hz): %f | \
    Link Margin (dB): %f",
              d_tracker->get_elapsed_time().ToString().c_str(), d_slant_range, d_elev, d_pathloss_attenuation, d_atmo_attenuation, d_rainfall_attenuation, d_pointing_attenuation, d_doppler_shift,
              d_link_margin_db);
#endif

    return d_total_attenuation;
}

void ChannelModelLEO::estimate_link_margin()
{
    /**
     * TODO: Fix hardcoded TX power and bandwidth. Also caution because losses
     * should be negative.
     */
    d_link_margin_db = d_link_margin->calc_link_margin(d_total_attenuation, tx_antenna->get_gain(), rx_antenna->get_gain(), get_tx_power_dbm() - 30, calculate_noise_floor());
}

double ChannelModelLEO::calculate_noise_floor()
{
    d_noise_floor = 10 * log10(1.38e-23 * get_noise_temperature() * 1e3) + get_noise_figure() + 10 * log10(get_receiver_bandwidth()) - 30.0;
    return d_noise_floor;
}

double ChannelModelLEO::get_frequency() { return d_frequency; }

uint8_t ChannelModelLEO::get_polarization() { return tx_antenna->get_polarization(); }

double ChannelModelLEO::get_tx_power_dbm() { return d_tx_power_dbm; }
double ChannelModelLEO::get_receiver_bandwidth() { return d_rx_bandwidth; }
double ChannelModelLEO::get_noise_temperature() { return d_rx_noise_temperature; }
double ChannelModelLEO::get_noise_figure() { return d_rx_noise_figure; }
double ChannelModelLEO::get_noise_floor() { return d_noise_floor; }

double ChannelModelLEO::get_doppler_freq() { return d_doppler_shift; }
double ChannelModelLEO::get_atmo_attenuation() { return d_atmo_attenuation; }
double ChannelModelLEO::get_rainfall_attenuation() { return d_rainfall_attenuation; }
double ChannelModelLEO::get_pathloss_attenuation() { return d_pathloss_attenuation; }
double ChannelModelLEO::get_pointing_attenuation() { return d_pointing_attenuation; }
double ChannelModelLEO::get_total_attenuation() { return d_total_attenuation; }
double ChannelModelLEO::get_slant_range() { return d_sat_range; }
double ChannelModelLEO::get_elevation() { return d_sat_elev; }
double ChannelModelLEO::get_link_margin() { return d_link_margin_db; }