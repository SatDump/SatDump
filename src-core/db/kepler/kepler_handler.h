#pragma once

#include "common/tracking/tle.h"
#include "core/exception.h"
#include "db/db_handler.h"

namespace satdump
{
    struct KeplerData
    {
        int satellite_number;                 // Satellite number
        long element_number;                  // Element number
        std::string name;                     // Satellite Name
        std::string designator;               // International designator
        double epoch;                         // Epoch (Unix Time)
        double inclination;                   // Inclination
        double right_ascension;               // Right Ascension of the Ascending Node [Degrees]
        double eccentricity;                  // Eccentricity
        double argument_of_perigee;           // Argument of Perigee [Degrees]
        double mean_anomaly;                  // Mean Anomaly [Degrees]
        double mean_motion;                   // Mean Motion [Revs per day]
        double derivative_mean_motion;        // First Time Derivative of the Mean Motion divided by two
        double second_derivative_mean_motion; // Second Time Derivative of Mean Motion divided by six
        double bstar_drag_term;               // BSTAR drag term
        int revolutions_at_epoch;             // Number of revolutions around Earth at epoch
    };

    class KeplerDBHandler : public DBHandlerBase
    {
    private:
        struct AutoUpdateKeplersEvent
        {
        };

    public:
        KeplerDBHandler(std::shared_ptr<DBHandler> h);

        void init();

        void putKepler(KeplerData kep);

        //  void updateTLEDatabase();

        //  std::optional<TLE> get_from_norad(int norad);
        //  std::optional<TLE> get_from_norad_time(int norad, time_t timestamp);

    private:
        // std::vector<TLE> get_all_tles();

    public:
        // std::vector<TLE> all; // TODOREWORK remove! Temporary!
    };
} // namespace satdump