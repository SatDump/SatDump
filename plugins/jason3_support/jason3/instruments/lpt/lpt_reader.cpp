#define cimg_use_jpeg
#include "lpt_reader.h"
#include "resources.h"
#include "tle.h"
#include "common/ccsds/ccsds_time.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace jason3
{
    namespace lpt
    {
        LPTReader::LPTReader(int start_byte, int channel_count, int pkt_size) : start_byte(start_byte),
                                                                                channel_count(channel_count),
                                                                                pkt_size(pkt_size)
        {
            tle::TLE jason_tle = tle::getTLEfromNORAD(41240); // This can be safely harcoded, only 1 satellite

            jason3_object = predict_parse_tle(jason_tle.line1.c_str(), jason_tle.line2.c_str());

            map_image = new image::Image<uint8_t>[channel_count];
            for (int i = 0; i < channel_count; i++)
            {
                map_image[i].load_jpeg(resources::getResourcePath("maps/nasa.jpg").c_str());
            }
        }

        LPTReader::~LPTReader()
        {
            delete jason3_object;
            delete[] map_image;
        }

        void LPTReader::work(ccsds::CCSDSPacket &packet)
        {
            if ((int)packet.payload.size() < pkt_size)
                return;

            // We need to know where the satellite was when that packet was created
            time_t currentTime = ccsds::parseCCSDSTime(packet, 16743, 1);
            predict_orbit(jason3_object, &jason3_orbit, predict_to_julian(currentTime));

            // Scale to the map
            int map_height = map_image[0].height();
            int map_width = map_image[0].width();
            int imageLat = map_height - ((90.0f + (jason3_orbit.latitude * 180.0f / M_PI)) / 180.0f) * map_height;
            int imageLon = ((jason3_orbit.longitude * 180.0f / M_PI) / 360.0f) * map_width + (map_width / 2);
            if (imageLon >= map_width)
                imageLon -= map_width;

            for (int ch = 0; ch < channel_count; ch++)
            {
                // Get all samples... Maybe we should average that later?
                uint16_t sample = packet.payload[start_byte + 2 * ch + 0] << 8 | packet.payload[start_byte + 2 * ch + 1];

                sample /= 4;
                if (sample > 255)
                    sample = 255;

                // Write on the map!
                unsigned char color0[] = {(unsigned char)sample, (unsigned char)std::max(0, 255 - sample), 0};
                map_image[ch].draw_circle(imageLon, imageLat, 2, color0);
            }
        }

        image::Image<uint8_t> LPTReader::getImage(int channel)
        {
            return map_image[channel];
        }
    } // namespace modis
} // namespace eos