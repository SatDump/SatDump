#include "../io.h"
#include <cstring>
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "core/exception.h"

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace image
{
    void save_pbm(Image &img, std::string file)
    {
        auto d_depth = img.depth();
        auto d_channels = img.channels();
        auto d_height = img.height();
        auto d_width = img.width();

        if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
        {
            logger->trace("Tried to save empty PBM!");
            return;
        }

        std::ofstream fileo(file, std::ios::binary);

        int channels = 1;
        if (d_channels == 3)
            channels = 3;
        else if (d_channels == 4)
            channels = 3;

        if (channels == 1)
            fileo.write("P5 ", 3); // Magic Number + Whitespace
        else
            fileo.write("P6 ", 3);                          // Magic Number + Whitespace
        std::string swidth = std::to_string(d_width) + " "; // Width + Whitespace
        fileo.write(swidth.c_str(), swidth.size());
        std::string sheight = std::to_string(d_height) + " "; // Height + Whitespace
        fileo.write(sheight.c_str(), sheight.size());

        if (d_depth == 8)
        {
            std::string smaxval = "255\n"; // MaxVal + Whitespace
            fileo.write(smaxval.c_str(), smaxval.size());

            for (size_t i = 0; i < d_width * d_height; i++)
            {
                for (int c = 0; c < channels; c++)
                {
                    if (d_channels == 4)
                    {
                        uint8_t write_val = (float)img.get(c, i) * ((float)img.get(3, i) / 255.0f);
                        fileo.write((char *)&write_val, sizeof(uint8_t));
                    }
                    else
                    {
                        uint8_t write_val = img.get(c, i);
                        fileo.write((char *)&write_val, sizeof(uint8_t));
                    }
                }
            }
        }
        else if (d_depth == 16)
        {
            std::string smaxval = "65535\n"; // MaxVal + Whitespace
            fileo.write(smaxval.c_str(), smaxval.size());

            for (size_t i = 0; i < d_width * d_height; i++)
            {
                for (int c = 0; c < channels; c++)
                {
                    uint16_t v = img.get(c, i);
                    if (d_channels == 4)
                        v = (float)v * (float)img.get(3, i) / 65535.0f;
                    v = INVERT_ENDIAN_16(v);
                    fileo.write((char *)&v, sizeof(uint16_t));
                }
            }
        }

        fileo.close();
    }

    void load_pbm(Image &img, std::string file)
    {
        if (!std::filesystem::exists(file))
            return;

        try
        {
            std::ifstream filei(file, std::ios::binary);
            std::string signature;
            size_t width, height, maxval;
            int channels;

            filei >> signature >> width >> height >> maxval;

            if (signature == "P5")
                channels = 1;
            else if (signature == "P6")
                channels = 3;
            else
                throw satdump_exception("Invalid Magic Number " + signature);

            int d_channels = channels;
            size_t d_width = width;
            size_t d_height = height;
            int d_depth = maxval == 255 ? 8 : 16;

            img.init(d_depth, width, height, channels);
            filei.seekg(1, std::ios_base::cur);

            if (d_depth == 8)
            {
                for (size_t y = 0; y < d_height; y++)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        for (int c = 0; c < d_channels; c++)
                        {
                            uint8_t v;
                            filei.read((char *)&v, sizeof(uint8_t));
                            img.set(c, y * d_width + x, v);
                        }
                    }
                }
            }
            else if (d_depth == 16)
            {
                for (size_t y = 0; y < d_height; y++)
                {
                    for (size_t x = 0; x < d_width; x++)
                    {
                        for (int c = 0; c < d_channels; c++)
                        {
                            uint16_t v;
                            filei.read((char *)&v, sizeof(uint16_t));
                            img.set(c, y * d_width + x, INVERT_ENDIAN_16(v));
                        }
                    }
                }
            }

            filei.close();
        }
        catch (std::exception &e)
        {
            logger->error("Could not open PBM file : %s", e.what());
        }
    }
}
