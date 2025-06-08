#include "utils.h"
#include "logger.h"
#include <cmath>
#include <sstream>

#include "common/dsp_source_sink/format_notated.h"
#include "core/config.h"

// Return filesize
uint64_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    uint64_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

std::string prepareAutomatedPipelineFolder(time_t timevalue, double frequency, std::string pipeline_name, std::string folder)
{
    std::tm *timeReadable = gmtime(&timevalue);
    if (folder == "")
    {
        folder = satdump::satdump_cfg.getValueFromSatDumpDirectories<std::string>("live_processing_path");
#ifdef __ANDROID__
        if (folder == "./live_output")
            folder = "/storage/emulated/0/live_output";
#endif
    }
    std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));
    std::string output_dir = folder + "/" + timestamp + "_" + pipeline_name + "_" + format_notated(frequency, "Hz");
    std::filesystem::create_directories(output_dir);
    logger->info("Generated folder name : " + output_dir);
    return output_dir;
}

std::string prepareBasebandFileName(double timeValue_precise, uint64_t samplerate, uint64_t frequency)
{
    const time_t timevalue = timeValue_precise;
    std::tm *timeReadable = gmtime(&timevalue);
    std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                            (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                            (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

    if (satdump::satdump_cfg.getValueFromSatDumpUI<bool>("recorder_baseband_filename_millis_precision"))
    {
        std::ostringstream ss;

        double ms_val = fmod(timeValue_precise, 1.0) * 1e3;
        ss << "-" << std::fixed << std::setprecision(0) << std::setw(3) << std::setfill('0') << ms_val;
        timestamp += ss.str();
    }

    return timestamp + "_" + std::to_string(samplerate) + "SPS_" + std::to_string(frequency) + "Hz";
}
