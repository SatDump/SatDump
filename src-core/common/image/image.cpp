#include "image.h"
#include "logger.h"
#include "core/config.h"
#include <cstring>
#include <limits>
#include <fstream>
#include "image_meta.h"

namespace image
{
    template <typename T>
    void Image<T>::init(size_t width, size_t height, int channels)
    {
        // Reset image if we already had one
        if (has_data)
            delete[] d_data;

        // Init buffer
        data_size = width * height * channels;
        d_data = new T[data_size];

        // Set to 0
        memset(d_data, 0, sizeof(T) * data_size);

        // Init local variables
        d_depth = sizeof(T) * 8;
        d_width = width;
        d_height = height;
        d_channels = channels;

        // We have data now if we didn't already
        has_data = true;
    }

    template <typename T>
    void Image<T>::clear()
    {
        // Reset image
        if (has_data)
            delete[] d_data;
        has_data = false;
    }

    template <typename T>
    Image<T>::Image()
    {
        // Do nothing
    }

    template <typename T>
    Image<T>::Image(size_t width, size_t height, int channels)
    {
        init(width, height, channels);
    }

    template <typename T>
    Image<T>::Image(const Image &img)
    {
        // Copy contents of the image over
        init(img.d_width, img.d_height, img.d_channels);
        memcpy(d_data, img.d_data, img.data_size * sizeof(T));
        copy_meta(img);
    }

    template <typename T>
    Image<T>::Image(T *buffer, size_t width, size_t height, int channels)
    {
        // Copy contents of the image over
        init(width, height, channels);
        memcpy(d_data, buffer, data_size * sizeof(T));
    }

    template <typename T>
    Image<T> &Image<T>::operator=(const Image<T> &img)
    {
        // Copy contents of the image over
        init(img.d_width, img.d_height, img.d_channels);
        memcpy(d_data, img.d_data, img.data_size * sizeof(T));
        copy_meta(img);
        return *this;
    }

    template <typename T>
    Image<T> &Image<T>::operator<<=(const int &shift)
    {
        for (size_t i = 0; i < data_size; i++)
            d_data[i] <<= shift;
        return *this;
    }

    template <typename T>
    Image<T>::~Image()
    {
        if (has_data)
            delete[] d_data;

        if (has_font)
        {
            for (auto &v : font.chars)
                free(v.bitmap);
            font.chars.clear();
            delete[] ttf_buffer;
        }

        free_metadata_proj_cfg(*this);
    }

    template <typename T>
    void Image<T>::init_font(std::string font_path)
    {
        std::ifstream infile(font_path, std::ios::binary);

        if (!infile.good())
            return;
        // get length of file
        infile.seekg(0, std::ios::end);
        size_t length = infile.tellg();
        infile.seekg(0, std::ios::beg);
        ttf_buffer = new uint8_t[length];
        // read file
        infile.read((char *)ttf_buffer, length);

        stbtt_fontinfo fontp;
        stbtt_InitFont(&fontp, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));

        stbtt_GetFontBoundingBox(&fontp, &font.x0, &font.y0, &font.x1, &font.y1);
        stbtt_GetFontVMetrics(&fontp, &font.asc, &font.dsc, &font.lg);

