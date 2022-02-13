#pragma once

#include <string>
#include <map>

namespace tle
{
    struct TLE
    {
        int norad;
        std::string name;
        std::string line1;
        std::string line2;
    };

    extern std::map<int, TLE> tle_map;

    void loadTLEs();
    TLE getTLEfromNORAD(int norad);
    void updateTLEs();
    void updateTLEsMT();
    void stopTLECleanMT();

    void fetchOrUpdateOne(int norad);
}