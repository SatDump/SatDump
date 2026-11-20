#pragma once

/*
Small whole-file read / write helpers, shared by the whole suite.
Everything is inline and lives in the test_helpers namespace, as every test
file is linked into the same executable.

- Files are always opened in binary mode.
- Writers create the missing parent directories, and fail silently (like a
  plain std::ofstream) if the file cannot be written.
- Readers return an empty string / vector if the file cannot be read.
- writeBytes / readBytes default to uint8_t, and take any trivially copyable
  element type for raw sample files (writeBytes(p, floats), readBytes<float>(p)).
  A braced list is a byte vector: writeBytes(p, {0xFF, 0x4F}).
*/

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace test_helpers
{
    namespace detail
    {
        inline void createParentDirs(const std::filesystem::path &path)
        {
            std::error_code ec;
            if (path.has_parent_path())
                std::filesystem::create_directories(path.parent_path(), ec);
        }
    } // namespace detail

    // Writes content to path (binary), replacing the file
    inline void writeFile(const std::filesystem::path &path, const std::string &content)
    {
        detail::createParentDirs(path);
        std::ofstream f(path, std::ios::binary);
        f.write(content.data(), content.size());
    }

    // Writes the raw bytes of data to path, replacing the file
    template <typename T = uint8_t>
    inline void writeBytes(const std::filesystem::path &path, const std::vector<T> &data)
    {
        static_assert(std::is_trivially_copyable_v<T>, "writeBytes needs a trivially copyable element type");
        detail::createParentDirs(path);
        std::ofstream f(path, std::ios::binary);
        f.write((const char *)data.data(), data.size() * sizeof(T));
    }

    // Whole file as a string (empty if it cannot be read)
    inline std::string readFile(const std::filesystem::path &path)
    {
        std::ifstream f(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    }

    // Whole file as a vector of T (empty if it cannot be read). A trailing partial element is dropped
    template <typename T = uint8_t>
    inline std::vector<T> readBytes(const std::filesystem::path &path)
    {
        static_assert(std::is_trivially_copyable_v<T>, "readBytes needs a trivially copyable element type");
        std::string raw = readFile(path);
        std::vector<T> out(raw.size() / sizeof(T));
        if (!out.empty())
            memcpy((void *)out.data(), raw.data(), out.size() * sizeof(T));
        return out;
    }
} // namespace test_helpers
