#pragma once

#include <ctime>
#include <cstdint>
#include <iostream>

namespace noaa
{
    class TIPTimeParser
    {
        time_t dayYearValue = 0;

    public:
        TIPTimeParser(int year_ov)
        {
            time_t curr_time = time(NULL);
            struct tm timeinfo_struct;
#ifdef _WIN32
            memcpy(&timeinfo_struct, gmtime(&curr_time), sizeof(struct tm));
#else
            gmtime_r(&curr_time, &timeinfo_struct);
#endif

            // Reset to be year
            timeinfo_struct.tm_mday = 1;
            timeinfo_struct.tm_mon = 0;
            timeinfo_struct.tm_sec = 0;
            timeinfo_struct.tm_min = 0;
            timeinfo_struct.tm_hour = 0;

            if (year_ov != -1) // year override
                timeinfo_struct.tm_year = year_ov - 1900;

#ifdef _WIN32
            dayYearValue = _mkgmtime(&timeinfo_struct);
#else
            dayYearValue = timegm(&timeinfo_struct);
#endif
        }

        inline double getTimestamp(int doy, uint64_t millisec) { return dayYearValue + ((doy - 1) * 86400) + double(millisec) / 1000.0; }
    };

}