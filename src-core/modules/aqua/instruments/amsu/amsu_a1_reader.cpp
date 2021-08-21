#include "amsu_a1_reader.h"

namespace aqua
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
            // First part of the scan
            if (packet.header.apid == 261)
            {
                if (packet.payload.size() < 704)
                    return;

                // This is is very messy, but the spacing between samples wasn't constant so...
                // We have to put it back together the hard way...
                int pos = 22;

                for (int i = 0; i < 20 * 17; i++)
                {
                    lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                    pos += 2;
                }

                // Plot into an images
                for (int channel = 0; channel < 13; channel++)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        channels[channel][lines * 30 + 30 - i] = lineBuffer[i * 17 + channel];
                    }
                }

                // Frame counter
                lines++;
            }
            // Second part of the scan
            else if (packet.header.apid == 262)
            {
                if (packet.payload.size() < 612)
                    return;

                // This is is very messy, but the spacing between samples wasn't constant so...
                // We have to put it back together the hard way...
                int pos = 16;

                for (int i = 20 * 17; i < 31 * 17; i++)
                {
                    lineBuffer[i] = packet.payload[pos + 0] << 8 | packet.payload[pos + 1];
                    pos += 2;
                }

                // Plot into an images
                for (int channel = 0; channel < 13; channel++)
                {
                    for (int i = 20; i < 30; i++)
                    {
                        channels[channel][lines * 30 + 30 - i] = lineBuffer[i * 17 + channel];
                    }
                }
            }
        }

        cimg_library::CImg<unsigned short> AMSUA1Reader::getChannel(int channel)
        {
            cimg_library::CImg<unsigned short> img = cimg_library::CImg<unsigned short>(channels[channel], 30, lines);
            img.normalize(0, 65535);
            img.equalize(1000);
            return img;
        }
    } // namespace amsu
} // namespace aqua