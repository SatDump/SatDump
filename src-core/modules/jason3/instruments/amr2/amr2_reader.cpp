#define cimg_use_jpeg
#include "amr2_reader.h"
#include "resources.h"
#include "tle.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace jason3
{
    namespace amr2
    {
        time_t parseTime(ccsds::ccsds_1_0_jason::CCSDSPacket &pkt)
        {
            uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
            uint32_t milliseconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
            //uint16_t milliseconds_of_millisecond = pkt.payload[6] << 8 | pkt.payload[7];

            return (16617 + days) * 86400 + milliseconds_of_day;
        }

        AMR2Reader::AMR2Reader()
        {
            tle::TLE jason_tle = tle::getTLEfromNORAD(41240); // This can be safely harcoded, only 1 satellite

            jason3_object = predict_parse_tle(jason_tle.line1.c_str(), jason_tle.line2.c_str());

            for (int i = 0; i < 3; i++)
            {
                map_image[i].load_jpeg(resources::getResourcePath("maps/nasa.jpg").c_str());
                imageBuffer[i] = new unsigned short[600 * 100000];
            }

            lines = 0;
        }

        AMR2Reader::~AMR2Reader()
        {
            delete jason3_object;

            for (int i = 0; i < 3; i++)
                delete[] imageBuffer[i];
        }

        void AMR2Reader::work(ccsds::ccsds_1_0_jason::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 186)
                return;

            // We need to know where the satellite was when that packet was created
            time_t currentTime = parseTime(packet);
            predict_orbit(jason3_object, &jason3_orbit, predict_to_julian(currentTime));

            // Scale to the map
            int map_height = map_image[0].height();
            int map_width = map_image[0].width();
            int imageLat = map_height - ((90.0f + (jason3_orbit.latitude * 180.0f / M_PI)) / 180.0f) * map_height;
            int imageLon = ((jason3_orbit.longitude * 180.0f / M_PI) / 360.0f) * map_width + (map_width / 2);
            if (imageLon >= map_width)
                imageLon -= map_width;

            // Get all samples... Maybe we should average that later?
            uint16_t sample0 = packet.payload[38] << 8 | packet.payload[37];
            uint16_t sample1 = packet.payload[38 + 2] << 8 | packet.payload[37 + 2];
            uint16_t sample2 = packet.payload[38 + 4] << 8 | packet.payload[37 + 4];

            // Scale and clamp them
            sample0 -= 11760;
            sample0 /= 4;
            if (sample0 > 255)
                sample0 = 255;

            sample1 -= 13300;
            sample1 /= 4;
            if (sample1 > 255)
                sample1 = 255;

            sample2 -= 12500;
            sample2 /= 4;
            if (sample2 > 255)
                sample2 = 255;

            // Write on the map!
            const unsigned char color0[] = {(unsigned char)sample0, (unsigned char)std::max(0, 255 - sample0), 0};
            map_image[0].draw_circle(imageLon, imageLat, 2, color0);

            const unsigned char color1[] = {(unsigned char)sample1, (unsigned char)std::max(0, 255 - sample1), 0};
            map_image[1].draw_circle(imageLon, imageLat, 2, color1);

            const unsigned char color2[] = {(unsigned char)sample2, (unsigned char)std::max(0, 255 - sample2), 0};
            map_image[2].draw_circle(imageLon, imageLat, 2, color2);

            // Also write them as a raw images
            for (int i = 0; i < 16; i++)
            {
                imageBuffer[0][lines * 16 + i] = packet.payload[38 + i * 6] << 8 | packet.payload[37 + i * 6];
                imageBuffer[1][lines * 16 + i] = packet.payload[38 + i * 6 + 2] << 8 | packet.payload[37 + i * 6 + 2];
                imageBuffer[2][lines * 16 + i] = packet.payload[38 + i * 6 + 4] << 8 | packet.payload[37 + i * 6 + 4];
            }

            lines++;
        }

        cimg_library::CImg<unsigned char> AMR2Reader::getImage(int channel)
        {
            return map_image[channel];
        }

        cimg_library::CImg<unsigned short> AMR2Reader::getImageNormal(int channel)
        {
            return cimg_library::CImg<unsigned short>(imageBuffer[channel], 16, lines);
        }
    } // namespace modis
} // namespace eos