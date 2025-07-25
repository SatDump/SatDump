#pragma once

#include "core/exception.h"
#include "file_iterators.h"
#include <filesystem>
#include <fstream>

namespace satdump
{
    namespace utils
    {

        class FolderFileIteratorItem : public FilesIteratorItem
        {
        private:
            const std::string path;

        public:
            FolderFileIteratorItem(std::string path) : FilesIteratorItem(std::filesystem::path(path).stem().string() + std::filesystem::path(path).extension().string()), path(path) {}
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
                bool vv = filesIterator != std::filesystem::recursive_directory_iterator();
                v.reset();

                if (vv)
                {
                    std::string path = filesIterator->path();

                    if (std::filesystem::is_regular_file(path))
                        v = std::make_unique<FolderFileIteratorItem>(path, std::filesystem::path(path).stem().string() + std::filesystem::path(path).extension().string());

                    filesIterator.increment(iteratorError);
                    if (iteratorError)
                        throw satdump_exception(iteratorError.message());
                }

                return vv;
            }

            void reset() { filesIterator = std::filesystem::recursive_directory_iterator(folder); }
        };
    } // namespace utils
} // namespace satdump