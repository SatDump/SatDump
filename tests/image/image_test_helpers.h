#pragma once

// Shared helpers for the src-core/image tests. Everything lives in the
// image_test namespace and is inline, as all test files link into one binary.

#include "core/config.h"
#include "image/image.h"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>

namespace image_test
{
    // Unique temporary directory, removed (with its contents) on destruction
    struct TempDir
    {
        std::filesystem::path path;

        TempDir()
        {
            static std::atomic<int> counter{0};
            std::mt19937 rng(std::random_device{}());
            path = std::filesystem::temp_directory_path() /
                   ("satdump_test_image_" + std::to_string(getpid()) + "_" + std::to_string(counter++) + "_" + std::to_string(rng()));
            std::filesystem::create_directories(path);
        }

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        std::string file(const std::string &name) const { return (path / name).string(); }

        TempDir(const TempDir &) = delete;
        TempDir &operator=(const TempDir &) = delete;
    };

    // Saves the global SatDump config and restores it on destruction
    struct ConfigGuard
    {
        nlohmann::ordered_json saved;
        ConfigGuard() : saved(satdump::satdump_cfg.main_cfg) {}
        ~ConfigGuard() { satdump::satdump_cfg.main_cfg = saved; }
    };

    // Deterministic pattern image: every sample is distinct-ish and covers the range
    inline satdump::image::Image makePattern(int depth, size_t width, size_t height, int channels, uint32_t seed = 1)
    {
        satdump::image::Image img(depth, width, height, channels);
        uint32_t state = seed * 2654435761u + 12345u;
        for (size_t i = 0; i < img.size(); i++)
        {
            state = state * 1664525u + 1013904223u;
            img.set(i, (state >> 8) % (img.maxval() + 1));
        }
        return img;
    }

    // Smooth gradient image (friendly to lossy codecs)
    inline satdump::image::Image makeGradient(int depth, size_t width, size_t height, int channels)
    {
        satdump::image::Image img(depth, width, height, channels);
        for (int c = 0; c < channels; c++)
            for (size_t y = 0; y < height; y++)
                for (size_t x = 0; x < width; x++)
                {
                    double v = (double(x) / (width - 1) * 0.6 + double(y) / (height - 1) * 0.3 + c * 0.03);
                    img.set(c, x, y, int(v * img.maxval()));
                }
        return img;
    }

    inline bool sameShape(satdump::image::Image &a, satdump::image::Image &b)
    {
        return a.depth() == b.depth() && a.width() == b.width() && a.height() == b.height() && a.channels() == b.channels();
    }

    inline bool samePixels(satdump::image::Image &a, satdump::image::Image &b)
    {
        if (!sameShape(a, b))
            return false;
        for (size_t i = 0; i < a.size(); i++)
            if (a.get(i) != b.get(i))
                return false;
        return true;
    }

    inline int maxAbsDiff(satdump::image::Image &a, satdump::image::Image &b)
    {
        int m = 0;
        for (size_t i = 0; i < a.size() && i < b.size(); i++)
            m = std::max(m, std::abs(a.get(i) - b.get(i)));
        return m;
    }

    inline std::vector<uint8_t> readFile(const std::string &path)
    {
        std::ifstream f(path, std::ios::binary);
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    inline void writeFile(const std::string &path, const std::vector<uint8_t> &data)
    {
        std::ofstream f(path, std::ios::binary);
        f.write((const char *)data.data(), data.size());
    }
} // namespace image_test
