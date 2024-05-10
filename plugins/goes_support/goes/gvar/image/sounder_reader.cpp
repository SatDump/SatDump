#include "sounder_reader.h"
#include <cstring>

#define WIDTH 1758
#define HEIGHT 1577

#include "logger.h"

namespace goes
{
    namespace gvar
    {
        SounderReader::SounderReader()
        {
            for (int i = 0; i < 19; i++)
                channels[i] = new uint16_t[HEIGHT * WIDTH];
        }

        SounderReader::~SounderReader()
        {
            for (int i = 0; i < 19; i++)
                delete[] channels[i];
        }

        void SounderReader::clear()
        {
            for (int i = 0; i < 19; i++)
                memset(channels[i], 0, HEIGHT * WIDTH * sizeof(uint16_t));
        }

        void SounderReader::pushFrame(uint8_t *data, int /*counter*/)
        {
            int y_pos[4];
            for (int i = 0; i < 4; i++)
                y_pos[i] = data[6112 + i * 2] << 8 | data[6113 + i * 2];

            int x_pos[11];
            for (int i = 0; i < 11; i++)
                x_pos[i] = data[6120 + i * 2] << 8 | data[6121 + i * 2];

            for (int ch = 0; ch < 19; ch++)
            {
                uint8_t *ch_ptr = &data[6466 + ch * 11 * 4 * 2];

                for (int i = 0; i < 11 * 4; i++)
                    temp_detect[i] = ch_ptr[i * 2 + 0] << 8 | ch_ptr[i * 2 + 1];

                for (int y = 0; y < 4; y++)
                {
                    for (int x = 0; x < 11; x++)
                    {
                        if (y_pos[y] == 0 || y_pos[y] >= 1577)
                            continue;
                        if (x_pos[x] == 0 || x_pos[x] >= 1758)
                            continue;
                        channels[ch][(y_pos[y] - 1) * WIDTH + (x_pos[x] - 1)] = temp_detect[y * 11 + x];
                    }
                }
            }
        }

        image::Image SounderReader::getImage(int channel)
        {
            return image::Image(channels[channel], 16, WIDTH, HEIGHT, 1);
        }
    }
}