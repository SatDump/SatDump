#include "helpers/temp_files.h"
#include "libs/miniz/miniz.h"
#include "utils/file/folder_file_iterators.h"
#include "utils/file/zip_file_iterators.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <map>

using satdump::utils::FilesIteratorItem;
using satdump::utils::FolderFileIteratorItem;
using satdump::utils::FolderFilesIterator;
using satdump::utils::ZipFilesIterator;

namespace
{
    using test_helpers::TempDir;
    using test_helpers::uniqueTempPath;
    using test_helpers::writeBytes;

    std::vector<uint8_t> bytes(const std::string &s) { return std::vector<uint8_t>(s.begin(), s.end()); }

    std::vector<uint8_t> patterned(size_t n)
    {
        std::vector<uint8_t> v(n);
        for (size_t i = 0; i < n; i++)
            v[i] = (uint8_t)((i * 7) ^ (i >> 8));
        return v;
    }

    // Drains an iterator, returning name -> payload for every file item and the number of entries seen
    std::map<std::string, std::vector<uint8_t>> drain(satdump::utils::FilesIterator &it, int &entries)
    {
        std::map<std::string, std::vector<uint8_t>> out;
        std::shared_ptr<FilesIteratorItem> item;
        entries = 0;
        while (it.getNext(item))
        {
            entries++;
            if (item)
                out[item->name] = item->getPayload();
        }
        return out;
    }

    struct TempZip
    {
        std::filesystem::path path;
        TempZip(const std::map<std::string, std::vector<uint8_t>> &entries) : path(uniqueTempPath("archive.zip"))
        {
            for (auto &e : entries)
                if (!mz_zip_add_mem_to_archive_file_in_place(path.string().c_str(), e.first.c_str(), e.second.data(), e.second.size(), nullptr, 0, MZ_DEFAULT_COMPRESSION))
                    throw std::runtime_error("Could not build test zip");
        }
        ~TempZip()
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    };
} // namespace

TEST_CASE("FolderFileIteratorItem reads the whole file and names it by filename", "[utils][file_iterators]")
{
    TempDir dir("item");
    auto data = patterned(5000);
    writeBytes(dir.path / "payload.bin", data);

    FolderFileIteratorItem item((dir.path / "payload.bin").string());
    CHECK(item.name == "payload.bin");
    CHECK(item.getPayload() == data);

    FolderFileIteratorItem named((dir.path / "payload.bin").string(), "custom");
    CHECK(named.name == "custom");
    CHECK(named.getPayload() == data);
}

TEST_CASE("FolderFileIteratorItem payload of a deleted file throws", "[utils][file_iterators]")
{
    // tellg() returns -1 for a file that cannot be opened, so the resize to SIZE_MAX throws
    FolderFileIteratorItem item(uniqueTempPath("gone.bin").string());
    CHECK_THROWS(item.getPayload());
}

TEST_CASE("FolderFilesIterator walks files recursively", "[utils][file_iterators]")
{
    TempDir dir("folder");
    std::filesystem::create_directories(dir.path / "sub" / "deeper");
    writeBytes(dir.path / "a.txt", bytes("alpha"));
    writeBytes(dir.path / "sub" / "b.dat", patterned(70000));
    writeBytes(dir.path / "sub" / "deeper" / "c", bytes("charlie"));

    FolderFilesIterator it(dir.path.string());
    int entries = 0;
    auto files = drain(it, entries);

    // Directories are reported as entries with a null item
    CHECK(entries == 5);
    REQUIRE(files.size() == 3);
    CHECK(files["a.txt"] == bytes("alpha"));
    CHECK(files["b.dat"] == patterned(70000));
    CHECK(files["c"] == bytes("charlie"));

    SECTION("getNext keeps returning false once exhausted")
    {
        std::shared_ptr<FilesIteratorItem> item = std::make_shared<FolderFileIteratorItem>("x", "x");
        CHECK_FALSE(it.getNext(item));
        CHECK_FALSE(it.getNext(item));
    }

    SECTION("reset restarts the walk")
    {
        it.reset();
        int entries2 = 0;
        auto files2 = drain(it, entries2);
        CHECK(entries2 == 5);
        CHECK(files2 == files);
    }
}

TEST_CASE("FolderFilesIterator on an empty folder yields nothing", "[utils][file_iterators]")
{
    TempDir dir("empty_folder");
    FolderFilesIterator it(dir.path.string());
    std::shared_ptr<FilesIteratorItem> item;
    CHECK_FALSE(it.getNext(item));
    CHECK(item == nullptr);
}

TEST_CASE("FolderFilesIterator on a missing folder throws", "[utils][file_iterators]")
{
    CHECK_THROWS_AS(FolderFilesIterator(uniqueTempPath("no_such_dir").string()), std::filesystem::filesystem_error);
}

TEST_CASE("ZipFilesIterator lists and extracts archive entries", "[utils][file_iterators]")
{
    std::map<std::string, std::vector<uint8_t>> content = {
        {"one.txt", bytes("first file")},
        {"nested/dir/two.bin", patterned(100000)},
        {"zeros.raw", std::vector<uint8_t>(20000, 0)},
    };
    TempZip zip(content);

    ZipFilesIterator it(zip.path.string());
    int entries = 0;
    auto files = drain(it, entries);

    CHECK(entries == 3);
    REQUIRE(files.size() == 3);
    // Directory components inside the archive are dropped from the item name
    CHECK(files["one.txt"] == content["one.txt"]);
    CHECK(files["two.bin"] == content["nested/dir/two.bin"]);
    CHECK(files["zeros.raw"] == content["zeros.raw"]);

    SECTION("reset restarts from the first entry")
    {
        std::shared_ptr<FilesIteratorItem> item;
        CHECK_FALSE(it.getNext(item));
        it.reset();
        int entries2 = 0;
        auto files2 = drain(it, entries2);
        CHECK(entries2 == 3);
        CHECK(files2 == files);
    }
}

TEST_CASE("ZipFilesIterator handles an empty entry", "[utils][file_iterators]")
{
    TempZip zip({{"empty.txt", {}}, {"full.txt", bytes("data")}});
    ZipFilesIterator it(zip.path.string());
    int entries = 0;
    auto files = drain(it, entries);
    CHECK(entries == 2);
    CHECK(files.count("empty.txt") == 1);
    CHECK(files["empty.txt"].empty());
    CHECK(files["full.txt"] == bytes("data"));
}

TEST_CASE("ZipFilesIterator rejects invalid archives", "[utils][file_iterators]")
{
    CHECK_THROWS_AS(ZipFilesIterator(uniqueTempPath("missing.zip").string()), satdump::satdump_exception_t);

    TempDir dir("notzip");
    writeBytes(dir.path / "fake.zip", bytes("this is definitely not a zip archive"));
    CHECK_THROWS_AS(ZipFilesIterator((dir.path / "fake.zip").string()), satdump::satdump_exception_t);
}
