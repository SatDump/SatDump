#pragma once

#include "helpers/file_io.h"
#include "helpers/temp_files.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dsp_test_io
{
    // Unique path under the system temp directory, removed (with any extension variants) on destruction
    class TempPath
    {
    public:
        explicit TempPath(const std::string &tag) : base(test_helpers::uniqueTempPath("dsp_" + tag).string()) {}

        ~TempPath()
        {
            std::error_code ec;
            for (const auto &p : created)
                std::filesystem::remove(p, ec);
            std::filesystem::remove(base, ec);
        }

        TempPath(const TempPath &) = delete;
        TempPath &operator=(const TempPath &) = delete;

        // Base path without extension
        const std::string &path() const { return base; }

        // Registers another file to be cleaned up
        std::string with(const std::string &suffix)
        {
            created.push_back(base + suffix);
            return created.back();
        }

    private:
        std::string base;
        std::vector<std::string> created;
    };

    using test_helpers::readBytes;
    using test_helpers::writeBytes;

    template <typename T>
    inline T readLE(const std::vector<uint8_t> &b, size_t off)
    {
        T v = 0;
        for (size_t i = 0; i < sizeof(T); i++)
            v |= (T)b[off + i] << (8 * i);
        return v;
    }
} // namespace dsp_test_io
