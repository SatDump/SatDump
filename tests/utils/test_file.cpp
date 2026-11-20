#include "utils/file.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <random>

namespace
{
    std::filesystem::path uniqueTempPath(const std::string &name)
    {
        std::mt19937 rng(std::random_device{}());
        return std::filesystem::temp_directory_path() / ("satdump_test_file_" + std::to_string(rng()) + "_" + name);
    }

    struct TempFile
    {
        std::filesystem::path path;
        TempFile(const std::string &name, size_t size) : path(uniqueTempPath(name))
        {
            std::ofstream f(path, std::ios::binary);
            std::string data(size, 'z');
            f.write(data.data(), data.size());
        }
        ~TempFile() { std::filesystem::remove(path); }
    };
} // namespace

TEST_CASE("satdump::getFilesize returns the size in bytes", "[utils][file]")
{
    TempFile small("small.bin", 17);
    CHECK(satdump::getFilesize(small.path.string()) == 17);

    TempFile big("big.bin", 3 * 1024 * 1024 + 5);
    CHECK(satdump::getFilesize(big.path.string()) == 3 * 1024 * 1024 + 5);

    TempFile empty("empty.bin", 0);
    CHECK(satdump::getFilesize(empty.path.string()) == 0);
}

TEST_CASE("satdump::getFilesize on a missing file returns all ones", "[utils][file]")
{
    // tellg() fails with -1, which converts to the maximum uint64_t
    CHECK(satdump::getFilesize(uniqueTempPath("does_not_exist.bin").string()) == UINT64_MAX);
}
