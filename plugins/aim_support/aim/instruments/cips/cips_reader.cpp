#include "cips_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include <cmath>

#include "logger.h"

namespace aim
{
    namespace cips
    {
        CIPSReader::CIPSReader()
        {
        }

        CIPSReader::~CIPSReader()
        {
            //  delete[] tmp_words;
        }

        float nominal_root_2n(uint16_t value)
        {
            // const float alpha = 512;
            const float delta = 510;

            float decompressed_value = 0;
            if (value > delta)
                decompressed_value = powf(value - delta - 1.0, 2) / 2.0;
            else
                decompressed_value = value;
            return decompressed_value;
        }

        float optimized_root_2n(uint16_t value)
        {
            // const float alpha = 512;
            const float delta = 510;
            const float gamma = 543;

            float decompressed_value = 0;
            if (value > gamma)
                decompressed_value = powf(value - delta - 1.0, 2) / 2.0;
            else
                decompressed_value = value;
            return decompressed_value;
        }

        void CIPSReader::work(ccsds::CCSDSPacket &packet)
        {
            packet.payload.resize(1018);

            CIPSScienceHeader science_header(&packet.payload[13]);

            if (science_header.start_pixel == 0)
                add_image();

            auto &img = images[images.size() - 1];

            if (science_header.bits_pixel == 10)
            {
                uint16_t pixels[774];
                repackBytesTo10bits(&packet.payload[54 - 6], 968, pixels);

                for (int i = science_header.start_pixel, i2 = 0; i < (int)img.size() && i2 < 774; i++, i2++)
                {
                    float value = 0;

                    if (science_header.compression_scheme == 0)
                        value = nominal_root_2n(pixels[i2]);
                    else if (science_header.compression_scheme == 1)
                        value = optimized_root_2n(pixels[i2]);
                    else
                        value = nominal_root_2n(pixels[i2]);

                    value /= 2.0;

                    if (value > 65536)
                        value = 65535;
                    if (value < 0)
                        value = 0;

                    img.set(i, value);
                }
            }
            else if (science_header.bits_pixel == 17)
            {
                uint32_t pixels[455];
                repackBytesTo17bits(&packet.payload[54 - 6], 968, pixels);

                for (int i = science_header.start_pixel, i2 = 0; i < (int)img.size() && i2 < 455; i++, i2++)
                {
                    float value = pixels[i2];

                    value /= 2.0;

                    if (value > 65536)
                        value = 65535;
                    if (value < 0)
                        value = 0;

                    img.set(i, value);
                }
            }
        }
    } // namespace swap
} // namespace proba