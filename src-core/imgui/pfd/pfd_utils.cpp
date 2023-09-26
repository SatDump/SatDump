#include "core/config.h"
#include "pfd_utils.h"

namespace satdump
{
    std::vector<std::string> get_file_formats()
    {
        std::string default_ext = satdump::config::main_cfg["satdump_general"]["image_format"]["value"].get<std::string>();
        std::vector<std::string> saveopts = {
            "PNG Files", "*.png",
            "JPEG 2000 Files", "*.j2k",
            "JPEG Files", "*.jpg *.jpeg"
        };

        for (auto it = saveopts.begin() + 1, lim = saveopts.end(); it != lim; it += 2)
        {
            if (it->substr(2, 3) == default_ext)
            {
                std::rotate(saveopts.begin(), it - 1, it);
                std::rotate(saveopts.begin() + 1, it, it + 1);
                break;
            }
        }

        return saveopts;
    }
}