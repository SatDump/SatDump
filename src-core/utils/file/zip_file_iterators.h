#pragma once

#include "core/exception.h"
#include "file_iterators.h"
#include "libs/miniz/miniz.h"
#include <filesystem>

namespace satdump
{
    namespace utils
    {
        class ZipFileIteratorItem : public FilesIteratorItem
        {
        private:
            const mz_zip_archive *zip;
            const int num;

        public:
            ZipFileIteratorItem(mz_zip_archive *zip, int num, std::string name) : FilesIteratorItem(name), zip(zip), num(num) {}
            ~ZipFileIteratorItem() {}

            std::vector<uint8_t> getPayload()
            {
                size_t filesize = 0;
                void *file_ptr = mz_zip_reader_extract_to_heap((mz_zip_archive *)zip, num, &filesize, 0);
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
                bool vv = file_index < numfiles;

                if (vv)
                {
                    v.reset();

                    if (mz_zip_reader_is_file_supported(&zip, file_index))
                    {
                        char filename[2000]; // TODOREWORK?
                        if (mz_zip_reader_get_filename(&zip, file_index, filename, 2000))
                            v = std::make_unique<ZipFileIteratorItem>(&zip, file_index, std::filesystem::path(filename).stem().string() + std::filesystem::path(filename).extension().string());
                    }

                    file_index++;
                }

                return vv;
            }

            void reset() { file_index = 0; }
        };
    } // namespace utils
} // namespace satdump