#pragma once

class LinkMargin
{

public:
    ~LinkMargin();

    double calc_link_margin(double total_loss_db, double satellite_antenna_gain, double tracker_antenna_gain, double transmission_power_dbw, double noise_floor);
};