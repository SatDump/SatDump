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

        void GOESLRITDataDecoderModule::saveImageP(GOESxRITProductMeta meta, image::Image<uint8_t> &img)
        {
            if (meta.is_goesn)
                img.resize(img.width(), img.height() * 1.75);

            if (meta.channel == -1 || meta.satellite_name == "" || meta.satellite_short_name == "" || meta.scan_time == 0)
            {
                std::string ext;
                img.append_ext(&ext);
                if (std::filesystem::exists(directory + "/IMAGES/Unknown/" + meta.filename + ext))
                {
                    int current_iteration = 1;
                    std::string filename_new;
                    do
                    {
                        filename_new = meta.filename + "_" + std::to_string(current_iteration++);
                    } while (std::filesystem::exists(directory + "/IMAGES/Unknown/" + filename_new + ext));
                    img.save_img(directory + "/IMAGES/Unknown/" + filename_new);
                    logger->warn("Image already existed. Written as %s", filename_new.c_str());
                }
                else
                    img.save_img(directory + "/IMAGES/Unknown/" + meta.filename);
            }
            else
            {
                if (meta.satellite_name == "Himawari")
                    productizer.setInstrumentID("ahi");
                else if (meta.is_goesn)
                    productizer.setInstrumentID("goesn_imager");
                productizer.saveImage(img, directory + "/IMAGES", meta.satellite_name, meta.satellite_short_name, std::to_string(meta.channel), meta.scan_time, meta.region, meta.image_navigation_record.get(), meta.image_data_function_record.get());
                if (meta.satellite_name == "Himawari" || meta.is_goesn)
                    productizer.setInstrumentID("abi");
            }
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

                GOESxRITProductMeta lmeta;
                lmeta.filename = file.filename;

                // Try to parse navigation
                if (file.hasHeader<::lrit::ImageNavigationRecord>())
                    lmeta.image_navigation_record = std::make_shared<::lrit::ImageNavigationRecord>(file.getHeader<::lrit::ImageNavigationRecord>());

                // Try to parse calibration
                if (file.hasHeader<::lrit::ImageDataFunctionRecord>())
                    lmeta.image_data_function_record = std::make_shared<::lrit::ImageDataFunctionRecord>(file.getHeader<::lrit::ImageDataFunctionRecord>());

                // Process as a specific dataset
                {
                    // GOES-R Data, from GOES-16 to 19.
                    // Once again peeked in goestools for the meso detection, sorry :-)
                    if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 ||
                                                               noaa_header.product_id == 17 ||
                                                               noaa_header.product_id == 18 ||
                                                               noaa_header.product_id == 19))
                    {
                        lmeta.satellite_name = "GOES-" + std::to_string(noaa_header.product_id);
                        lmeta.satellite_short_name = "G" + std::to_string(noaa_header.product_id);
                        std::vector<std::string> cutFilename = splitString(current_filename, '-');

                        if (cutFilename.size() >= 4)
                        {
                            int mode = -1;

                            if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &lmeta.channel) == 2)
                            {
                                AncillaryTextRecord ancillary_record = file.getHeader<AncillaryTextRecord>();

                                // Parse Region
                                if (ancillary_record.meta.count("Region") > 0)
                                {
                                    std::string regionName = ancillary_record.meta["Region"];

                                    if (regionName == "Full Disk")
                                    {
                                        lmeta.region = "Full Disk";
                                    }
                                    else if (regionName == "Mesoscale")
                                    {
                                        if (cutFilename[2] == "CMIPM1")
                                            lmeta.region = "Mesoscale 1";
                                        else if (cutFilename[2] == "CMIPM2")
                                            lmeta.region = "Mesoscale 2";
                                        else
                                            lmeta.region = "Mesoscale";
                                    }
                                }

                                // Parse scan time
                                std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                                if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                                {
                                    std::string scanTime = ancillary_record.meta["Time of frame start"];
                                    strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                                    lmeta.scan_time = mktime_utc(&scanTimestamp);
                                }
                            }
                        }
                    }
                    // GOES-N Data, from GOES-13 to 15.
                    else if (primary_header.file_type_code == 0 && (noaa_header.product_id == 13 ||
                                                                    noaa_header.product_id == 14 ||
                                                                    noaa_header.product_id == 15))
                    {
                        lmeta.satellite_name = "GOES-" + std::to_string(noaa_header.product_id);
                        lmeta.satellite_short_name = "G" + std::to_string(noaa_header.product_id);
                        lmeta.is_goesn = true;

                        // Parse channel
                        if (noaa_header.product_subid <= 10)
                            lmeta.channel = 4;
                        else if (noaa_header.product_subid <= 20)
                            lmeta.channel = 1;
                        else if (noaa_header.product_subid <= 30)
                            lmeta.channel = 3;

                        // Parse Region. Had to peak in goestools again...
                        if (noaa_header.product_subid % 10 == 1)
                            lmeta.region = "Full Disk";
                        else if (noaa_header.product_subid % 10 == 2)
                            lmeta.region = "Northern Hemisphere";
                        else if (noaa_header.product_subid % 10 == 3)
                            lmeta.region = "Southern Hemisphere";
                        else if (noaa_header.product_subid % 10 == 4)
                            lmeta.region = "United States";
                        else
                        {
                            char buf[32];
                            size_t len;
                            int num = (noaa_header.product_subid % 10) - 5;
                            len = snprintf(buf, 32, "Special Interest %d", num);
                            lmeta.region = std::string(buf, len);
                        }

                        // Parse scan time
                        AncillaryTextRecord ancillary_record = file.getHeader<AncillaryTextRecord>();
                        std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                        if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                        {
                            std::string scanTime = ancillary_record.meta["Time of frame start"];
                            strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                            lmeta.scan_time = mktime_utc(&scanTimestamp);
                        }
                    }
                    // Himawari-8 rebroadcast
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == ID_HIMAWARI)
                    {
                        lmeta.satellite_name = "Himawari";
                        lmeta.satellite_short_name = "HIM";
                        lmeta.channel = noaa_header.product_subid;
                        lmeta.region = "";

                        // Translate to real numbers
                        if (lmeta.channel == 3)
                            lmeta.channel = 13;
                        else if (lmeta.channel == 7)
                            lmeta.channel = 8;
                        else if (lmeta.channel == 1)
                            lmeta.channel = 3;

                        // Apparently the timestamp is in there for Himawari-8 data
                        AnnotationRecord annotation_record = file.getHeader<AnnotationRecord>();

                        std::vector<std::string> strParts = splitString(annotation_record.annotation_text, '_');
                        if (strParts.size() > 3)
                        {
                            strptime(strParts[2].c_str(), "%Y%m%d%H%M", timeReadable);
                            lmeta.scan_time = mktime_utc(timeReadable);
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

                    if (lmeta.image_navigation_record)
                        if (noaa_header.product_id != ID_HIMAWARI)
                            lmeta.image_navigation_record->line_offset = lmeta.image_navigation_record->line_offset + (segment_id_header.segment_sequence_number) * image_structure_record.lines_count;

                    uint16_t image_identifier = segment_id_header.image_identifier;
                    if (noaa_header.product_id == ID_HIMAWARI) // Image IDs are invalid for Himawari; make one up
                        image_identifier = lmeta.scan_time % 10000 + lmeta.channel;

                    if (segmentedDecoder.image_id != image_identifier)
                    {
                        if (segmentedDecoder.image_id != -1)
                        {
                            wip_img->imageStatus = SAVING;
                            saveImageP(segmentedDecoder.meta, *segmentedDecoder.image);
                            wip_img->imageStatus = RECEIVING;
                        }

                        segmentedDecoder = SegmentedLRITImageDecoder(segment_id_header.max_segment,
                                                                     segment_id_header.max_column,
                                                                     segment_id_header.max_row,
                                                                     image_identifier);
                        segmentedDecoder.meta = lmeta;
                    }

                    if (noaa_header.product_id == ID_HIMAWARI)
                        segmentedDecoder.pushSegment(&file.lrit_data[primary_header.total_header_length],
                                                     file.lrit_data.size() - primary_header.total_header_length, segment_id_header.segment_sequence_number - 1);
                    else
                        segmentedDecoder.pushSegment(&file.lrit_data[primary_header.total_header_length],
                                                     file.lrit_data.size() - primary_header.total_header_length, segment_id_header.segment_sequence_number);

                    // If the UI is active, update texture
                    if (wip_img->textureID > 0)
                    {
                        // Downscale image
                        wip_img->img_height = 1000;
                        wip_img->img_width = 1000;
                        image::Image<uint8_t> imageScaled = *(segmentedDecoder.image);
                        imageScaled.resize(wip_img->img_width, wip_img->img_height);
                        uchar_to_rgba(imageScaled.data(), wip_img->textureBuffer, wip_img->img_height * wip_img->img_width, 1);
                        wip_img->hasToUpdate = true;
                    }

                    if (segmentedDecoder.isComplete())
                    {
                        wip_img->imageStatus = SAVING;
                        saveImageP(segmentedDecoder.meta, *segmentedDecoder.image);
                        segmentedDecoder = SegmentedLRITImageDecoder();
                        wip_img->imageStatus = IDLE;
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
                        image::Image<uint8_t> image(&file.lrit_data[primary_header.total_header_length], image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        saveImageP(lmeta, image);
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
