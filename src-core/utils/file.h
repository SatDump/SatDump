#pragma once

/**
 * @file file.h
 * @brief File manipulation utilities
 */

#include <cstdint>
#include <string>

namespace satdump
{
    /**
     * @brief Gets the filesize in bytes
     *
     * @param filepath path of the file to check
     * @return size in bytes
     */
    uint64_t getFilesize(std::string filepath);
} // namespace satdump