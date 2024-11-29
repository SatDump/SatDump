#include "wav.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <inttypes.h>
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
        char month_str[4];
        double freq_mhz;
        uint64_t freq;
        uint64_t samp;
        std::tm timeS;
        memset(&timeS, 0, sizeof(std::tm));

        if (audio)
        {
            // SDR++ Audio filename
            if (sscanf(filename.c_str(),
                       "audio_%" PRIu64 "Hz_%d-%d-%d_%d-%d-%d",
                       &freq,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &timeS.tm_mday, &timeS.tm_mon, &timeS.tm_year) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timeS.tm_isdst = -1;
                md.frequency = freq;
                md.timestamp = mktime(&timeS);
            }

            // HDSDR Audio filename
            if (sscanf(filename.c_str(),
                       "HDSDR_%4d%2d%2d_%2d%2d%2dZ_%" PRIu64 "kHz_AF",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq * 1e3;
                md.timestamp = timegm(&timeS);
            }

            // SDRSharp Audio filename
            if (sscanf(filename.c_str(),
                       "SDRSharp_%4d%2d%2d_%2d%2d%2dZ_%" PRIu64 "Hz_AF",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = timegm(&timeS);
            }

            // SDRUno Audio filename
            if (sscanf(filename.c_str(),
                       "SDRUno_%4d%2d%2d_%2d%2d%2d_%" PRIu64 "Hz",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timeS.tm_isdst = -1;
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
                md.timestamp = timegm(&timeS);
            }

            // GQRX Audio filename (UTC)
            if (sscanf(filename.c_str(),
                       "gqrx_%4d%2d%2d_%2d%2d%2d_%" PRIu64,
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &freq) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = timegm(&timeS);
            }

            // Simple UTC timestamp (WXtoImg)
            if (sscanf(filename.c_str(),
                       "%4d%2d%2d%2d%2d%2d",
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 6)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.timestamp = timegm(&timeS);
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
                timeS.tm_isdst = -1;
                md.timestamp = mktime(&timeS);
            }

            // Other filename (NOAA-APT-v2?)
            // satnum = 0;
            if (sscanf(filename.c_str(),
                       "NOAA%2d-%4d%2d%2d-%2d%2d%2d",
                       &satnum,
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timeS.tm_isdst = -1;
                md.timestamp = mktime(&timeS);
            }

            // Other filename (NOAA-APT-v2?)
            // satnum = 0;
            if (sscanf(filename.c_str(),
                       "NOAA-%2d-%4d%2d%2d-%2d%2d%2d",
                       &satnum,
                       &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timeS.tm_isdst = -1;
                md.timestamp = mktime(&timeS);
            }
        }
        else
        {
            // SDR++ Baseband filename
            if (sscanf(filename.c_str(),
                       "baseband_%" PRIu64 "Hz_%d-%d-%d_%d-%d-%d",
                       &freq,
                       &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                       &timeS.tm_mday, &timeS.tm_mon, &timeS.tm_year) == 7)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                timeS.tm_isdst = -1;
                md.frequency = freq;
                md.timestamp = mktime(&timeS);
            }

            // SDR Console File names
            else if (sscanf(filename.c_str(),
                            "%d-%3s-%d %2d%2d%2d.%*d %lfMHz",
                            &timeS.tm_mday, month_str, &timeS.tm_year,
                            &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                            &freq_mhz) == 7)
            {
                month_str[3] = '\0';
                if (strcmp(month_str, "Jan") == 0)
                    timeS.tm_mon = 0;
                else if (strcmp(month_str, "Feb") == 0)
                    timeS.tm_mon = 1;
                else if (strcmp(month_str, "Mar") == 0)
                    timeS.tm_mon = 2;
                else if (strcmp(month_str, "Apr") == 0)
                    timeS.tm_mon = 3;
                else if (strcmp(month_str, "May") == 0)
                    timeS.tm_mon = 4;
                else if (strcmp(month_str, "Jun") == 0)
                    timeS.tm_mon = 5;
                else if (strcmp(month_str, "Jul") == 0)
                    timeS.tm_mon = 6;
                else if (strcmp(month_str, "Aug") == 0)
                    timeS.tm_mon = 7;
                else if (strcmp(month_str, "Sep") == 0)
                    timeS.tm_mon = 8;
                else if (strcmp(month_str, "Oct") == 0)
                    timeS.tm_mon = 9;
                else if (strcmp(month_str, "Nov") == 0)
                    timeS.tm_mon = 10;
                else if (strcmp(month_str, "Dec") == 0)
                    timeS.tm_mon = 11;

                timeS.tm_year -= 1900;
                timeS.tm_isdst = -1;
                md.frequency = freq_mhz * 1e6;
                md.timestamp = mktime(&timeS);
            }

            // SatDump Baseband
            else if (sscanf(filename.c_str(),
                            "%d-%d-%d_%d-%d-%d_%" PRIu64 "SPS_%" PRIu64 "Hz",
                            &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                            &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                            &samp, &freq) == 8)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = timegm(&timeS);

                std::string ext = std::filesystem::path(filepath).extension().string();
                if (ext == ".cf32" || ext == ".cs32" || ext == ".cs16" || ext == ".cs8")
                    md.baseband_format = ext.substr(1);
                else if (ext == ".f32" || ext == ".s32" || ext == ".s16" || ext == ".s8") // Later, this should input REAL and not complex!
                    md.baseband_format = "c" + ext.substr(1);
                if (ext != ".wav")
                    md.samplerate = samp;
            }

            // SatDump Baseband with milliseconds
            else if (sscanf(filename.c_str(),
                            "%d-%d-%d_%d-%d-%d-%*d_%" PRIu64 "SPS_%" PRIu64 "Hz",
                            &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                            &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec,
                            &samp, &freq) == 8)
            {
                timeS.tm_year -= 1900;
                timeS.tm_mon -= 1;
                md.frequency = freq;
                md.timestamp = timegm(&timeS);

                std::string ext = std::filesystem::path(filepath).extension().string();
                if (ext == ".cf32" || ext == ".cs16" || ext == ".cs8")
                    md.baseband_format = ext.substr(1);
                else if (ext == ".f32" || ext == ".s16" || ext == ".s8") // Later, this should input REAL and not complex!
                    md.baseband_format = "c" + ext.substr(1);
                if (ext != ".wav")
                    md.samplerate = samp;
            }
        }

        return md;
    }
};
