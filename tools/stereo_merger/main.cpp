#include "logger.h"

#include <fstream>
#include <vector>
#include <array>
#include <algorithm>

double get_cadu_timestamp(uint8_t *data)
{
    uint8_t *tttt = &data[4 + 7];
    double timestamp = (tttt[0] << 24 |
                        tttt[1] << 16 |
                        tttt[2] << 8 |
                        tttt[3]) +
                       tttt[4] / 256.0 +
                       1161734400 + 43200;
    return timestamp;
}

int main(int argc, char *argv[])
{
    initLogger();
    completeLoggerInit();

    std::vector<double> all_timestamps;
    std::vector<std::array<uint8_t, 1119>> all_frames;

    for (int i = 0; i < argc - 2; i++)
    {
        logger->info("Loading %s", argv[1 + i]);
        std::ifstream data_in_frames_1(argv[1 + i], std::ios::binary);
        std::array<uint8_t, 1119> cadu;
        while (!data_in_frames_1.eof())
        {
            data_in_frames_1.read((char *)&cadu[0], 1119);

            double tt1 = get_cadu_timestamp(&cadu[0]);

            if (std::find(all_timestamps.begin(), all_timestamps.end(), tt1) == all_timestamps.end())
            {
                all_timestamps.push_back(tt1);
                all_frames.push_back(cadu);
            }
        }
    }

    logger->info("Sorting...");
    std::sort(all_frames.begin(), all_frames.end(), [](const std::array<uint8_t, 1119> &v1, const std::array<uint8_t, 1119> &v2)
              { return get_cadu_timestamp((uint8_t *)&v1[0]) < get_cadu_timestamp((uint8_t *)&v2[0]); });

    logger->info("Saving to %s", argv[argc - 1]);

    std::ofstream data_out(argv[argc - 1], std::ios::binary);

    for (auto &f : all_frames)
        data_out.write((char *)&f[0], 1119);
}
