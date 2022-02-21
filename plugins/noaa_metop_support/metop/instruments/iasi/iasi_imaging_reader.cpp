#include "iasi_imaging_reader.h"
#include "common/repack.h"
#include "common/ccsds/ccsds_time.h"

namespace metop
{
    namespace iasi
    {
        IASIIMGReader::IASIIMGReader()
        {
            ir_channel.create(10000 * 64 * 30);
            lines = 0;
            timestamps_ifov.push_back(std::vector<double>(30));
            std::fill(timestamps_ifov[lines].begin(), timestamps_ifov[lines].end(), -1);
        }

        IASIIMGReader::~IASIIMGReader()
        {
            ir_channel.destroy();
        }

        void IASIIMGReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 6196)
                return;

            int counter = packet.payload[16];

            if (counter <= 36)
            {
                // Repack to 12-bits
                repackBytesTo12bits(&packet.payload.data()[50], 6144, iasi_buffer);

                for (int i = 0; i < 64; i++)
                {
                    for (int y = 0; y < 64; y++)
                    {
                        ir_channel[(lines * 64 + i) * (36 * 64) + ((counter - 1) * 64) + (y)] = iasi_buffer[y * 64 + i] << 4;
                    }
                }

                if (counter <= 30)
                {
                    double timestamp = ccsds::parseCCSDSTimeFull(packet, 10957);
                    timestamps_ifov[lines][counter - 1] = timestamp;
                }
            }

            // Frame counter
            if (counter == 36)
            {
                lines++;
                timestamps_ifov.push_back(std::vector<double>(30));
                std::fill(timestamps_ifov[lines].begin(), timestamps_ifov[lines].end(), -1);
            }

            // Make sure we have enough room
            if (lines * 64 * 36 >= (int)ir_channel.size())
            {
                ir_channel.resize((lines + 1000) * 64 * 36);
            }
        }

        image::Image<uint16_t> IASIIMGReader::getIRChannel()
        {
            image::Image<uint16_t> img = image::Image<uint16_t>(ir_channel.buf, 36 * 64, lines * 64, 1);
            img.mirror(true, false);

            // Calibrate to remove the noise junk
            int mask[64 * 64];

            for (int y2 = 0; y2 < img.height() / 64; y2++)
            {
                // Read calibration
                for (int y = 0; y < 64; y++)
                {
                    for (int x = 0; x < 64; x++)
                    {
                        mask[y * 64 + x] = img[(y2 * 64 + y) * 36 * 64 + (4 * 64) + x];
                    }
                }

                // Apply
                for (int x2 = 0; x2 < 36; x2++)
                {
                    for (int y = 0; y < 64; y++)
                    {
                        for (int x = 0; x < 64; x++)
                        {
                            double initial = img[(y2 * 64 + y) * 36 * 64 + (x2 * 64) + x];
                            initial -= mask[y * 64 + x];
                            initial += 20000;
                            img[(y2 * 64 + y) * 36 * 64 + (x2 * 64) + x] = std::max<double>(initial, 0);
                        }
                    }
                }
            }

            // Crop calibration out
            img.crop(6 * 64, 0, 36 * 64, img.height());

            return img;
        }
    } // namespace iasi
} // namespace metop