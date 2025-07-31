#include "image/io.h"
#include "image/processing.h"
#include "imgui/imgui_image.h"
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "logger.h"
#include "lrit_header.h"
#include "module_goes_lrit_data_decoder.h"
#include "utils/string.h"
#include <filesystem>
#include <memory>

namespace goes
{
    namespace hrit
    {
        std::string getHRITImageFilename(std::tm *timeReadable, std::string sat_name, std::string channel)
        {
            std::string utc_filename = sat_name + "_" + channel + "_" +                                                                                             // Satellite name and channel
                                       std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
            return utc_filename;
        }

        const int ID_HIMAWARI = 43;

        void GOESLRITDataDecoderModule::saveLRITFile(satdump::xrit::XRITFile &file, std::string path)
        {
            if (!std::filesystem::exists(path))
                std::filesystem::create_directory(path);

            // If segmented, rename file after segment name
            std::string current_filename = file.filename;
            if (file.hasHeader<SegmentIdentificationHeader>())
            {
                SegmentIdentificationHeader segment_id_header = file.getHeader<SegmentIdentificationHeader>();
                std::ostringstream oss;
                oss << "_" << std::setfill('0') << std::setw(3) << segment_id_header.segment_sequence_number;
                current_filename.insert(current_filename.length() - 5, oss.str());
            }

            logger->info("Writing file " + path + "/" + current_filename + "...");

            // Write file out
            std::ofstream fileo(path + "/" + current_filename, std::ios::binary);
            fileo.write((char *)file.lrit_data.data(), file.lrit_data.size());
            fileo.close();
        }

