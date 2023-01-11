#include "wav.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include "common/utils.h"

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

    FileMetadata tryParseFilenameMetadata(std::string filepath, bool audio)
    {
        FileMetadata md;

        std::string filename = std::filesystem::path(filepath).stem().string();

        uint64_t freq;
        std::tm timeS;
        memset(&timeS, 0, sizeof(std::tm));

        if (audio)
        {
            // SDR++ Audio filename
            if (sscanf(filename.c_str(),
                       "audio_%luHz_%d-%d-%d_%d-%d-%d",
                       &freq,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &timeS.tm_mday, &timeS.tm_mon, &timeS.tm_year) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = mktime(&timeS);
            }

            // HDSDR Audio filename
            if (sscanf(filename.c_str(),
                       "%4d%2d%2d_%2d%2d%2dZ_%lukHz_AF",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq * 1e3;
                md.timestamp = mktime_utc(&timeS);
            }

            // SDRSharp Audio filename
            if (sscanf(filename.c_str(),
                       "SDRSharp_%4d%2d%2d_%2d%2d%2dZ_%luHz_AF",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = mktime_utc(&timeS);
            }

            // SDRUno Audio filename
            if (sscanf(filename.c_str(),
                       "SDRUno_%4d%2d%2d_%2d%2d%2d_%luHz",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = mktime(&timeS);
            }

            // AltiWx Audio UTC timestamp
            if (sscanf(filename.c_str(),
                       "%4d%2d%2dT%2d%2d%2dZ",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.timestamp = mktime_utc(&timeS);
            }

            // Simple UTC timestamp (WXtoImg)
            if (sscanf(filename.c_str(),
                       "%4d%2d%2d%2d%2d%2d",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.timestamp = mktime_utc(&timeS);
            }

            // Other filename (NOAA-APT-v2?)
            int satnum = 0;
            if (sscanf(filename.c_str(),
                       "NOAA%2d%4d%2d%2d-%2d%2d%2d",
                       &satnum,
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.timestamp = mktime(&timeS);
            }
        }
        else
        {
            // SDR++ Baseband filename
            if (sscanf(filename.c_str(),
                       "baseband_%luHz_%d-%d-%d_%d-%d-%d",
                       &freq,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &timeS.tm_mday, &timeS.tm_mon, &timeS.tm_year) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = mktime(&timeS);
            }
        }

        return md;
    }
};