        font.fontp = fontp;
        infile.close();
        has_font = true;
    }

    template <typename T>
    T Image<T>::clamp(int input)
    {
        if (input > std::numeric_limits<T>::max())
            return std::numeric_limits<T>::max();
        else if (input < 0)
            return 0;
        else
            return input;
    }

    template <typename T>
    void Image<T>::to_rgb()
    {
        if (d_channels == 1)
        {
            Image<T> tmp = *this;       // Backup image
            init(d_width, d_height, 3); // Init new image as RGB

            // Fill in all 3 channels
            draw_image(0, tmp);
            draw_image(1, tmp);
            draw_image(2, tmp);
        }
        else if (d_channels == 4)
        {
            Image<T> tmp = *this;       // Backup image
            init(d_width, d_height, 3); // Init new image as RGB

            // Copy over all 3 channels
            memcpy(d_data, tmp.data(), d_width * d_height * 3 * sizeof(T));
        }
    }

    template <typename T>
    void Image<T>::to_rgba()
    {
        if (d_channels == 1)
        {
            Image<T> tmp = *this;       // Backup image
            init(d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy(&d_data[d_width * d_height * 0], tmp.data(), d_width * d_height * sizeof(T));
            memcpy(&d_data[d_width * d_height * 1], tmp.data(), d_width * d_height * sizeof(T));
            memcpy(&d_data[d_width * d_height * 2], tmp.data(), d_width * d_height * sizeof(T));
            for (size_t i = 0; i < d_width * d_height; i++)
                channel(3)[i] = std::numeric_limits<T>::max();
        }
        else if (d_channels == 22)
        {
            Image<T> tmp = *this;       // Backup image
            init(d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy(&d_data[d_width * d_height * 0], tmp.data(), d_width * d_height * sizeof(T));
            memcpy(&d_data[d_width * d_height * 1], tmp.data(), d_width * d_height * sizeof(T));
            memcpy(&d_data[d_width * d_height * 2], tmp.data(), d_width * d_height * sizeof(T));

            // Copy over RGBA
            memcpy(&d_data[d_width * d_height * 3], tmp.data() + d_width * d_height, d_width * d_height * sizeof(T));
        }
        else if (d_channels == 3)
        {
            Image<T> tmp = *this;       // Backup image
            init(d_width, d_height, 4); // Init new image as RGBA

            // Copy over all 3 channels
            memcpy(d_data, tmp.data(), d_width * d_height * 3 * sizeof(T));

            // Fill in RGBA
            for (size_t i = 0; i < d_width * d_height; i++)
                channel(3)[i] = std::numeric_limits<T>::max();
        }
    }

    template <typename T>
    Image<uint8_t> Image<T>::to8bits()
    {
        if (d_depth == 8)
        {
            return *((image::Image<uint8_t> *)this);
        }
        else if (d_depth == 16)
        {
            image::Image<uint8_t> image8(d_width, d_height, d_channels);
            for (size_t i = 0; i < data_size; i++)
                image8[i] = d_data[i] >> 8;
            return image8;
        }

        return Image<uint8_t>(); // This should never happen
    }

    template <typename T>
    Image<uint16_t> Image<T>::to16bits()
    {
        if (d_depth == 16)
        {
            return *((image::Image<uint16_t> *)this);
        }
        else if (d_depth == 8)
        {
            image::Image<uint16_t> image16(d_width, d_height, d_channels);
            for (size_t i = 0; i < data_size; i++)
                image16[i] = d_data[i] << 8;
            return image16;
        }

        return Image<uint16_t>(); // This should never happen
    }

    template <typename T>
    void Image<T>::load_img(std::string file)
    {
        std::ifstream file_sigature_src(file, std::ios::binary);
        uint8_t signature[10];
        file_sigature_src.read((char *)signature, 10);
        if (signature[0] == 0xFF && signature[1] == 0xD8)
            load_jpeg(file);
        else if (signature[0] == 0x89 && signature[1] == 0x50 && signature[2] == 0x4E && signature[3] == 0x47)
            load_png(file);
        else if (signature[0] == 0xff && signature[1] == 0x4f && signature[2] == 0xff && signature[3] == 0x51)
            load_j2k(file);
        else if (signature[0] == 'P' && (signature[1] == '5' || signature[1] == '6'))
            load_pbm(file);
        else if (signature[0] == 'I' && signature[1] == 'I' && signature[2] == '*')
            load_tiff(file);
    }

    template <typename T>
    void Image<T>::load_img(uint8_t *buffer, int size)
    {
        if (buffer[0] == 0xFF && buffer[1] == 0xD8)
            load_jpeg(buffer, size);
        else if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E && buffer[3] == 0x47)
            load_png(buffer, size);
    }

    template <typename T>
    void Image<T>::save_img(std::string file, bool fast)
    {
        if (!append_ext(&file))
            return;
        logger->info("Saving " + file + "...");
        if (file.find(".png") != std::string::npos)
            save_png(file, fast);
        else if (file.find(".jpeg") != std::string::npos || file.find(".jpg") != std::string::npos)
            save_jpeg(file);
        else if (file.find(".j2k") != std::string::npos)
            save_j2k(file);
        else if ((file.find(".ppm") != std::string::npos) || (file.find(".pgm") != std::string::npos) || (file.find(".pbm") != std::string::npos))
            save_pbm(file);
        else if ((file.find(".tif") != std::string::npos) || (file.find(".gtif") != std::string::npos) || (file.find(".tiff") != std::string::npos))
            save_tiff(file);
    }

    // Append selected file extension
    template <typename T>
    bool Image<T>::append_ext(std::string *file)
    {
        // Do nothing if there's already an extension
        if (file->find(".png") != std::string::npos ||
            file->find(".jpeg") != std::string::npos ||
            file->find(".jpg") != std::string::npos ||
            file->find(".j2k") != std::string::npos ||
            file->find(".pgm") != std::string::npos ||
            file->find(".pbm") != std::string::npos ||
            file->find(".ppm") != std::string::npos ||
            file->find(".tif") != std::string::npos ||
            file->find(".tiff") != std::string::npos ||
            file->find(".gtif") != std::string::npos)
            return true;

        // Otherwise, load the user setting
        std::string image_format;
        try
        {
            image_format = satdump::config::main_cfg["satdump_general"]["image_format"]["value"];
        }
        catch (std::exception &e)
        {
            logger->error("Image format not specified, and default format cannot be found! %s", e.what());
            return false;
        }

        if (image_format != "png" && image_format != "jpg" && image_format != "j2k" && image_format != "pbm" && image_format != "tif")
        {
            logger->error("Image format not specified, and default format is invalid!");
            return false;
        }

        *file += "." + image_format;
        return true;
    }

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}
