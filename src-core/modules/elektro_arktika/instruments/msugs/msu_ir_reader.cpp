#include "msu_ir_reader.h"

namespace elektro_arktika
{
    namespace msugs
    {
        MSUIRReader::MSUIRReader()
        {
            for (int i = 0; i < 7; i++)
                imageBuffer[i] = new unsigned short[3400 * (183 * (36 / 2))];
            frames = 0;
        }

        MSUIRReader::~MSUIRReader()
        {
            for (int i = 0; i < 7; i++)
                delete[] imageBuffer[i];
        }

        void MSUIRReader::pushFrame(uint8_t *data)
        {
            if (frames > 122400) // Only parse the first IR dump for now
                return;

            frames++;

            uint16_t counter = (data[8] & 0b00011111) << 8 | data[9];
            int segment = data[10] & 0b00111111;

            // Offset to start reading from
            int pos = 5 * 1;

            // Convert to 10 bits values
            for (int i = 0; i < 1546; i += 4)
            {
                msuLineBuffer[i] = (data[pos + 0] << 2) | (data[pos + 1] >> 6);
                msuLineBuffer[i + 1] = ((data[pos + 1] % 64) << 4) | (data[pos + 2] >> 4);
                msuLineBuffer[i + 2] = ((data[pos + 2] % 16) << 6) | (data[pos + 3] >> 2);
                msuLineBuffer[i + 3] = ((data[pos + 3] % 4) << 8) | data[pos + 4];
                pos += 5;
            }

            // Parse all channels
            if (segment % 2 == 0 && segment < 36 && counter < 3400)
            {
                // 1
                for (int i = 0; i < 183; i++)
                    imageBuffer[0][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[113 + i] * 60;

                // 2
                for (int i = 0; i < 183; i++)
                    imageBuffer[1][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[305 + i] * 60;

                // 3
                for (int i = 0; i < 183; i++)
                    imageBuffer[2][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[497 + i] * 60;

                // 4
                for (int i = 0; i < 183; i++)
                    imageBuffer[3][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[689 + i] * 60;

                // 5
                for (int i = 0; i < 183; i++)
                    imageBuffer[4][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[881 + i] * 60;

                // 6
                for (int i = 0; i < 183; i++)
                    imageBuffer[5][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[1073 + i] * 60;

                // 7
                for (int i = 0; i < 183; i++)
                    imageBuffer[6][counter * (183 * (36 / 2)) + ((segment / 2) * 183) + i] = msuLineBuffer[1265 + i] * 60;
            }
            else
            {
                // There is another, inversed segment here, maybe it should be parsed to then average or something?
            }
        }

        image::Image<uint16_t> MSUIRReader::getImage(int channel)
        {
            return image::Image<uint16_t>(&imageBuffer[channel][0], 183 * (36 / 2), 3400, 1);
        }
    } // namespace msugs
} // namespace elektro_arktika