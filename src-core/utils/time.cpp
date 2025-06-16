#include "time.h"
#include <iomanip>
#include <sstream>

namespace satdump
{
    double getTime()
    {
        auto time = std::chrono::system_clock::now();
        auto since_epoch = time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
        return millis.count() / 1e3;
    }

    std::string timestamp_to_string(double timestamp, bool local)
    {
        if (timestamp < 0)
            timestamp = 0;

        time_t tttime = timestamp;
        std::tm *timeReadable = (local ? localtime(&tttime) : gmtime(&tttime));
        std::stringstream timestamp_string;
        std::string timezone_string = "";

        if (timeReadable == nullptr)
        {
            return std::to_string(timestamp);
        }

        if (local)
        {
#ifdef _WIN32
            size_t tznameSize = 0;
            char *tznameBuffer = NULL;
            timezone_string = " ";
            _get_tzname(&tznameSize, NULL, 0, timeReadable->tm_isdst);
            if (tznameSize > 0)
            {
                if (nullptr != (tznameBuffer = (char *)(malloc(tznameSize))))
                {
                    if (_get_tzname(&tznameSize, tznameBuffer, tznameSize, timeReadable->tm_isdst) == 0)
                        for (size_t i = 0; i < tznameSize; i++)
                            if (std::isupper(tznameBuffer[i]))
                                timezone_string += tznameBuffer[i];

                    free(tznameBuffer);
                }
            }
            if (timezone_string == " ")
                timezone_string = " Local";
#else
            timezone_string = " " + std::string(timeReadable->tm_zone);
#endif
        }

        timestamp_string << std::setfill('0')                               //
                         << timeReadable->tm_year + 1900 << "/"             //
                         << std::setw(2) << timeReadable->tm_mon + 1 << "/" //
                         << std::setw(2) << timeReadable->tm_mday << " "    //
                         << std::setw(2) << timeReadable->tm_hour << ":"    //
                         << std::setw(2) << timeReadable->tm_min << ":"     //
                         << std::setw(2) << timeReadable->tm_sec            //
                         << timezone_string;

        return timestamp_string.str();
    }
} // namespace satdump