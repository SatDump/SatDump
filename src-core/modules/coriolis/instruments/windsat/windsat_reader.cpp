#include "windsat_reader.h"
#include "common/repack.h"

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define __bswap_16 OSSwapInt16
#elif _WIN32
#define __bswap_16 _byteswap_ushort
#elif __ANDROID__
#include <byteswap.h>
#define __bswap_16 __swap16
#endif

namespace coriolis
{
    namespace windsat
    {
        WindSatReader::WindSatReader(int chid, int width)
        {
            this->width = width;
            channel_id = chid;
            image1 = new unsigned short[100000 * width];
            image2 = new unsigned short[100000 * width];
            lines = 0;
            scanline_id = 0;
        }

        WindSatReader::~WindSatReader()
        {
            delete[] image1;
            delete[] image2;
        }

        void WindSatReader::work(uint8_t *frame)
        {
            if ((frame[0] >> 4) == 0b0011) // Only let radiometer frames scan frames through
            {
                if ((frame[0] & 0b1111) == channel_id) // Select a channel
                {
                    uint32_t scan_id = /*frame[4] << 24 |*/ frame[5] << 16 | frame[6] << 8 | frame[7]; // Scanline ID
                    int pixel_offset = (frame[8] & 0b1111) << 8 | frame[9];                        // Pixel offset

                    if (pixel_offset + 12 > width)
                        return;

                    // Parse into image
                    int16_t *data16 = (int16_t *)&frame[12];
                    for (int i = 0; i < 13; i++)
                    {
                        uint16_t dd1 = __bswap_16(data16[i * 2 + 0]);
                        uint16_t dd2 = __bswap_16(data16[i * 2 + 1]);
                        image1[lines * width + pixel_offset + (12 - i)] = *((int16_t *)&dd1) + 32768;
                        image2[lines * width + pixel_offset + (12 - i)] = *((int16_t *)&dd2) + 32768;
                    }

                    // Detect new scanline
                    if (scan_id != scanline_id)
                    {
                        lines++;
                        scanline_id = scan_id;
                    }
                }
            }
        }

        image::Image<uint16_t> WindSatReader::getImage1()
        {
            return image::Image<uint16_t>(image1, width, lines, 1);
        }

        image::Image<uint16_t> WindSatReader::getImage2()
        {
            return image::Image<uint16_t>(image2, width, lines, 1);
        }
    } // namespace avhrr
} // namespace noaa
