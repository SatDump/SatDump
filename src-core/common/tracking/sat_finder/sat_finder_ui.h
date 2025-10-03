#pragma once

#include "libs/predict/predict.h"

#include <mutex>
#include <thread>
#include <list>
#include <string>

namespace satdump
{
    class SatFinder {

    private:
        struct Satellite {
            std::string name;
            double azimuth;
            double elevation;
            double distance;
        };

        predict_observer_t *observer_station = nullptr;
        std::list<Satellite> sats_in_sight;
        std::list<std::string> blacklist;

    private: 
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;
        double target_az = -1;  
        double target_el = -1;    
        double tolerance = 3;

    private: //Thread
        std::mutex mutex;
        std::thread satfinder_thread;
        bool satfinder_should_run = true;
        double update_period = 1;

        void satfinder_run();

    private: // Helper func
        template<typename IteratorT>
        bool containsSubstring(const std::string& str, IteratorT first, IteratorT last) {
            for (; first != last; ++first) {
                if (str.find(*first) != std::string::npos) {
                    return true;
                }
            }
            return false;
        }

    public:
        SatFinder();
        ~SatFinder();

        void setQTH(double lon, double lat, double alt);
        void setTarget(double azimuth, double elevation);

        void saveConfig();
        void loadConfig();

        void render();
    };

};