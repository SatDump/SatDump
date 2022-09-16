#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>

namespace fazzt
{
    struct FazztFile
    {
        std::string name;
        uint32_t size;
        int parts;
        std::vector<uint8_t> data;
    };

    class FazztProcessor
    {
    private:
        std::map<int, FazztFile> files_in_progress;

    public:
        std::vector<FazztFile> work(std::vector<uint8_t> fazzt_frame);
    };
};