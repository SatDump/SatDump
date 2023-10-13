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
            logger->trace("Tried to save empty PNG!");
            return;
        }

        std::ofstream fileo(file, std::ios::binary);

        int channels = 1;
        if (d_channels == 3)
            channels = 3;
        else if (d_channels == 4)
            channels = 3;

        if (channels == 1)
            fileo.write("P4  ", 4); // Magic Number + Whitespace
        else
            fileo.write("P6  ", 4);                          // Magic Number + Whitespace
        std::string swidth = std::to_string(d_width) + "  "; // Width + Whitespace
        fileo.write(swidth.c_str(), swidth.size());
        std::string sheight = std::to_string(d_height) + "  "; // Height + Whitespace
        fileo.write(sheight.c_str(), sheight.size());

        if (d_depth == 8)
        {
            std::string smaxval = "255\n"; // MaxVal + Whitespace
            fileo.write(smaxval.c_str(), smaxval.size());

            for (int y = 0; y < d_height; y++)
                for (int x = 0; x < d_width; x++)
                    for (int c = 0; c < channels; c++)
                        fileo.write((char *)&channel(c)[y * d_width + x], sizeof(uint8_t));
        }
        else if (d_depth == 16)
        {
            std::string smaxval = "65535\n"; // MaxVal + Whitespace
            fileo.write(smaxval.c_str(), smaxval.size());

            for (int y = 0; y < d_height; y++)
            {
                for (int x = 0; x < d_width; x++)
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
            signature.resize(2);
            filei.read((char *)signature.c_str(), 2);

            logger->critical(signature);

            int channels = 1;
            if (signature == "P4")
                channels = 1;
            else if (signature == "P6")
                channels = 3;

            uint8_t dump; // Extra char
            filei.read((char *)&dump, 1);

            std::string widthstr;
            while (!filei.eof())
            {
                char c;
                filei.read(&c, 1);
                if (c != ' ' && c != '\r' && c != '\n')
                    widthstr.push_back(c);
                else
                    break;
            }
            int width = std::stoi(widthstr);

            logger->critical(widthstr);

            std::string heightstr;
            while (!filei.eof())
            {
                char c;
                filei.read(&c, 1);
                if (c != ' ' && c != '\r' && c != '\n')
                    heightstr.push_back(c);
                else
                    break;
            }
            int height = std::stoi(heightstr);

            logger->critical(heightstr);

            init(width, height, channels);

            std::string maxvalstr;
            while (!filei.eof())
            {
                char c;
                filei.read(&c, 1);
                if (c != ' ' && c != '\r' && c != '\n')
                    maxvalstr.push_back(c);
                else
                    break;
            }
            int maxval = std::stoi(maxvalstr);

            if (d_depth == 8)
            {
                if (maxval > 255)
                {
                    for (int y = 0; y < d_height; y++)
                    {
                        for (int x = 0; x < d_width; x++)
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
                    for (int y = 0; y < d_height; y++)
                    {
                        for (int x = 0; x < d_width; x++)
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
                    for (int y = 0; y < d_height; y++)
                    {
                        for (int x = 0; x < d_width; x++)
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
                    for (int y = 0; y < d_height; y++)
                    {
                        for (int x = 0; x < d_width; x++)
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