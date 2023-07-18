#pragma once

#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
#include <vector>

namespace satdump
{
    class TrackingWidget
    {
    private:
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

        bool has_tle = false;

        bool horizons_mode = false;

        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;
        predict_observer_t *observer_station;
        predict_observation observation_pos;
        double next_aos_time, next_los_time;

        std::string satoptionstr;
        int current_satellite = 0;

        std::vector<std::pair<float, float>> upcoming_pass_points;
        void updateNextPass();

        double getTime();

        double last_horizons_fetch_time = 0;

        struct HorizonsV
        {
            double timestamp;
            float az, el;
        };

        std::vector<std::string> horizons_ids = {"STEREO-A", "STEREO-B", "JUICE", "JWST", "SOHO", "Sun", "CH-3"};
        std::string horizonsoptionstr;
        int current_horizons = 0;
        std::vector<HorizonsV> horizons_data;

        void loadHorizons();

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
    };
}
