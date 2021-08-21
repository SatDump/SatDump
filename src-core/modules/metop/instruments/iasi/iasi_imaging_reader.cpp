#include "iasi_imaging_reader.h"
#include "utils.h"

namespace metop
{
    namespace iasi
    {
        IASIIMGReader::IASIIMGReader()
        {
            ir_channel = new unsigned short[10000 * 64 * 30];
            lines = 0;
        }

        IASIIMGReader::~IASIIMGReader()
        {
            delete[] ir_channel;
        }

        void IASIIMGReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 6196)
                return;

            int counter = packet.payload[16];

            if (counter <= 30)
            {
                std::vector<uint16_t> values = repackBits(&packet.payload.data()[50], 12, 0, 6196 - 50);

                for (int i = 0; i < 64; i++)
                {
                    for (int y = 0; y < 64; y++)
                    {
                        if (y >= 48)
                            ir_channel[(lines * 64 + i) * (30 * 64) + ((counter - 1) * 64) + (y)] = (values[y * 64 + i] + 10) << 3;
                        else
                            ir_channel[(lines * 64 + i) * (30 * 64) + ((counter - 1) * 64) + (y)] = values[y * 64 + i] << 3;
                    }
                }
            }

            // Frame counter
            if (counter == 36)
                lines++;
        }

        int percentile(unsigned short *array, int size, float percentile)
        {
            float number_percent = (size + 1) * percentile / 100.0f;
            if (number_percent == 1)
                return array[0];
            else if (number_percent == size)
                return array[size - 1];
            else
                return array[(int)number_percent - 1] + (number_percent - (int)number_percent) * (array[(int)number_percent] - array[(int)number_percent - 1]);
        }

        cimg_library::CImg<unsigned short> IASIIMGReader::getIRChannel()
        {
            cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(ir_channel, 30 * 64, lines * 64);
            img.mirror('x');
            return img;
        }
    } // namespace iasi
} // namespace metop