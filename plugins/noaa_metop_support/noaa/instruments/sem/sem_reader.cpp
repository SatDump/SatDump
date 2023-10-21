#include "sem_reader.h"

#include "logger.h"

namespace noaa
{
    namespace sem
    {
        SEMReader::SEMReader(int year) : ttp(year)
        {
            for (int i = 0; i < 62; i++)
            {
                channels[i] = new std::vector<int>;
                timestamps[i] = new std::vector<double>;
            }
        }

        SEMReader::~SEMReader()
        {
            for (int i = 0; i < 62; i++)
            {
                delete channels[i];
                delete timestamps[i];
            }
        }

        void SEMReader::work(uint8_t *buffer)
        {
            uint16_t mf = ((buffer[4] & 1) << 8) | (buffer[5]);

            if (mf > 319)
                return;

            if (mf == 0){
                int days = (buffer[8] << 1) | (buffer[9] >> 7);
                uint64_t millisec = ((buffer[9] & 0b00000111) << 24) | (buffer[10] << 16) | (buffer[11] << 8) | buffer[12];
                lastTS = ttp.getTimestamp(days, millisec);
            }
            
            uint8_t mf20 = mf % 20;
            uint8_t word0 = buffer[20];
            uint8_t word1 = buffer[21];

            // MEPED start
            if (mf20 == 10)
            {
                pushChannelDataAndTS(19, word0, mf);
                if ((mf + 10) % 40 == 0)
                    pushChannelDataAndTS(20, word1, mf);
                else
                    pushChannelDataAndTS(21, word1, mf);
            }
            else if (mf20 == 0)
                pushChannelDataAndTS(0, word1, mf);
            else if (mf20 > 0 && mf20 < 10)
            {
                uint8_t n = 2 * mf20;
                pushChannelDataAndTS(n - 1, word0, mf);
                pushChannelDataAndTS(n, word1, mf);
            }
            // MEPED end
            // TED 4-PES start
            else if ((mf20 == 11 || mf20 == 12) && mf / 20 < 14)
            {
                uint8_t n = (((mf20 - 11) * 2 + 4 * mf / 20) % 16) + 22;
                pushChannelDataAndTS(n, word0, mf);
                pushChannelDataAndTS(n + 1, word1, mf);
            }
            // TED 4-PES end
            // TED Flux start
            else if (mf20 > 12 && mf < 17)
            {
                uint8_t n = 2 * (mf20 - 13) + 38;
                pushChannelDataAndTS(n, word0, mf);
                pushChannelDataAndTS(n + 1, word1, mf);
            }
            else
                switch (mf20)
                {
                case 17:
                    pushChannelDataAndTS(46, word0 >> 4, mf);
                    pushChannelDataAndTS(48, word0 & 0b00001111, mf);
                    pushChannelDataAndTS(50, word1, mf);
                    break;
                case 18:
                    pushChannelDataAndTS(52, word0, mf);
                    pushChannelDataAndTS(47, word1 >> 4, mf);
                    pushChannelDataAndTS(49, word1 & 0b00001111, mf);
                    break;
                case 19:
                    pushChannelDataAndTS(51, word0, mf);
                    pushChannelDataAndTS(53, word1, mf);
                    break;

                default:
                    break;
                }
            // TED Flux end
            // TED Background start
            switch (mf)
            {
            case 292:
                pushChannelDataAndTS(54, word0, mf);
                pushChannelDataAndTS(55, word1, mf);
                break;
            case 311:
            case 312:
                pushChannelDataAndTS(mf-255, word1, mf);
                break;
            case 291:
                pushChannelDataAndTS(58, word0, mf);
                pushChannelDataAndTS(60, word1, mf);
                break;
            case 280:
                pushChannelDataAndTS(59, word0, mf);
                break;
            case 300:
                pushChannelDataAndTS(61, word0, mf);
                break;

            default:
                break;
            }
            // TED Background end
        }
        std::vector<int> SEMReader::getChannel(int channel){
            return *channels[channel];
        }
        std::vector<double> SEMReader::getTimestamps(int channel){
            return *timestamps[channel];
        }
    }
}