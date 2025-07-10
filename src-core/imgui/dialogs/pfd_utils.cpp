#include "pfd_utils.h"
#include "core/backend.h"
#include "core/config.h"
#include "logger.h"
#include <algorithm>
#include <cstdio>
#include <sstream>

#ifdef _MSC_VER
#include <direct.h>
#endif

#include "image/io.h"

// TODOREWORK move/rename this file
namespace satdump
{
    std::string save_image_dialog(std::string default_name, std::string default_path, std::string window_title, image::Image *image, std::string *default_ext)
    {
        std::vector<std::string> saveopts = {"PNG Files",         "*.png",      "JPEG 2000 Files",     "*.j2k",     "JPEG Files", "*.jpg *.jpeg", "PBM Files",
                                             "*.pbm *.pgm *.ppm", "TIFF Files", "*.tif *.tiff *.gtif", "QOI Files", "*.qoi"};

        size_t i = 1;
        for (auto it = saveopts.begin() + 1;; it += 2)
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
        *default_ext = satdump_cfg.getValueFromSatDumpGeneral<std::string>("image_format");
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
        std::string save_name = default_name + "." + *default_ext;

#ifndef __ANDROID__
        std::vector<std::pair<std::string, std::string>> final_saveopt;
        for (int i = 0; i < saveopts.size() / 2; i++)
            final_saveopt.push_back({saveopts[i * 2 + 0], saveopts[i * 2 + 1]});

        std::string path = backend::saveFileDialog(final_saveopt, default_path, save_name);

        if (path.size() > 0)
        {
            image::save_img(*image, path);
            std::string extension = std::filesystem::path(path).extension().string();
            if (extension.size() > 1)
                *default_ext = extension.substr(1);
        }
#else
        std::string path = save_name;
        image::save_img(*image, save_name);
#endif
        return path;
    }
} // namespace satdump