        void GOESLRITDataDecoderModule::processLRITFile(satdump::xrit::XRITFile &file)
        {
            std::string current_filename = file.filename;

            satdump::xrit::PrimaryHeader primary_header = file.getHeader<satdump::xrit::PrimaryHeader>();
            NOAALRITHeader noaa_header = file.getHeader<NOAALRITHeader>();

            // Handle LRIT files with no data
            if (primary_header.total_header_length == file.lrit_data.size())
            {
                logger->warn("Received LRIT header with no body! Saving as .lrit");
                saveLRITFile(file, directory + "/LRIT");
            }

            // Check if this is image data, and if so also write it as an image
            else if (primary_header.file_type_code == 0 && file.hasHeader<satdump::xrit::ImageStructureRecord>())
            {
                if (!write_images)
                    return;

                if (!std::filesystem::exists(directory + "/IMAGES"))
                    std::filesystem::create_directory(directory + "/IMAGES");

                satdump::xrit::ImageStructureRecord image_structure_record = file.getHeader<satdump::xrit::ImageStructureRecord>();

                satdump::xrit::XRITFileInfo finfo = satdump::xrit::identifyXRITFIle(file);

                // Check if this is image data, and if so also write it as an image
                if (finfo.type != satdump::xrit::XRIT_UNKNOWN)
                {
                    std::string processor_name = finfo.satellite_short_name;

                    bool is_l2 = false;
                    if (finfo.channel.find_first_of("0123456789") == std::string::npos)
                    {
                        processor_name = processor_name + "" + finfo.channel;
                        // finfo.region = finfo.channel; TODOREWORK???
                        is_l2 = true;
                    }

                    all_processors_mtx.lock();
                    if (all_processors.count(processor_name) == 0)
                    {
                        auto p = std::make_shared<satdump::xrit::XRITChannelProcessor>();
                        if (is_l2)
                            p->directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/L2";
                        else
                            p->directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGES";
                        all_processors.emplace(processor_name, p);
                    }
                    all_processors_mtx.unlock();

                    all_processors[processor_name]->push(finfo, file);
                }
                else if (noaa_header.noaa_specific_compression == 5) // Gif?
                {
                    logger->info("Writing file " + directory + "/IMAGES/" + current_filename + ".gif" + "...");

                    int offset = primary_header.total_header_length;

                    // Write file out
                    std::ofstream fileo(directory + "/IMAGES/" + current_filename + ".gif", std::ios::binary);
                    fileo.write((char *)&file.lrit_data[offset], file.lrit_data.size() - offset);
                    fileo.close();
                }
                else // Write raw image dats
                {
                    image::Image image(&file.lrit_data[primary_header.total_header_length], 8, image_structure_record.columns_count, image_structure_record.lines_count, 1);
                    image::save_img(image, directory + "/IMAGES/" + current_filename);
                }
            }
            // Check if this EMWIN data
            else if (primary_header.file_type_code == 2 && (noaa_header.product_id == 9 || noaa_header.product_id == 6))
            {
                if (!write_emwin)
                    return;

                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions
                if (noaa_header.noaa_specific_compression == 0)                                       // Uncompressed TXT
                {
                    if (!std::filesystem::exists(directory + "/EMWIN"))
                        std::filesystem::create_directory(directory + "/EMWIN");

                    logger->info("Writing file " + directory + "/EMWIN/" + clean_filename + ".txt" + "...");

                    int offset = primary_header.total_header_length;

                    // Write file out
                    std::ofstream fileo(directory + "/EMWIN/" + clean_filename + ".txt", std::ios::binary);
                    fileo.write((char *)&file.lrit_data[offset], file.lrit_data.size() - offset);
                    fileo.close();
                }
                else if (noaa_header.noaa_specific_compression == 10) // ZIP Files
                {
                    if (!std::filesystem::exists(directory + "/EMWIN"))
                        std::filesystem::create_directory(directory + "/EMWIN");

                    int offset = primary_header.total_header_length;

                    // Init
                    mz_zip_archive zipFile;
                    MZ_CLEAR_OBJ(zipFile);
                    if (!mz_zip_reader_init_mem(&zipFile, &file.lrit_data[offset], file.lrit_data.size() - offset, MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
                    {
                        logger->error("Could not open ZIP data! Discarding...");
                        return;
                    }

                    // Read filename
                    char filenameStr[1000];
                    int chs = mz_zip_reader_get_filename(&zipFile, 0, filenameStr, 1000);
                    std::string filename(filenameStr, &filenameStr[chs - 1]);

                    // Decompress
                    size_t outSize = 0;
                    uint8_t *outBuffer = (uint8_t *)mz_zip_reader_extract_to_heap(&zipFile, 0, &outSize, 0);

                    // Write out
                    logger->info("Writing file " + directory + "/EMWIN/" + filename + "...");
                    std::ofstream file(directory + "/EMWIN/" + filename, std::ios::binary);
                    file.write((char *)outBuffer, outSize);
                    file.close();

                    // Free memory
                    zipFile.m_pFree(&zipFile, outBuffer);
                }
            }
            // Check if this is message data. If we slipped to here we know it's not EMWIN
            else if (write_messages && (primary_header.file_type_code == 1 || primary_header.file_type_code == 2))
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                if (!std::filesystem::exists(directory + "/Admin Messages"))
                    std::filesystem::create_directory(directory + "/Admin Messages");

                logger->info("Writing file " + directory + "/Admin Messages/" + clean_filename + ".txt" + "...");

                int offset = primary_header.total_header_length;

                // Write file out
                std::ofstream fileo(directory + "/Admin Messages/" + clean_filename + ".txt", std::ios::binary);
                fileo.write((char *)&file.lrit_data[offset], file.lrit_data.size() - offset);
                fileo.close();
            }
            // Check if this is DCS data
            else if (primary_header.file_type_code == 130)
            {
                int offset = primary_header.total_header_length;
                if (parse_dcs && (file.vcid != 32 || !processDCS(&file.lrit_data.data()[offset], file.lrit_data.size() - offset)))
                    saveLRITFile(file, directory + "/DCS/Unknown");
                if (write_dcs)
                    saveLRITFile(file, directory + "/DCS_LRIT");
            }
            // Otherwise, write as generic, unknown stuff. This should not happen
            // Do not write if already saving LRIT data
            else if (write_unknown && !write_lrit)
                saveLRITFile(file, directory + "/LRIT");
        }
    } // namespace hrit
} // namespace goes
