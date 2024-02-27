#include "module_goes_lrit_data_decoder.h"
#include "lrit_header.h"
#include "logger.h"
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "imgui/imgui_image.h"
#include <filesystem>

#ifdef _MSC_VER
#define timegm _mkgmtime
#endif

namespace goes
{
    namespace hrit
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

        void GOESLRITDataDecoderModule::saveLRITFile(::lrit::LRITFile &file, std::string path)
        {
            if (!std::filesystem::exists(path))
                std::filesystem::create_directory(path);

            //If segmented, rename file after segment name
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

        void GOESLRITDataDecoderModule::processLRITFile(::lrit::LRITFile &file)
        {
            std::string current_filename = file.filename;

            ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();
            NOAALRITHeader noaa_header = file.getHeader<NOAALRITHeader>();

            // Handle LRIT files with no data
            if (primary_header.total_header_length == file.lrit_data.size())
            {
                logger->warn("Received LRIT header with no body! Saving as .lrit");
                saveLRITFile(file, directory + "/LRIT");
            }

            // Check if this is image data, and if so also write it as an image
            else if (write_images && primary_header.file_type_code == 0 && file.hasHeader<::lrit::ImageStructureRecord>())
            {
                if (!std::filesystem::exists(directory + "/IMAGES"))
                    std::filesystem::create_directory(directory + "/IMAGES");

                ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                ::lrit::TimeStampRecord timestamp_record = file.getHeader<::lrit::TimeStampRecord>();
                std::tm *timeReadable = gmtime(&timestamp_record.timestamp);

                std::string old_filename = current_filename;

                bool is_goesn = false;

                // Process as a specific dataset
                {
                    // GOES-R Data, from GOES-16 to 19.
                    // Once again peeked in goestools for the meso detection, sorry :-)
                    if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                               noaa_header.product_id == 17 ||
                                                               noaa_header.product_id == 18 ||
                                                               noaa_header.product_id == 19))
                    {
                        std::vector<std::string> cutFilename = splitString(current_filename, '-');

                        if (cutFilename.size() >= 4)
                        {
                            int mode = -1;
                            int channel = -1;

                            if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                            {
                                AncillaryTextRecord ancillary_record = file.getHeader<AncillaryTextRecord>();

                                std::string region = "Others";

                                // Parse Region
                                if (ancillary_record.meta.count("Region") > 0)
                                {
                                    std::string regionName = ancillary_record.meta["Region"];

                                    if (regionName == "Full Disk")
                                    {
                                        region = "Full Disk";
                                    }
                                    else if (regionName == "Mesoscale")
                                    {
                                        if (cutFilename[2] == "CMIPM1")
                                            region = "Mesoscale 1";
                                        else if (cutFilename[2] == "CMIPM2")
                                            region = "Mesoscale 2";
                                        else
                                            region = "Mesoscale";
                                    }
                                }

                                // Parse scan time
                                std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                                if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                                {
                                    std::string scanTime = ancillary_record.meta["Time of frame start"];
                                    strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                                }

                                std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/" + region;

                                if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                                    std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                                current_filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), channel);

                                //Configure mesoscale color compoer - FD is defined further down
                                if ((region == "Mesoscale 1" || region == "Mesoscale 2"))
                                {
                                    GOESRFalseColorComposer *goes_r_fc_composer;

                                    if (region == "Mesoscale 1")
                                        goes_r_fc_composer = &goes_r_fc_composer_meso1;
                                    else // Meso 2
                                        goes_r_fc_composer = &goes_r_fc_composer_meso2;

                                    std::shared_ptr<image::Image<uint8_t>> image = std::make_shared<image::Image<uint8_t>> (&file.lrit_data[primary_header.total_header_length],
                                                                                                                            image_structure_record.columns_count,
                                                                                                                            image_structure_record.lines_count, 1);

                                    if (channel == 2)
                                    {
                                        goes_r_fc_composer->push2(image, timegm(&scanTimestamp));
                                    }
                                    else if (channel == 13)
                                    {
                                        goes_r_fc_composer->push13(image, timegm(&scanTimestamp));
                                    }

                                    goes_r_fc_composer->filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), "FC");
                                }
                            }
                        }
                    }
                    // GOES-N Data, from GOES-13 to 15.
                    else if (primary_header.file_type_code == 0 && (noaa_header.product_id == 13 ||
                                                                    noaa_header.product_id == 14 ||
                                                                    noaa_header.product_id == 15))
                    {
                        is_goesn = true;

                        int channel = -1;

                        // Parse channel
                        if (noaa_header.product_subid <= 10)
                            channel = 4;
                        else if (noaa_header.product_subid <= 20)
                            channel = 1;
                        else if (noaa_header.product_subid <= 30)
                            channel = 3;

                        std::string region = "Others";

                        // Parse Region. Had to peak in goestools again...
                        if (noaa_header.product_subid % 10 == 1)
                            region = "Full Disk";
                        else if (noaa_header.product_subid % 10 == 2)
                            region = "Northern Hemisphere";
                        else if (noaa_header.product_subid % 10 == 3)
                            region = "Southern Hemisphere";
                        else if (noaa_header.product_subid % 10 == 4)
                            region = "United States";
                        else
                        {
                            char buf[32];
                            size_t len;
                            int num = (noaa_header.product_subid % 10) - 5;
                            len = snprintf(buf, 32, "Special Interest %d", num);
                            region = std::string(buf, len);
                        }

                        // Parse scan time
                        AncillaryTextRecord ancillary_record = file.getHeader<AncillaryTextRecord>();
                        std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                        if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                        {
                            std::string scanTime = ancillary_record.meta["Time of frame start"];
                            strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                        }

                        std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/" + region;

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        current_filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), channel);
                    }
                    // Himawari-8 rebroadcast
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == 43)
                    {
                        std::string subdir = "Himawari-8/Full Disk";

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        // Apparently the timestamp is in there for Himawari-8 data
                        AnnotationRecord annotation_record = file.getHeader<AnnotationRecord>();

                        std::vector<std::string> strParts = splitString(annotation_record.annotation_text, '_');
                        if (strParts.size() > 3)
                        {
                            strptime(strParts[2].c_str(), "%Y%m%d%H%M", timeReadable);
                            current_filename = subdir + "/" + getHRITImageFilename(timeReadable, "HIM", noaa_header.product_subid); // SubID = Channel
                        }
                    }
                    // NWS Images
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == 6)
                    {
                        std::string subdir = "NWS";

                        if (!std::filesystem::exists(directory + "/IMAGES/" + subdir))
                            std::filesystem::create_directories(directory + "/IMAGES/" + subdir);

                        std::string back = current_filename;
                        current_filename = subdir + "/" + back;
                    }
                }

                if (file.hasHeader<SegmentIdentificationHeader>())
                {
                    SegmentIdentificationHeader segment_id_header = file.getHeader<SegmentIdentificationHeader>();

                    if (all_wip_images.count(file.vcid) == 0)
                        all_wip_images.insert({file.vcid, std::make_unique<wip_images>()});

                    std::unique_ptr<wip_images> &wip_img = all_wip_images[file.vcid];

                    wip_img->imageStatus = RECEIVING;

                    if (segmentedDecoders.count(file.vcid) == 0)
                        segmentedDecoders.insert({file.vcid, SegmentedLRITImageDecoder()});

                    SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[file.vcid];

                    if (segmentedDecoder.image_id != segment_id_header.image_identifier)
                    {
                        if (segmentedDecoder.image_id != -1)
                        {
                            wip_img->imageStatus = SAVING;
                            if (is_goesn)
                                segmentedDecoder.image->resize(segmentedDecoder.image->width(), segmentedDecoder.image->height() * 1.75);
                            segmentedDecoder.image->save_img(std::string(directory + "/IMAGES/" + segmentedDecoder.filename).c_str());
                            wip_img->imageStatus = RECEIVING;
                        }

                        segmentedDecoder = SegmentedLRITImageDecoder(segment_id_header.max_segment,
                                                                     segment_id_header.max_column,
                                                                     segment_id_header.max_row,
                                                                     segment_id_header.image_identifier);
                        segmentedDecoder.filename = current_filename;
                    }

                    if (noaa_header.product_id == ID_HIMAWARI)
                        segmentedDecoder.pushSegment(&file.lrit_data[primary_header.total_header_length],
                            file.lrit_data.size() - primary_header.total_header_length, segment_id_header.segment_sequence_number - 1);
                    else
                        segmentedDecoder.pushSegment(&file.lrit_data[primary_header.total_header_length],
                            file.lrit_data.size() - primary_header.total_header_length, segment_id_header.segment_sequence_number);

                    // Check if this is GOES-R, if yes, this is Full Disk
                    if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                               noaa_header.product_id == 17 ||
                                                               noaa_header.product_id == 18 ||
                                                               noaa_header.product_id == 19))
                    {
                        int mode = -1;
                        int channel = -1;
                        std::vector<std::string> cutFilename = splitString(old_filename, '-');
                        if (cutFilename.size() > 3)
                        {
                            if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                            {
                                AncillaryTextRecord ancillary_record = file.getHeader<AncillaryTextRecord>();

                                // Parse scan time
                                std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                                if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                                {
                                    std::string scanTime = ancillary_record.meta["Time of frame start"];
                                    strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                                }

                                if (channel == 2)
                                {
                                    goes_r_fc_composer_full_disk.push2(segmentedDecoder.image, timegm(&scanTimestamp));
                                    std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/Full Disk";
                                    goes_r_fc_composer_full_disk.filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), "FC");
                                }

                                else if (channel == 13 && file.vcid == 13) //Redundant check keeps relayed channel 13 from entering the color composer
                                {
                                    goes_r_fc_composer_full_disk.push13(segmentedDecoder.image, timegm(&scanTimestamp));
                                    std::string subdir = "GOES-" + std::to_string(noaa_header.product_id) + "/Full Disk";
                                    goes_r_fc_composer_full_disk.filename = subdir + "/" + getHRITImageFilename(&scanTimestamp, "G" + std::to_string(noaa_header.product_id), "FC");
                                }


                            }
                        }
                    }

                    // If the UI is active, update texture
                    if (wip_img->textureID > 0)
                    {
                        // Downscale image
                        wip_img->img_height = 1000;
                        wip_img->img_width = 1000;
                        image::Image<uint8_t> imageScaled = *(segmentedDecoder.image);
                        imageScaled.resize(wip_img->img_width, wip_img->img_height);
                        uchar_to_rgba(imageScaled.data(), wip_img->textureBuffer, wip_img->img_height * wip_img->img_width);
                        wip_img->hasToUpdate = true;
                    }

                    if (segmentedDecoder.isComplete())
                    {
                        wip_img->imageStatus = SAVING;
                        if (is_goesn)
                            segmentedDecoder.image->resize(segmentedDecoder.image->width(), segmentedDecoder.image->height() * 1.75);
                        segmentedDecoder.image->save_img(std::string(directory + "/IMAGES/" + current_filename).c_str());
                        segmentedDecoder = SegmentedLRITImageDecoder();
                        wip_img->imageStatus = IDLE;

                        // Check if this is GOES-R
                        if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                                   noaa_header.product_id == 17 ||
                                                                   noaa_header.product_id == 18 ||
                                                                   noaa_header.product_id == 19))
                        {
                            int mode = -1;
                            int channel = -1;
                            std::vector<std::string> cutFilename = splitString(old_filename, '-');
                            if (cutFilename.size() > 3)
                            {
                                if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel) == 2)
                                {
                                    if (channel == 2)
                                        goes_r_fc_composer_full_disk.save();
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (noaa_header.noaa_specific_compression == 5) // Gif?
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
                        //Sometimes, multiple different images can be sent down with the same name
                        //Do not overwrite files
                        std::string suffix = "";
                        std::string extension = "";
                        int suffixInt = 1;

                        image::Image<uint8_t> image(&file.lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        image.append_ext(&extension);

                        while(std::filesystem::exists(directory + "/IMAGES/" + current_filename + suffix + extension))
                        {
                            suffixInt++;
                            suffix = "-" + std::to_string(suffixInt);
                        }

                        if (is_goesn)
                            image.resize(image.width(), image.height() * 1.75);
                        image.save_img(std::string(directory + "/IMAGES/" + current_filename + suffix).c_str());

                        // Check if this is GOES-R
                        if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                                   noaa_header.product_id == 17 ||
                                                                   noaa_header.product_id == 18 ||
                                                                   noaa_header.product_id == 19))
                        {
                            goes_r_fc_composer_meso1.save();
                            goes_r_fc_composer_meso2.save();
                        }
                    }
                }
            }
            // Check if this EMWIN data
            else if (write_emwin && primary_header.file_type_code == 2 && (noaa_header.product_id == 9 || noaa_header.product_id == 6))
            {
                std::string clean_filename = current_filename.substr(0, current_filename.size() - 5); // Remove extensions

                if (noaa_header.noaa_specific_compression == 0) // Uncompressed TXT
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
            else if (write_dcs && primary_header.file_type_code == 130)
                saveLRITFile(file, directory + "/DCS");
            // Otherwise, write as generic, unknown stuff. This should not happen
            // Do not write if already saving LRIT data
            else if (write_unknown && !write_lrit)
                saveLRITFile(file, directory + "/LRIT");
        }
    } // namespace hrit
} // namespace goes
