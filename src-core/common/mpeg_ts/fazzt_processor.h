#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <ctime>

namespace fazzt
{
    struct FazztFile
    {
        std::string name;
        uint32_t size;
        int parts;
        std::vector<bool> has_parts;
        std::vector<uint8_t> data;
        time_t last_pkt_time;
    };

    class FazztProcessor
    {
    private:
        std::map<int, FazztFile> files_in_progress;
        int frame_cnt = 0;

        std::vector<FazztFile> finished_files;

    private:
        const int PAYLOAD_SIZE;

    public:
        int MAX_SIZE = 1e9;
        int MAX_TIME = 60 * 2;

    public:
        FazztProcessor(int payload_size)
            : PAYLOAD_SIZE(payload_size)
        {
        }

        std::vector<FazztFile> work(std::vector<uint8_t> fazzt_frame);
    };
};