#include "image/io.h"
#include "image/j2k_utils.h"
#include "image/jpeg_utils.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "module_fy4_lrit_data_decoder.h"
#include "utils/string.h"
#include "xrit/fy4/fy4_headers.h"
#include <filesystem>
#include <fstream>

namespace fy4
{
    namespace lrit
    {
        std::string getHRITImageFilename(std::tm *timeReadable, std::string sat_name, int channel)
        {
            std::string utc_filename = sat_name + "_" + std::to_string(channel) + "_" +                                                                             // Satellite name and channel
                                       std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
            return utc_filename;
        }

        void FY4LRITDataDecoderModule::processLRITFile(satdump::xrit::XRITFile &file)
        {
            std::string current_filename = file.filename;

            satdump::xrit::PrimaryHeader primary_header = file.getHeader<satdump::xrit::PrimaryHeader>();

            if (file.custom_flags[IS_ENCRYPTED]) // We lack decryption
            {
                if (!std::filesystem::exists(directory + "/LRIT_ENCRYPTED"))
                    std::filesystem::create_directory(directory + "/LRIT_ENCRYPTED");

                logger->info("Writing file " + directory + "/LRIT_ENCRYPTED/" + file.filename + "...");

                // Write file out
                std::ofstream fileo(directory + "/LRIT_ENCRYPTED/" + file.filename, std::ios::binary);
                fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                fileo.close();
            }
            else
            {
                if (primary_header.file_type_code == 0 && file.hasHeader<satdump::xrit::fy4::ImageInformationRecord>())
                {
                    satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(file);

                    // Check if this is image data, and if so also write it as an image
                    if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
                    {
                        std::string processor_name = finfo.satellite_short_name;

                        if (all_processors.count(processor_name) == 0)
                        {
                            auto p = std::make_shared<satdump::xrit::XRITChannelProcessor>();
                            p->directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGES";
                            all_processors.emplace(processor_name, p);
                        }

                        all_processors[processor_name]->push(finfo, file);
                    }
                    else
                    {
                        if (!std::filesystem::exists(directory + "/LRIT"))
                            std::filesystem::create_directory(directory + "/LRIT");

                        logger->info("Writing file " + directory + "/LRIT/" + file.filename + "...");

                        // Write file out
                        std::ofstream fileo(directory + "/LRIT/" + file.filename, std::ios::binary);
                        fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                        fileo.close();
                    }
                }
                else
                {
                    if (!std::filesystem::exists(directory + "/LRIT"))
                        std::filesystem::create_directory(directory + "/LRIT");

                    logger->info("Writing file " + directory + "/LRIT/" + file.filename + "...");

                    // Write file out
                    std::ofstream fileo(directory + "/LRIT/" + file.filename, std::ios::binary);
                    fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
                    fileo.close();
                }
            }
        }
    } // namespace lrit
} // namespace fy4