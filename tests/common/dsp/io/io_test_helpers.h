#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace dsp_test_io
{
    // Unique path under the system temp directory, removed (with any extension variants) on destruction
    class TempPath
    {
    public:
        explicit TempPath(const std::string &tag)
        {
            static std::atomic<int> counter{0};
            base = (std::filesystem::temp_directory_path() /
                    ("satdump_dsp_test_" + tag + "_" + std::to_string(getpid()) + "_" + std::to_string(counter++)))
                       .string();
        }

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

    template <typename T>
    inline void writeFile(const std::string &path, const std::vector<T> &data)
    {
        std::ofstream f(path, std::ios::binary);
        f.write((const char *)data.data(), data.size() * sizeof(T));
    }

    inline std::vector<uint8_t> readFile(const std::string &path)
    {
        std::ifstream f(path, std::ios::binary);
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    template <typename T>
    inline T readLE(const std::vector<uint8_t> &b, size_t off)
    {
        T v = 0;
        for (size_t i = 0; i < sizeof(T); i++)
            v |= (T)b[off + i] << (8 * i);
        return v;
    }
}
