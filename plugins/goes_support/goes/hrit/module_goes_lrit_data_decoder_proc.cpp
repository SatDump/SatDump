#include "module_goes_lrit_data_decoder.h"
#include "lrit_header.h"
#include "logger.h"
#include "libs/miniz/miniz.h"
#include "libs/miniz/miniz_zip.h"
#include "imgui/imgui_image.h"
#include <filesystem>
#include "common/image/io.h"
#include "common/image/processing.h"

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

        void GOESLRITDataDecoderModule::saveImageP(GOESxRITProductMeta meta, image::Image &img)
        {
            if (meta.is_goesn)
                img.resize(img.width(), img.height() * 1.75);

            if (meta.channel == "" || meta.satellite_name == "" || meta.satellite_short_name == "" || meta.scan_time == 0)
            {
                std::string ext;
                image::append_ext(&ext, true);
                if (std::filesystem::exists(directory + "/IMAGES/Unknown/" + meta.filename + ext))
                {
                    int current_iteration = 1;
                    std::string filename_new;
                    do
                    {
                        filename_new = meta.filename + "_" + std::to_string(current_iteration++);
                    } while (std::filesystem::exists(directory + "/IMAGES/Unknown/" + filename_new + ext));
                    image::save_img(img, directory + "/IMAGES/Unknown/" + filename_new);
                    logger->warn("Image already existed. Written as %s", filename_new.c_str());
                }
                else
                    image::save_img(img, directory + "/IMAGES/Unknown/" + meta.filename);
            }
            else
            {
                std::string images_subdir = "/IMAGES";
                if (meta.satellite_name == "Himawari")
                    productizer.setInstrumentID("ahi");
                else if (meta.is_goesn)
                    productizer.setInstrumentID("goesn_imager");
                else if (meta.channel.find_first_of("0123456789") == std::string::npos)
                {
                    images_subdir = "/L2"; // GOES-R Level 2 (Non-CMIP)

                    // TODO: Once calibration of custom types is possible, stop doing this!
                    // RRPQE inverts its raw counts and lookup tables seemingly randomly.
                    // So far, only RRQPE seems to do this...
                    if (meta.channel == "RRQPE" && img.get(0) == 255)
                        image::linear_invert(img);
                }
                productizer.saveImage(img, 8 /*bit depth on GOES is ALWAYS 8*/, directory + images_subdir, meta.satellite_name, meta.satellite_short_name, meta.channel, meta.scan_time, meta.region, meta.image_navigation_record.get(), meta.image_data_function_record.get());
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
            else if (primary_header.file_type_code == 0 && file.hasHeader<::lrit::ImageStructureRecord>())
            {
                if (!write_images)
                    return;

                if (!std::filesystem::exists(directory + "/IMAGES"))
                    std::filesystem::create_directory(directory + "/IMAGES");

                ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();

                ::lrit::TimeStampRecord timestamp_record = file.getHeader<::lrit::TimeStampRecord>();
                std::tm *timeReadable = gmtime(&timestamp_record.timestamp);

                std::string old_filename = current_filename;
                bool used_ancillary_proj = false;
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
                            int channel_buf = -1;
                            if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel_buf) == 2 ||
                                sscanf(cutFilename[3].c_str(), "M%d_", &mode) == 1)
                            {
                                if (channel_buf == -1)
                                {
                                    lmeta.channel = cutFilename[2];
                                    if (lmeta.channel.back() == 'F' || lmeta.channel.back() == 'C')
                                        lmeta.channel.pop_back();
                                    else if (lmeta.channel.find_last_of('M') != std::string::npos)
                                        lmeta.channel = lmeta.channel.substr(0, lmeta.channel.find_last_of('M'));
                                }
                                else
                                    lmeta.channel = std::to_string(channel_buf);

                                AncillaryTextRecord ancillary_record = file.getHeader<AncillaryTextRecord>();

                                // On GOES-R HRIT, the projection information in the Image Navigation Header is not accurate enough. Use the data
                                // in the Ancillary record instead
                                if (ancillary_record.meta.count("perspective_point_height") > 0 &&
                                    ancillary_record.meta.count("y_add_offset") > 0 && ancillary_record.meta.count("y_scale_factor") > 0)
                                {
                                    used_ancillary_proj = true;
                                    double scale_factor = std::stod(ancillary_record.meta["y_scale_factor"]);
                                    lmeta.image_navigation_record->line_scalar =
                                        std::abs(std::stod(ancillary_record.meta["perspective_point_height"]) * scale_factor);
                                    lmeta.image_navigation_record->line_offset = -std::stod(ancillary_record.meta["y_add_offset"]) / scale_factor;

                                    // Avoid upstream rounding errors on smaller images
                                    if (image_structure_record.columns_count < 5424)
                                        lmeta.image_navigation_record->line_offset = std::ceil(lmeta.image_navigation_record->line_offset);
                                }
                                if (ancillary_record.meta.count("perspective_point_height") > 0 &&
                                    ancillary_record.meta.count("x_add_offset") > 0 && ancillary_record.meta.count("x_scale_factor") > 0)
                                {
                                    double scale_factor = std::stod(ancillary_record.meta["x_scale_factor"]);
                                    lmeta.image_navigation_record->column_scalar =
                                        std::abs(std::stod(ancillary_record.meta["perspective_point_height"]) * scale_factor);
                                    lmeta.image_navigation_record->column_offset = -std::stod(ancillary_record.meta["x_add_offset"]) / scale_factor;

                                    // Avoid upstream rounding errors on smaller images
                                    if (image_structure_record.columns_count < 5424)
                                        lmeta.image_navigation_record->column_offset = std::ceil(lmeta.image_navigation_record->column_offset);
                                }

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
                                    lmeta.scan_time = timegm(&scanTimestamp);
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
                            lmeta.channel = "4";
                        else if (noaa_header.product_subid <= 20)
                            lmeta.channel = "1";
                        else if (noaa_header.product_subid <= 30)
                            lmeta.channel = "3";

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
                            lmeta.scan_time = timegm(&scanTimestamp);
                        }
                    }
                    // Himawari rebroadcast
                    else if (primary_header.file_type_code == 0 && noaa_header.product_id == ID_HIMAWARI)
                    {
                        lmeta.satellite_name = "Himawari";
                        lmeta.satellite_short_name = "HIM";
                        lmeta.channel = std::to_string(noaa_header.product_subid);
                        lmeta.region = "";

                        // Translate to real numbers
                        if (lmeta.channel == "3")
                            lmeta.channel = "13";
                        else if (lmeta.channel == "7")
                            lmeta.channel = "8";
                        else if (lmeta.channel == "1")
                            lmeta.channel = "3";

                        // Apparently the timestamp is in there for Himawari-8 data
                        AnnotationRecord annotation_record = file.getHeader<AnnotationRecord>();

                        std::vector<std::string> strParts = splitString(annotation_record.annotation_text, '_');
                        if (strParts.size() > 3)
                        {
                            strptime(strParts[2].c_str(), "%Y%m%d%H%M", timeReadable);
                            lmeta.scan_time = timegm(timeReadable);
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

                    // Internal "VCID" to use. Due to interleaving of channel segments on the Himawari relay,
                    // we need to use adjusted VCIDs to keep one channel's segment from prematurely ending
                    // another in-progress channel. Use VCID 70+ as these channels should not be used OTA.
                    int vcid = file.vcid;
                    if (noaa_header.product_id == ID_HIMAWARI)
                        vcid = 70 + std::stoi(lmeta.channel);

                    if (all_wip_images.count(vcid) == 0)
                        all_wip_images.insert({vcid, std::make_unique<wip_images>()});

                    std::unique_ptr<wip_images> &wip_img = all_wip_images[vcid];

                    wip_img->imageStatus = RECEIVING;

                    if (segmentedDecoders.count(vcid) == 0)
                        segmentedDecoders.insert({vcid, SegmentedLRITImageDecoder()});

                    SegmentedLRITImageDecoder &segmentedDecoder = segmentedDecoders[vcid];

                    if (lmeta.image_navigation_record && noaa_header.product_id != ID_HIMAWARI && !used_ancillary_proj)
                        lmeta.image_navigation_record->line_offset = lmeta.image_navigation_record->line_offset +
                                                                     segment_id_header.segment_sequence_number * (segment_id_header.max_row / segment_id_header.max_segment);

                    uint16_t image_identifier = segment_id_header.image_identifier;
                    if (noaa_header.product_id == ID_HIMAWARI) // Image IDs are invalid for Himawari; make one up
                        image_identifier = lmeta.scan_time % 10000 + std::stoi(lmeta.channel);

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
                        image::Image imageScaled = *(segmentedDecoder.image);
                        imageScaled.resize(wip_img->img_width, wip_img->img_height);
                        if (imageScaled.typesize() == 1)
                            image::image_to_rgba(imageScaled, wip_img->textureBuffer);
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
                        image::Image image(&file.lrit_data[primary_header.total_header_length], 8, image_structure_record.columns_count, image_structure_record.lines_count, 1);
                        saveImageP(lmeta, image);
                    }
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
