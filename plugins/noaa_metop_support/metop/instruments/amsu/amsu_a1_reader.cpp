#include "amsu_a1_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace metop
{
    namespace amsu
    {
        AMSUA1Reader::AMSUA1Reader()
        {
            for (int i = 0; i < 13; i++)
                channels[i] = new unsigned short[10000 * 30];
            lines = 0;
        }

        AMSUA1Reader::~AMSUA1Reader()
        {
            for (int i = 0; i < 13; i++)
                delete[] channels[i];
        }

        void AMSUA1Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 2096)
                return;

            // This is is very messy, but the spacing between samples wasn't constant so...
            // We have to put it back together the hard way...
            // There also are a few weird shifts here and there, which are fixed by checked at the filler AMSU word.
            int pos = 72 - 6;
            for (int i = 0; i < 12 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 700 - 6;
            for (int i = 12 * 26; i < 14 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            bool shifted = uint16_t(packet.payload[804 - 6 + 0] << 8 | packet.payload[804 - 6 + 1]) != 0b0000000000000001;
            pos = (shifted ? 804 : 806) - 6;
            for (int i = 14 * 26; i < 15 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 862 - 6;
            for (int i = 15 * 26; i < 16 * 30; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 920 - 6;
            for (int i = 16 * 26; i < 17 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 1034 - 6;
            for (int i = 17 * 26; i < 18 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            shifted = uint16_t(packet.payload[976 - 6 + 0] << 8 | packet.payload[976 - 6 + 1]) == 0b0000000000000001;
            pos = (shifted ? 978 : 976) - 6;
            for (int i = 18 * 26; i < 19 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            pos = 1086 - 6;
            for (int i = 19 * 26; i < 30 * 26; i++)
            {
                lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                pos += 2;
            }

            // Plot into an images
            for (int channel = 0; channel < 13; channel++)
                for (int i = 0; i < 30; i++)
                    channels[channel][lines * 30 + 29 - i] = lineBuffer[i * 26 + channel];

            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, 10957));

            // Frame counter
            lines++;
        }

        image::Image<uint16_t> AMSUA1Reader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 30, lines, 1);
        }
    } // namespace amsu
} // namespace metop