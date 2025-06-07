#include "file.h"
#include <fstream>

namespace satdump
{
    // Return filesize
    uint64_t getFilesize(std::string filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        uint64_t fileSize = file.tellg();
        file.close();
        return fileSize;
    }
} // namespace satdump