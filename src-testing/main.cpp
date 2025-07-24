/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "core/exception.h"
#include "init.h"
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "logger.h"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>

class FilesIteratorItem
{
public:
    const std::string name;
    FilesIteratorItem(std::string name) : name(name) {}
    virtual std::vector<uint8_t> getPayload() = 0;
};

class FilesIterator
{
public:
    virtual bool getNext(std::unique_ptr<FilesIteratorItem> &v) = 0;
    virtual void reset() = 0;
};

class FolderFileIteratorItem : public FilesIteratorItem
{
private:
    const std::string path;

public:
    FolderFileIteratorItem(std::string path, std::string name) : FilesIteratorItem(name), path(path) {}

    std::vector<uint8_t> getPayload()
    {
        std::vector<uint8_t> v;
        std::ifstream f(path, std::ios::binary);
        char c;
        while (!f.eof())
        {
            f.read(&c, 1);
            v.push_back(c);
        }
        f.close();
        return v;
    }
};

class FolderFilesIterator : public FilesIterator
{
private:
    const std::string folder;
    std::filesystem::recursive_directory_iterator filesIterator;
    std::error_code iteratorError;

public:
    FolderFilesIterator(std::string folder) : folder(folder) { filesIterator = std::filesystem::recursive_directory_iterator(folder); }

    bool getNext(std::unique_ptr<FilesIteratorItem> &v)
    {
        v.reset();

        std::string path = filesIterator->path();

        if (std::filesystem::is_regular_file(path))
            v = std::make_unique<FolderFileIteratorItem>(path, std::filesystem::path(path).stem().string() + std::filesystem::path(path).extension().string());

        filesIterator.increment(iteratorError);
        if (iteratorError)
            throw satdump_exception(iteratorError.message());

        return filesIterator != std::filesystem::recursive_directory_iterator();
    }

    void reset() { filesIterator = std::filesystem::recursive_directory_iterator(folder); }
};

class ZipFileIteratorItem : public FilesIteratorItem
{
private:
    const mz_zip_archive *zip;
    const int num;

public:
    ZipFileIteratorItem(mz_zip_archive *zip, int num, std::string name) : FilesIteratorItem(name), zip(zip), num(num) {}

    std::vector<uint8_t> getPayload()
    {
        size_t filesize = 0;
        void *file_ptr = mz_zip_reader_extract_to_heap((mz_zip_archive *)&zip, num, &filesize, 0);
        std::vector<uint8_t> vec((uint8_t *)file_ptr, (uint8_t *)file_ptr + filesize);
        mz_free(file_ptr);
        return vec;
    }
};

class ZipFilesIterator : public FilesIterator
{
private:
    mz_zip_archive zip{};
    int numfiles;
    int file_index;

public:
    ZipFilesIterator(std::string zipfile)
    {
        if (!mz_zip_reader_init_file(&zip, zipfile.c_str(), 0))
            throw satdump_exception("Invalid zip file! " + zipfile);
        numfiles = mz_zip_reader_get_num_files(&zip);
        file_index = 0;
    }

    ~ZipFilesIterator() { mz_zip_reader_end(&zip); }

    bool getNext(std::unique_ptr<FilesIteratorItem> &v)
    {
        v.reset();

        if (mz_zip_reader_is_file_supported(&zip, file_index))
        {
            char filename[2000];
            if (mz_zip_reader_get_filename(&zip, file_index, filename, 2000))
                v = std::make_unique<ZipFileIteratorItem>(&zip, file_index, std::filesystem::path(filename).stem().string() + std::filesystem::path(filename).extension().string());
        }

        file_index++;

        return file_index < numfiles;
    }

    void reset() { file_index = 0; }
};

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    // std::unique_ptr<FilesIterator> fit = std::make_unique<FolderFilesIterator>(
    //     "/tmp/satdump_official/W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI1+FCI-1C-RRAD-FDHSI-FD--x-x---x_C_EUMT_20250724111342_IDPFI_OPE_20250724111007_20250724111935_N__O_0068_0000 (2)");

    std::unique_ptr<FilesIterator> fit = std::make_unique<ZipFilesIterator>(
        "/tmp/satdump_official/W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI1+FCI-1C-RRAD-FDHSI-FD--x-x---x_C_EUMT_20250724120349_IDPFI_OPE_20250724120007_20250724120935_N__O_0073_0000.zip");

    std::unique_ptr<FilesIteratorItem> f;

    while (fit->getNext(f))
    {
        if (f)
        {

            uint32_t test1, test2, test3;
            if (sscanf(f->name.c_str(), "W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI%*d+FCI-1C-RRAD-FDHSI-FD--CHK-BODY---NC4E_C_EUMT_%14u_IDPFI_OPE_%14u_%14u_N__O_%*d_%*d.nc", &test1, &test2, &test3) == 3)
            {
                logger->trace(f->name + " MTG FCI Images");
            }
            else
            {
                logger->info(f->name);
            }
        }
    }
}
