#include "image.h"
#include <cstring>
#include <fstream>
#include "logger.h"
#include <filesystem>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace image
{
    template <typename T>
    void Image<T>::save_pbm(std::string file)
    {
        if (data_size == 0 || d_height == 0) // Make sure we aren't just gonna crash
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

            for (size_t y = 0; y < d_height; y++)
                for (size_t x = 0; x < d_width; x++)
                    for (int c = 0; c < channels; c++)
                        fileo.write((char *)&channel(c)[y * d_width + x], sizeof(uint8_t));
        }
        else if (d_depth == 16)
        {
            std::string smaxval = "65535\n"; // MaxVal + Whitespace
            fileo.write(smaxval.c_str(), smaxval.size());

            for (size_t y = 0; y < d_height; y++)
            {
                for (size_t x = 0; x < d_width; x++)
                {
                    for (int c = 0; c < channels; c++)
                    {
                        uint16_t v = INVERT_ENDIAN_16(channel(c)[y * d_width + x]);
                        fileo.write((char *)&v, sizeof(uint16_t));
                    }
                }
            }
        }

        fileo.close();
    }

    template <typename T>
    void Image<T>::load_pbm(std::string file)
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
                throw std::runtime_error("Invalid Magic Number " + signature);

            init(width, height, channels);
            filei.seekg(1, std::ios_base::cur);

            if (d_depth == 8)
            {
                if (maxval > 255)
                {
                    for (size_t y = 0; y < d_height; y++)
                    {
                        for (size_t x = 0; x < d_width; x++)
                        {
                            for (int c = 0; c < d_channels; c++)
                            {
                                uint16_t v;
                                filei.read((char *)&v, sizeof(uint16_t));
                                channel(c)[y * d_width + x] = INVERT_ENDIAN_16(v) >> 8;
                            }
                        }
                    }
                }
                else
                {
                    for (size_t y = 0; y < d_height; y++)
                    {
                        for (size_t x = 0; x < d_width; x++)
                        {
                            for (int c = 0; c < d_channels; c++)
                            {
                                uint8_t v;
                                filei.read((char *)&v, sizeof(uint8_t));
                                channel(c)[y * d_width + x] = v;
                            }
                        }
                    }
                }
            }
            else if (d_depth == 16)
            {
                if (maxval > 255)
                {
                    for (size_t y = 0; y < d_height; y++)
                    {
                        for (size_t x = 0; x < d_width; x++)
                        {
                            for (int c = 0; c < d_channels; c++)
                            {
                                uint16_t v;
                                filei.read((char *)&v, sizeof(uint16_t));
                                channel(c)[y * d_width + x] = INVERT_ENDIAN_16(v);
                            }
                        }
                    }
                }
                else
                {
                    for (size_t y = 0; y < d_height; y++)
                    {
                        for (size_t x = 0; x < d_width; x++)
                        {
                            for (int c = 0; c < d_channels; c++)
                            {
                                uint8_t v;
                                filei.read((char *)&v, sizeof(uint8_t));
                                channel(c)[y * d_width + x] = v << 8;
                            }
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

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}
