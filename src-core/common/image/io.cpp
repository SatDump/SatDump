#include "io.h"
#include <fstream>
#include "logger.h"
#include "core/config.h"

namespace image
{
    void load_img(Image &img, std::string file)
    {
        std::ifstream file_sigature_src(file, std::ios::binary);
        uint8_t signature[10];
        file_sigature_src.read((char *)signature, 10);
        if (signature[0] == 0xFF && signature[1] == 0xD8)
            load_jpeg(img, file);
        else if (signature[0] == 0x89 && signature[1] == 0x50 && signature[2] == 0x4E && signature[3] == 0x47)
            load_png(img, file);
        else if (signature[0] == 0xff && signature[1] == 0x4f && signature[2] == 0xff && signature[3] == 0x51)
            load_j2k(img, file);
        else if (signature[0] == 'P' && (signature[1] == '5' || signature[1] == '6'))
            load_pbm(img, file);
        else if (signature[0] == 'I' && signature[1] == 'I' && signature[2] == '*')
            load_tiff(img, file);
        else if (signature[0] == 'q' && signature[1] == 'o' && signature[2] == 'i' && signature[3] == 'f')
            load_qoi(img, file);
    }

    void load_img(Image &img, uint8_t *buffer, int size)
    {
        if (buffer[0] == 0xFF && buffer[1] == 0xD8)
            load_jpeg(img, buffer, size);
        else if (buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E && buffer[3] == 0x47)
            load_png(img, buffer, size);
    }

    void save_img(Image &img, std::string file, bool fast)
    {
        if (!append_ext(&file))
            return;
        logger->info("Saving " + file + "...");
        if (file.find(".png") != std::string::npos)
            save_png(img, file, fast);
        else if (file.find(".jpeg") != std::string::npos || file.find(".jpg") != std::string::npos)
            save_jpeg(img, file);
        else if (file.find(".j2k") != std::string::npos)
            save_j2k(img, file);
        else if ((file.find(".ppm") != std::string::npos) || (file.find(".pgm") != std::string::npos) || (file.find(".pbm") != std::string::npos))
            save_pbm(img, file);
        else if ((file.find(".tif") != std::string::npos) || (file.find(".gtif") != std::string::npos) || (file.find(".tiff") != std::string::npos))
            save_tiff(img, file);
        else if (file.find(".qoi") != std::string::npos)
            save_qoi(img, file);
    }

    // Append selected file extension
    bool append_ext(std::string *file, bool prod)
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
            file->find(".gtif") != std::string::npos ||
            file->find(".qoi") != std::string::npos)
            return true;

        // Otherwise, load the user setting
        std::string image_format;
        try
        {
            if (prod)
                image_format = satdump::config::main_cfg["satdump_general"]["product_format"]["value"];
            else
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
}