#pragma once

#include "common/tracking/tle.h"
#include "core/exception.h"
#include "db/db_handler.h"
#include <mutex>
#include <vector>

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

    TLE keplerToTle(KeplerData kep);
    bool ccsdsOmmToKepler(std::string omm_line, KeplerData &kep_out);
    bool tleToKepler(TLE tle, KeplerData &kep_out);

    std::vector<KeplerData> parseCcsdsOmmFile(std::string fileCont);

    std::vector<KeplerData> tryFetchOMMFileFromURL(std::string url_str);
    std::vector<KeplerData> tryFetchSingleOMMwithNorad(int norad);

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
        bool getKepler(KeplerData &kep, int norad, time_t time = -1);
        std::vector<KeplerData> getAllNewestKepler();

        void autoUpdateKeplers();
        void updateKeplerDatabase();

    public: // Legacy
        std::optional<TLE> get_from_norad(int norad)
        {
            KeplerData kep;
            if (getKepler(kep, norad))
                return keplerToTle(kep);
            else
                return {};
        }

        std::optional<TLE> get_from_norad_time(int norad, time_t timestamp)
        {
            // TODOTLE Space-track

            KeplerData kep;
            if (getKepler(kep, norad, timestamp))
                return keplerToTle(kep);
            else
                return {};
        }

    private:
        std::vector<TLE> get_all_tles()
        {
            std::vector<TLE> tles;
            auto keps = getAllNewestKepler();
            for (auto &kep : keps)
                tles.push_back(keplerToTle(kep));
            return tles;
        }

    public:
        std::vector<TLE> all_;
    };
} // namespace satdump