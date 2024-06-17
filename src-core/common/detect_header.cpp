#include "detect_header.h"
#include "common/wav.h"
#include <cstdint>
#include "wav.h"
#ifdef BUILD_ZIQ
#include "ziq.h"
#endif

#include "logger.h"

#include "ziq2.h"

HeaderInfo try_parse_header(std::string file)
{
    HeaderInfo info;

    if (wav::isValidWav(wav::parseHeaderFromFileWav(file)))
    {
        logger->debug("File is wav!");
        info.samplerate = wav::parseHeaderFromFileWav(file).samplerate;
        if (wav::parseHeaderFromFileWav(file).bits_per_sample == 8)
            info.type = "cu8";
        else if (wav::parseHeaderFromFileWav(file).bits_per_sample == 16)
            info.type = "cs16";
        info.valid = true;
    }
    else if (wav::isValidRF64(wav::parseHeaderFromFileWav(file)))
    {
        logger->debug("File is RF64!");
        info.samplerate = wav::parseHeaderFromFileRF64(file).samplerate;
        if (wav::parseHeaderFromFileRF64(file).bits_per_sample == 8)
            info.type = "cu8";
        else if (wav::parseHeaderFromFileRF64(file).bits_per_sample == 16)
            info.type = "cs16";
        info.valid = true;
    }
#ifdef BUILD_ZIQ
    else if (ziq::isValidZIQ(file))
    {
        logger->debug("File is ZIQ!");
        info.type = "ziq";
        info.valid = true;
        info.samplerate = ziq::getCfgFromFile(file).samplerate;
    }
#endif
#ifdef BUILD_ZIQ2
    else if (ziq2::ziq2_is_valid_ziq2(file))
    {
        logger->debug("File is ZIQ2!");
        info.type = "ziq2";
        info.valid = true;
        info.samplerate = ziq2::ziq2_try_parse_header(file);
    }
#endif

    return info;
}

void try_get_params_from_input_file(nlohmann::json &parameters, std::string input_file)
{
    if (std::filesystem::exists(input_file) && !std::filesystem::is_directory(input_file))
    {
        HeaderInfo hdr = try_parse_header(input_file);
        if (hdr.valid)
        {
            if (!parameters.contains("samplerate"))
                parameters["samplerate"] = hdr.samplerate;
            if (!parameters.contains("baseband_format"))
                parameters["baseband_format"] = hdr.type;
        }
        wav::FileMetadata md = wav::tryParseFilenameMetadata(input_file, false);
        if (!parameters.contains("start_timestamp") && md.timestamp != 0)
            parameters["start_timestamp"] = md.timestamp;
        if (!parameters.contains("samplerate") && md.samplerate != 0)
            parameters["samplerate"] = md.samplerate;
        if (!parameters.contains("frequency") && md.frequency != 0)
            parameters["frequency"] = md.frequency;
        if (!parameters.contains("baseband_format") && md.baseband_format != "")
            parameters["baseband_format"] = md.baseband_format;
    }
}