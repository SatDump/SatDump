#define cimg_use_jpeg
#include "sem_reader.h"
#include "resources.h"
#include "tle.h"
#include "common/ccsds/ccsds_time.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace metop
{
    namespace sem
    {
        SEMReader::SEMReader(int norad)
        {
            tle::TLE jason_tle = tle::getTLEfromNORAD(norad); // This can be safely harcoded, only 1 satellite

            metop_object = predict_parse_tle(jason_tle.line1.c_str(), jason_tle.line2.c_str());

            map_image.load_jpeg(resources::getResourcePath("maps/nasa.jpg").c_str());
        }

        SEMReader::~SEMReader()
        {
            delete metop_object;
        }

        void SEMReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 656)
                return;

            // We need to know where the satellite was when that packet was created
            time_t currentTime = ccsds::parseCCSDSTime(packet, 10957);
            predict_orbit(metop_object, &metop_orbit, predict_to_julian(currentTime));

            // Scale to the map
            int map_height = map_image.height();
            int map_width = map_image.width();
            int imageLat = map_height - ((90.0f + (metop_orbit.latitude * 180.0f / M_PI)) / 180.0f) * map_height;
            int imageLon = ((metop_orbit.longitude * 180.0f / M_PI) / 360.0f) * map_width + (map_width / 2);
            if (imageLon >= map_width)
                imageLon -= map_width;

            // Get all samples, and average themu
            float count = 0;
            for (int i = 0; i < 640; i += 40)
            {
                float tempCount = 0;
                for (int j = 0; j < 40; j++)
                {
                    tempCount += packet.payload[i + j] ^ 0xFF;
                }
                count += tempCount / 40.0f;
            }
            count /= 16.0f;

            // Scale it
            count -= 16;
            count *= 70;

            if (count > 255)
                count = 255;

            // Write on the map!
            uint8_t color0[] = {(uint8_t)count, (uint8_t)std::max<int>(0, 255 - count), 0};
            map_image.draw_circle(imageLon, imageLat, 4, color0, true);
        }

        image::Image<uint8_t> SEMReader::getImage()
        {
            return map_image;
        }
    } // namespace modis
} // namespace eos