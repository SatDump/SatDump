#pragma once

#include "../antenna/generic.h"
#include "../attenuation/generic.h"
#include "../const.h"
#include "link_margin.h"
#include "nlohmann/json.hpp"
#include <memory>

class ChannelModelLEO
{
private:
    // Antenna and other RF settings
    const double d_frequency;
    const bool d_uplink_mode;
    const double d_rx_noise_figure;
    const double d_rx_noise_temperature;
    const double d_tx_power_dbm;
    const double d_rx_bandwidth;

    // Calculature noise floor
    double d_noise_floor;

    // Satellite info. Range in KM, elevation in degrees
    double d_sat_range;
    double d_sat_elev;
    double d_sat_range_rate;

    // double d_surface_watervap_density;
    // double d_temperature;
    // double d_rainfall_rate;

    // Calculated attenuation & doppler & link margin
    double d_doppler_shift;
    double d_atmo_attenuation;
    double d_rainfall_attenuation;
    double d_pathloss_attenuation;
    double d_pointing_attenuation;
    double d_total_attenuation;
    double d_link_margin_db;

    const std::shared_ptr<antenna::GenericAntenna> tx_antenna;
    const std::shared_ptr<antenna::GenericAntenna> rx_antenna;

    const impairment_enum_t d_doppler_shift_enum;
    std::shared_ptr<GenericAttenuation> d_atmo_gases_attenuation;
    std::shared_ptr<GenericAttenuation> d_precipitation_attenuation;
    std::shared_ptr<GenericAttenuation> d_fspl_attenuation;
    std::shared_ptr<GenericAttenuation> d_pointing_loss_attenuation;

    std::shared_ptr<LinkMargin> d_link_margin;

    /*!
     * Calculate the free-space path-loss attenuation for a
     * given slant range.
     * \param slant_range The distance in kilometers between the observer and the
     *      satellite.
     * @return a double representing the attenuation in dB.
     */
    double calculate_free_space_path_loss(double slant_range);

    /*!
     * Calculate the Doppler frequency shift for a given satellite range rate.
     * \param velocity The range rate of the satellite expressed in
     * kilometers/sec.
     * @return a double representing the frequency shift in hertz.
     */
    double calculate_doppler_shift(double velocity);

    double calculate_total_attenuation();

public:
    ChannelModelLEO(bool uplink_mode,                                    //                   //
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
    );

    ~ChannelModelLEO();

public:
    void set_sat_info(double range, double elev, double range_rate)
    {
        d_sat_range = range;
        d_sat_elev = elev;
        d_sat_range_rate = range_rate;
    }

    void update();
    nlohmann::json get_channel_model_settings();

public:
    double get_frequency();
    uint8_t get_polarization();
    double get_tx_power_dbm();
    double get_receiver_bandwidth();
    double get_noise_temperature();
    double get_noise_figure();
    double get_noise_floor();

    void estimate_link_margin();
    double calculate_noise_floor();
    double get_doppler_freq();
    double get_atmo_attenuation();
    double get_rainfall_attenuation();
    double get_pathloss_attenuation();
    double get_pointing_attenuation();
    double get_total_attenuation();
    double get_slant_range();
    double get_elevation();
    double get_link_margin();
};