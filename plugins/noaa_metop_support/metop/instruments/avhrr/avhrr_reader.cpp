#include "avhrr_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace metop
{
    namespace avhrr
    {
        AVHRRReader::AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i].resize(2048);
            lines = 0;
        }

        AVHRRReader::~AVHRRReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i].clear();
        }

        void AVHRRReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 12960)
                return;

            // Convert AVHRR payload into 10-bits values
            repackBytesTo10bits(&packet.payload[14], 12944, avhrr_buffer);

            for (int channel = 0; channel < 5; channel++)
                for (int i = 0; i < 2048; i++)
                    channels[channel][lines * 2048 + i] = avhrr_buffer[55 + channel + i * 5] << 6;

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;

            for (int channel = 0; channel < 5; channel++)
                channels[channel].resize((lines + 1) * 2048);
        }

        image::Image<uint16_t> AVHRRReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel].data(), 2048, lines, 1);
        }
    } // namespace avhrr
} // namespace metop