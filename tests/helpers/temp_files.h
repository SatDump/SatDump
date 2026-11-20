#pragma once

/*
Self-removing temporary files and directories, shared by the whole suite.
Everything is inline and lives in the test_helpers namespace, as every test
file is linked into the same executable.

- uniqueTempPath(name) returns a path in the system temp directory that no
  other thread or concurrently running test process uses. name is kept at
  the end of the file name, so extensions survive ("x.wav" -> ..._x.wav).
  Nothing is created.
- TempDir creates a unique directory, and removes it with its contents on
  destruction. dir.path / "x", dir.file("x") (a std::string) and dir.str().
- TempFile reserves a unique file path, optionally writing it right away,
  and removes the file (if it exists) on destruction. f.path and f.str().
Removal never throws, both classes are non-copyable.
*/

#include "helpers/file_io.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

namespace test_helpers
{
    namespace detail
    {
        // Random per-process tag. std::random_device is mixed with the clock and
        // an address, as some platforms implement it deterministically.
        inline const std::string &processTag()
        {
            static const std::string tag = []()
            {
                std::random_device rd;
                uint64_t v = ((uint64_t)rd() << 32) ^ rd();
                v ^= (uint64_t)std::chrono::system_clock::now().time_since_epoch().count();
                v ^= (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count() * 0x9E3779B97F4A7C15ull;
                v ^= (uint64_t)(uintptr_t)&rd;
                return std::to_string(v);
            }();
            return tag;
        }
    } // namespace detail

    // Unique path in the temp directory, ending with name. Nothing is created
    inline std::filesystem::path uniqueTempPath(const std::string &name)
    {
        static std::atomic<uint64_t> counter{0};
        return std::filesystem::temp_directory_path() / ("satdump_test_" + detail::processTag() + "_" + std::to_string(counter++) + "_" + name);
    }

    // Unique directory, created on construction and removed with its contents on destruction
    class TempDir
    {
    public:
        const std::filesystem::path path;

        explicit TempDir(const std::string &tag = "dir") : path(create(tag)) {}

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        TempDir(const TempDir &) = delete;
        TempDir &operator=(const TempDir &) = delete;

        std::string str() const { return path.string(); }
        // Path of an entry inside the directory (not created)
        std::string file(const std::string &name) const { return (path / name).string(); }

    private:
        static std::filesystem::path create(const std::string &tag)
        {
            // create_directories() returns false if the directory already existed
            for (int attempt = 0; attempt < 100; attempt++)
            {
                std::filesystem::path p = uniqueTempPath(tag);
                if (std::filesystem::create_directories(p))
                    return p;
            }
            throw std::runtime_error("Could not create a unique temporary directory");
        }
    };

    // Unique file path, removed (if the file exists) on destruction
    class TempFile
    {
    public:
        const std::filesystem::path path;

        // Only reserves the path, the file is not created
        explicit TempFile(const std::string &name = "file.bin") : path(uniqueTempPath(name)) {}

        // Creates the file with this content
        TempFile(const std::string &name, const std::string &content) : TempFile(name) { writeFile(path, content); }

        // Creates the file with size raw bytes
        TempFile(const std::string &name, const void *data, size_t size) : TempFile(name)
        {
            std::ofstream f(path, std::ios::binary);
            f.write((const char *)data, size);
        }

        // A number is not content (0 would silently convert to a null const char *)
        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        TempFile(const std::string &name, T) = delete;

        ~TempFile()
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }

        TempFile(const TempFile &) = delete;
        TempFile &operator=(const TempFile &) = delete;

        std::string str() const { return path.string(); }
    };
} // namespace test_helpers
