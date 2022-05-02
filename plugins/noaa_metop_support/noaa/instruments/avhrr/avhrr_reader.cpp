#include "avhrr_reader.h"
#include <ctime>

namespace noaa
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader(bool gac) : gac_mode(gac), width(gac_mode ? 409 : 2048)
        {
            for (int i = 0; i < 5; i++)
                channels[i] = new unsigned short[(gac_mode ? 40000 : 14000) * width];
            lines = 0;
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

        AVHRRReader::~AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }

        void AVHRRReader::work(uint16_t *buffer)
        {
            // Parse timestamp
            int day_of_year = buffer[8] >> 1;
            uint64_t milliseconds = (buffer[9] & 0x7F) << 20 | (buffer[10] << 10) | buffer[11];
            double timestamp = dayYearValue + (day_of_year * 86400) + double(milliseconds) / 1000.0;
            timestamps.push_back(timestamp);

            spacecraft_ids.push_back(((buffer[6] & 0x078) >> 3) & 0x000F);

            int pos = gac_mode ? 1182 : 750; // AVHRR Data

            for (int channel = 0; channel < 5; channel++)
            {
                for (int i = 0; i < width; i++)
                {
                    uint16_t pixel = buffer[pos + channel + i * 5];
                    channels[channel][lines * width + i] = pixel * 60;
                }
            }

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> AVHRRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], width, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa
