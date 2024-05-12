#include "../io.h"
#include <cstring>
#define QOI_IMPLEMENTATION 1
#include "libs/qoi.h"
#include "logger.h"
#include <filesystem>
#include <fstream>
#include "core/exception.h"

namespace image
{
    void save_qoi(Image &img, std::string file)
    {
        auto d_depth = img.depth();
        auto d_channels = img.channels();
        auto d_height = img.height();
        auto d_width = img.width();

        if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
        {
            logger->trace("Tried to save empty QOI!");
            return;
        }

        int final_channels = d_channels == 4 ? 4 : 3;
        int d_shift = d_depth - 8;

        uint8_t *qoi_buffer = new uint8_t[d_width * d_height * final_channels];

        if (d_channels == 1)
        {
            for (size_t i = 0; i < d_width * d_height; i++)
                for (int c = 0; c < final_channels; c++)
                    qoi_buffer[i * final_channels + c] = img.get(i) >> d_shift;
        }
        else if (d_channels == 3 || d_channels == 4)
        {
            for (size_t i = 0; i < d_width * d_height; i++)
                for (int c = 0; c < final_channels; c++)
                    qoi_buffer[i * final_channels + c] = img.get(c, i) >> d_shift;
        }

        qoi_desc desc = {.width = d_width, .height = d_height, .channels = final_channels, .colorspace = QOI_SRGB};
        qoi_write(file.c_str(), qoi_buffer, &desc);

        // Custom marker for single-channel files
        if (d_channels == 1)
        {
            uint8_t v = 0xAA;
            std::ofstream(file, std::ios::app | std::ios::binary).write((char *)&v, 1);
        }

        delete[] qoi_buffer;
    }

    void load_qoi(Image &img, std::string file)
    {
        if (!std::filesystem::exists(file))
            return;

        // Custom marker for single-channel files
        bool is_single_channel = false;
        {
            FILE *fin = fopen(file.c_str(), "r");
            if (fin != nullptr)
            {
                fseek(fin, -1, SEEK_END);
                uint8_t lastByte;
                if (fread(&lastByte, 1, 1, fin) != 0)
                {
                    fclose(fin);
                    is_single_channel = lastByte == 0xAA;
                    logger->info("TRAILER");
                }
            }
        }

        logger->critical("isSingle %d", (int)is_single_channel);

        qoi_desc desc;
        uint8_t *qoi_buffer = (uint8_t *)qoi_read(file.c_str(), &desc, 4);

        int d_channels = is_single_channel ? 1 : (desc.channels == 3 ? 3 : 4);
        size_t d_width = desc.width;
        size_t d_height = desc.height;
        int d_depth = 8;

        img.init(d_depth, d_width, d_height, d_channels);

        if (d_channels == 1)
        {
            for (size_t i = 0; i < d_width * d_height; i++)
                img.set(i, qoi_buffer[i * 4 + 0]);
        }
        else if (d_channels == 3)
        {
            for (size_t i = 0; i < d_width * d_height; i++)
                for (int c = 0; c < d_channels; c++)
                    img.set(c, i, qoi_buffer[i * 4 + c]);
        }

        if (qoi_buffer != nullptr)
            QOI_FREE(qoi_buffer);
    }
}
