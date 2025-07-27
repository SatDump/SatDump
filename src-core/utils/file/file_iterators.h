#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace satdump
{
    namespace utils
    {
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
            virtual bool getNext(std::shared_ptr<FilesIteratorItem> &v) = 0;
            virtual void reset() = 0;
        };
    } // namespace utils
} // namespace satdump