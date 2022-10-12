#include "sem_reader.h"

#include "logger.h"

namespace noaa
{
    namespace sem
    {
        SEMReader::SEMReader()
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

            uint8_t mf20 = mf % 20;

            uint8_t word0 = buffer[20];
            uint8_t word1 = buffer[21];

            // MEPED start
            if (mf20 == 10)
            {
                channels[19]->push_back(word0);
                if ((mf + 10) % 40 == 0)
                    channels[20]->push_back(word1);
                else
                    channels[21]->push_back(word1);
            }
            else if (mf20 == 0)
                channels[0]->push_back(word1);
            else if (mf20 > 0 && mf20 < 10)
            {
                uint8_t n = 2 * mf20;
                channels[n - 1]->push_back(word0);
                channels[n]->push_back(word1);
            }
            // MEPED end
            // TED 4-PES start
            else if ((mf20 == 11 || mf20 == 12) && mf / 20 < 14)
            {
                uint8_t n = (((mf20 - 11) * 2 + 4 * mf / 20) % 16) + 22;
                channels[n]->push_back(word0);
                channels[n + 1]->push_back(word1);
            }
            // TED 4-PES end
            // TED Flux start
            else if (mf20 > 12 && mf < 17)
            {
                uint8_t n = 2 * (mf20 - 13) + 38;
                channels[n]->push_back(word0);
                channels[n + 1]->push_back(word1);
            }
            else
                switch (mf20)
                {
                case 17:
                    channels[46]->push_back(word0 >> 4);
                    channels[48]->push_back(word0 & 0b00001111);
                    channels[50]->push_back(word1);
                    break;
                case 18:
                    channels[52]->push_back(word0);
                    channels[47]->push_back(word1 >> 4);
                    channels[49]->push_back(word1 & 0b00001111);
                    break;
                case 19:
                    channels[51]->push_back(word0);
                    channels[53]->push_back(word1);
                    break;

                default:
                    break;
                }
            // TED Flux end
            // TED Background start
            switch (mf)
            {
            case 292:
                channels[54]->push_back(word0);
                channels[55]->push_back(word1);
                break;
            case 311:
            case 312:
                channels[mf-255]->push_back(word1);
                break;
            case 291:
                channels[58]->push_back(word0);
                channels[60]->push_back(word1);
                break;
            case 280:
                channels[59]->push_back(word0);
                break;
            case 300:
                channels[61]->push_back(word0);
                break;

            default:
                break;
            }
            // TED Background end
        }
        std::vector<int> SEMReader::getChannel(int channel){
            return *channels[channel];
        }
    }
}