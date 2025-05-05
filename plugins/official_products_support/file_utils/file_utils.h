#pragma once

#include <string>
#include <fstream>

namespace satdump
{
    inline bool file_is_zip(std::string path)
    {
        char sign[2];
        std::ifstream(path, std::ios::binary).read(sign, 2);
        return sign[0] == 'P' && sign[1] == 'K';
    }

    std::string try_get_eumetsat_id(std::string zippath);
}