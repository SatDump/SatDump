#define cimg_use_jpeg
#include "poseidon_reader.h"
#include "resources.h"
#include "tle.h"

namespace jason3
{
    namespace poseidon
    {
        time_t parseTime(ccsds::ccsds_1_0_jason::CCSDSPacket &pkt)
        {
            uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
            uint32_t milliseconds_of_day = pkt.payload[2] << 24 | pkt.payload[3] << 16 | pkt.payload[4] << 8 | pkt.payload[5];
            //uint16_t milliseconds_of_millisecond = pkt.payload[6] << 8 | pkt.payload[7];

            return (16617 + days) * 86400 + milliseconds_of_day;
        }

        PoseidonReader::PoseidonReader()
        {
            tle::TLE jason_tle = tle::getTLEfromNORAD(41240); // This can be safely harcoded, only 1 satellite

            jason3_object = predict_parse_tle(jason_tle.line1.c_str(), jason_tle.line2.c_str());

            map_image_height.load_jpeg(resources::getResourcePath("maps/nasa.jpg").c_str());
            map_image_scatter.load_jpeg(resources::getResourcePath("maps/nasa.jpg").c_str());
        }

        PoseidonReader::~PoseidonReader()
        {
            delete jason3_object;
        }

        void PoseidonReader::work(ccsds::ccsds_1_0_jason::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 930)
                return;

            // We need to know where the satellite was when that packet was created
            time_t currentTime = parseTime(packet);
            predict_orbit(jason3_object, &jason3_orbit, predict_to_julian(currentTime));

            // Scale to the map
            int map_height = map_image_height.height();
            int map_width = map_image_height.width();
            int imageLat = map_height - ((90.0f + (jason3_orbit.latitude * 180.0f / M_PI)) / 180.0f) * map_height;
            int imageLon = ((jason3_orbit.longitude * 180.0f / M_PI) / 360.0f) * map_width + (map_width / 2);
            if (imageLon >= map_width)
                imageLon -= map_width;

            // Here is a very naive way of detecting when we got a radar echo...
            // And how stable it was...
            //
            // The first just checks the position of the first value over a thresold,
            // while the second counts all values over that thresold.
            // All 8 samples are averaged to smooth the data out.
            //
            // My first time dealing with this kind of data so well...
            // Hopefulyl not too bad :-)
            int thresold = 10;
            int firstOverThresoldAvg = 0, countOverThresoldAvg = 0;
            for (int y = 0; y < 8; y++)
            {
                int firstOverThresold = 0;
                for (int i = 0; i < 104; i++)
                {
                    if (packet.payload[102 + (104 * y) + i] > thresold)
                    {
                        firstOverThresold = i;
                        break;
                    }
                }

                int countOverThresold = 0;
                for (int i = 0; i < 104; i++)
                {
                    if (packet.payload[102 + (104 * y) + i] > thresold)
                        countOverThresold++;
                }

                firstOverThresoldAvg += firstOverThresold;
                countOverThresoldAvg += countOverThresold;
            }

            // Average
            firstOverThresoldAvg /= 8;
            countOverThresoldAvg /= 8;

            // "Scatter" samples
            int sampleScatter = 65 + ((float)countOverThresoldAvg / 104.0f) * 255;
            if (sampleScatter < 0)
                sampleScatter = 0;
            if (sampleScatter > 255)
                sampleScatter = 255;

            // Height samples
            int sampleHeight = 200 - ((float)firstOverThresoldAvg / 104.0f) * 255;
            if (sampleHeight < 0)
                sampleHeight = 0;
            if (sampleHeight > 255)
                sampleHeight = 255;

            // Write on the map
            const unsigned char colorHeight[] = {(unsigned char)sampleHeight, (unsigned char)std::max(0, 255 - sampleHeight), 0};
            map_image_height.draw_circle(imageLon, imageLat, 2, colorHeight);

            // Write on the map
            const unsigned char colorScatter[] = {(unsigned char)sampleScatter, (unsigned char)std::max(0, 255 - sampleScatter), 0};
            map_image_scatter.draw_circle(imageLon, imageLat, 2, colorScatter);
        }

        cimg_library::CImg<unsigned char> PoseidonReader::getImageHeight()
        {
            return map_image_height;
        }

        cimg_library::CImg<unsigned char> PoseidonReader::getImageScatter()
        {
            return map_image_scatter;
        }
    } // namespace modis
} // namespace eos