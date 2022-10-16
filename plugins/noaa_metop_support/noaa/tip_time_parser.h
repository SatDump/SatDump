#pragma once

#include <ctime>
#include <cstdint>

namespace noaa
{
    class TIPTimeParser
    {
        time_t dayYearValue = 0;

    public:
        TIPTimeParser()
        {
            time_t curr_time = time(NULL);
            struct tm timeinfo_struct;
#ifdef _WIN32
            memcpy(&timeinfo_struct, gmtime(&curr_time), sizeof(struct tm));
#else
            gmtime_r(&curr_time, &timeinfo_struct);
#endif

            // Reset to be year
            timeinfo_struct.tm_mday = 0;
            timeinfo_struct.tm_wday = 0;
            timeinfo_struct.tm_yday = 0;
            timeinfo_struct.tm_mon = 0;

            dayYearValue = mktime(&timeinfo_struct);
            dayYearValue = dayYearValue - (dayYearValue % 86400);
        }

        inline double getTimestamp(int doy, uint64_t millisec) { return dayYearValue + (doy * 86400) + double(millisec) / 1000.0; }
    };

}