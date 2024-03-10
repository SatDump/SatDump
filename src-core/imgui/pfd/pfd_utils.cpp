#include <algorithm>
#include <sstream>
#include "portable-file-dialogs.h"
#include "core/config.h"
#include "logger.h"
#include "pfd_utils.h"

#ifdef _MSC_VER
#include <direct.h>
#endif

namespace satdump
{
    template <typename T>
    std::string save_image_dialog(std::string default_name, std::string default_path, std::string window_title, image::Image<T> *image, std::string *default_ext)
    {
        std::vector<std::string> saveopts = {
            "PNG Files", "*.png",
            "JPEG 2000 Files", "*.j2k",
            "JPEG Files", "*.jpg *.jpeg",
            "PBM Files", "*.pbm *.pgm *.ppm",
            "TIFF Files", "*.tif *.tiff *.gtif"
        };

        size_t i = 1;
        for (auto it = saveopts.begin() + 1 ;; it += 2)
        {
            std::stringstream ss(*it);
            std::string token;

            while (std::getline(ss, token, ' '))
            {
                if (token.substr(2) == *default_ext)
                {
                    std::rotate(saveopts.begin(), it - 1, it);
                    std::rotate(saveopts.begin() + 1, it, it + 1);
                    goto done_ext;
                }
            }

            i += 2;
            if (i >= saveopts.size())
                break;
        }

done_ext:
#ifdef __ANDROID__
        *default_ext = config::main_cfg["satdump_general"]["image_format"]["value"].get<std::string>();
#endif

#if defined(_MSC_VER)
        if (default_path == ".")
        {
            char *cwd;
            cwd = _getcwd(NULL, 0);
            if (cwd != 0)
                default_path = cwd;
        }
        default_path += "\\";
#elif defined(__ANDROID__)
        if (default_path == ".")
            default_path = "/storage/emulated/0";
        default_path += "/";
#else
        default_path += "/";
#endif
        std::string save_name = default_path + default_name + "." + *default_ext;
        std::string path = "";

#ifndef __ANDROID__
        auto result = pfd::save_file(window_title, save_name, saveopts);
        while (!result.ready(1000))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (result.result().size() > 0)
        {
            path = result.result();
            image->save_img(path);
            std::string extension = std::filesystem::path(path).extension().string();
            if (extension.size() > 1)
                *default_ext = extension.substr(1);
        }
#else
        path = save_name;
        image->save_img(save_name);
#endif
        return path;
    }

    template std::string save_image_dialog<uint8_t>(std::string default_name, std::string default_path, std::string window_title, image::Image<uint8_t> *image, std::string *default_ext);
    template std::string save_image_dialog<uint16_t>(std::string default_name, std::string default_path, std::string window_title, image::Image<uint16_t> *image, std::string *default_ext);
}
