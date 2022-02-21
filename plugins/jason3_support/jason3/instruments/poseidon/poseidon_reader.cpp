#define cimg_use_jpeg
#include "poseidon_reader.h"
#include "resources.h"
#include "tle.h"
#include "common/ccsds/ccsds_time.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif

namespace jason3
{
    namespace poseidon
    {
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

        void PoseidonReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 930)
                return;

            // We need to know where the satellite was when that packet was created
            time_t currentTime = ccsds::parseCCSDSTime(packet, 16743, 1);
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
                    if (packet.payload[96 + (104 * y) + i] > thresold)
                    {
                        firstOverThresold = i;
                        break;
                    }
                }

                int countOverThresold = 0;
                for (int i = 0; i < 104; i++)
                {
                    if (packet.payload[96 + (104 * y) + i] > thresold)
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
            unsigned char colorHeight[] = {(unsigned char)sampleHeight, (unsigned char)std::max(0, 255 - sampleHeight), 0};
            map_image_height.draw_circle(imageLon, imageLat, 2, colorHeight);

            // Write on the map
            unsigned char colorScatter[] = {(unsigned char)sampleScatter, (unsigned char)std::max(0, 255 - sampleScatter), 0};
            map_image_scatter.draw_circle(imageLon, imageLat, 2, colorScatter);
        }

        image::Image<uint8_t> PoseidonReader::getImageHeight()
        {
            return map_image_height;
        }

        image::Image<uint8_t> PoseidonReader::getImageScatter()
        {
            return map_image_scatter;
        }
    } // namespace modis
} // namespace eos