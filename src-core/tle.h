#pragma once

#include <string>

namespace tle
{
    struct TLE
    {
        int norad;
        std::string name;
        std::string line1;
        std::string line2;
    };

    void loadTLEs();
    TLE getTLEfromNORAD(int norad);
    void updateTLEs();
    void updateTLEsMT();
}