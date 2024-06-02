#include "fazzt_processor.h"
#include <cstring>
#include <ctime>
#include <algorithm>
// #include "logger.h"

namespace fazzt
{
    std::vector<FazztFile> FazztProcessor::work(std::vector<uint8_t> fazzt_frame)
    {
        finished_files.clear();

        if (fazzt_frame.size() < 8)
            return finished_files;

        // Parse header
        uint8_t pkt_type = fazzt_frame[1];
        uint16_t pkt_length = fazzt_frame[3] << 8 | fazzt_frame[2];
        uint32_t id = fazzt_frame[7] << 24 | fazzt_frame[6] << 16 | fazzt_frame[5] << 8 | fazzt_frame[4];

        bool file_exists = files_in_progress.count(id) > 0;

        if (pkt_length <= fazzt_frame.size())
        {
            if (pkt_type == 0x03) // Head
            {
                if (fazzt_frame.size() < 85)
                    return finished_files;

                fazzt_frame.resize(1431);

                std::string filename((char *)&fazzt_frame[84]);
                int newPos_Path = 84 + filename.size() + 72;
                int newPos_Size = 84 + filename.size() + 56;
                std::string path((char *)&fazzt_frame[newPos_Path]);
                uint16_t parts = fazzt_frame[73] << 8 | fazzt_frame[72];
                uint32_t length = fazzt_frame[newPos_Size + 3] << 24 | fazzt_frame[newPos_Size + 2] << 16 | fazzt_frame[newPos_Size + 1] << 8 | fazzt_frame[newPos_Size + 0];

                // logger->warn("New file ID %lld NAME %s, PATH %s, PARTS %d, LENGTH %d", id, filename.c_str(), path.c_str(), parts, length);

                if (length <= (uint32_t)MAX_SIZE &&
                    filename.size() > 4 &&
                    size_t(parts) * size_t(PAYLOAD_SIZE) >= length) // Ensure we don't generate massive files
                {
                    FazztFile file;
                    file.data.resize(parts * PAYLOAD_SIZE); // Be safe
                    file.size = length;
                    file.name = filename;
                    file.parts = parts;
                    file.has_parts = std::vector<bool>(parts, false);
                    file.last_pkt_time = time(0);

                    if (files_in_progress.count(id) > 0)
                    {
                        files_in_progress[id].size = length;
                        files_in_progress[id].parts = parts;
                        files_in_progress[id].name = filename;
                    }
                    else
                        files_in_progress.insert({id, file});
                }
            }
            else if (pkt_type == 0x01 && file_exists) // Body
            {
                uint16_t part = fazzt_frame[9] << 8 | fazzt_frame[8];
                if (part < files_in_progress[id].parts)
                {
                    memcpy(&files_in_progress[id].data[part * PAYLOAD_SIZE], &fazzt_frame[16], std::min<int>(PAYLOAD_SIZE, fazzt_frame.size() - 16));
                    files_in_progress[id].has_parts[part] = true;
                }
            }
            else if (pkt_type == 0xFF && file_exists) // Tail
            {
                if (files_in_progress[id].data.size() > 0 && files_in_progress[id].size > 0)
                {
                    files_in_progress[id].data.resize(files_in_progress[id].size);
                    finished_files.push_back(files_in_progress[id]); // Push the file out
                    files_in_progress.erase(id);
                    // logger->warn("End file ID %lld", id);
                }
            }
        }

        if (frame_cnt++ % 1000)
        {
            time_t ctime = time(0);
        recheck:
            for (auto &v : files_in_progress)
            {
                if (ctime - v.second.last_pkt_time > MAX_TIME)
                {
                    files_in_progress.erase(v.first);
                    goto recheck;
                }
            }
        }

        return finished_files;
    }
};