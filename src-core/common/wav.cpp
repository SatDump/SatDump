#include "wav.h"
#include <fstream>
#include <filesystem>
#include <cstring>

namespace wav
{
    WavHeader parseHeaderFromFileWav(std::string file)
    {
        WavHeader header;
        memset(&header, 0, sizeof(header));

        if (std::filesystem::exists(file))
        {
            std::ifstream file_stream(file, std::ios::binary);
            file_stream.read((char *)&header, sizeof(header));
            file_stream.close();
        }

        return header;
    }

    RF64Header parseHeaderFromFileRF64(std::string file)
    {
        RF64Header header;
        memset(&header, 0, sizeof(header));

        if (std::filesystem::exists(file))
        {
            std::ifstream file_stream(file, std::ios::binary);
            file_stream.read((char *)&header, sizeof(header));
            file_stream.close();
        }

        return header;
    }

    bool isValidWav(WavHeader header)
    {
        return std::string(&header.RIFF[0], &header.RIFF[4]) == "RIFF";
    }

    bool isValidRF64(WavHeader header)
    {
        return std::string(&header.RIFF[0], &header.RIFF[4]) == "RF64";
    }
};