#include "helpers/temp_files.h"
#include "utils/file.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

namespace
{
    using test_helpers::TempFile;
    using test_helpers::uniqueTempPath;
} // namespace

TEST_CASE("satdump::getFilesize returns the size in bytes", "[utils][file]")
{
    TempFile small("small.bin", std::string(17, 'z'));
    CHECK(satdump::getFilesize(small.path.string()) == 17);

    TempFile big("big.bin", std::string(3 * 1024 * 1024 + 5, 'z'));
    CHECK(satdump::getFilesize(big.path.string()) == 3 * 1024 * 1024 + 5);

    TempFile empty("empty.bin", std::string(0, 'z'));
    CHECK(satdump::getFilesize(empty.path.string()) == 0);
}

TEST_CASE("satdump::getFilesize on a missing file returns all ones", "[utils][file]")
{
    // tellg() fails with -1, which converts to the maximum uint64_t
    CHECK(satdump::getFilesize(uniqueTempPath("does_not_exist.bin").string()) == UINT64_MAX);
}
