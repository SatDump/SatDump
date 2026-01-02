#include "link_margin.h"

LinkMargin::~LinkMargin() {}

double LinkMargin::calc_link_margin(double total_loss_db, double satellite_antenna_gain, double tracker_antenna_gain, double transmission_power_dbw, double noise_floor)
{
    double signal_power_gs_input = transmission_power_dbw - total_loss_db + satellite_antenna_gain + tracker_antenna_gain;

    // double gs_noise_power = BOLTZMANS_CONST + 10 * log10(GS_NOISE_TEMP) + 10 * log10(gs_receiver_bw);
    double gs_snr = signal_power_gs_input - noise_floor;

    // TODO: Fix dynamic minimum required modulation SNR
    return  gs_snr;
}