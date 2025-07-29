#include "identify.h"
#include "common/lrit/lrit_file.h"
#include "logger.h"
#include "msg/msg_headers.h"
#include "utils/string.h"
#include "xrit/goes/goes_headers.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include <exception>
#include <string>

namespace satdump
{
    namespace xrit
    {
        ////////////////////////////////////////
        //  Identifcation done from file data
        ////////////////////////////////////////

        bool identifyElektroFile(XRITFileInfo &i, lrit::LRITFile &file)
        {
            i.filename = file.filename;

            auto parts = splitString(i.filename, '-');

            if (parts.size() != 8)
                return false;

            if ((parts[0] != "H" && parts[0] != "L") || parts[1] != "000")
                return false;

            int sat_num;
            if (sscanf(parts[2].c_str(), "GOMS%1d", &sat_num) != 1)
                return false;

            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf(parts[6].c_str(), "%4d%2d%2d%2d%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min) != 5)
                return false;

            i.type = XRIT_ELEKTRO_MSUGS;

            timeS.tm_year -= 1900;
            timeS.tm_mon -= 1;

            i.timestamp = timegm(&timeS);
            i.instrument_id = "msu_gs";
            i.instrument_name = "MSU-GS";
            i.satellite_name = "ELEKTRO-L" + std::to_string(sat_num);
            i.satellite_short_name = "L" + std::to_string(sat_num);

            i.groupid = std::to_string(sat_num) + "_" + std::to_string((time_t)i.timestamp);
            if (parts[4] != "_________")
                i.seg_groupid = parts[5] + "_" + i.groupid;

            if (file.all_headers.count(msg::SegmentIdentificationHeader::TYPE))
            {
                i.channel = std::to_string(file.getHeader<msg::SegmentIdentificationHeader>().channel_id + 1);
                i.seg_groupid = i.groupid + "_" + i.channel;
            }

            if (file.all_headers.count(lrit::ImageStructureRecord::TYPE))
                i.bit_depth = file.getHeader<lrit::ImageStructureRecord>().bit_per_pixel;

            if (file.all_headers.count(lrit::ImageNavigationRecord::TYPE))
                i.image_navigation_record = std::make_shared<::lrit::ImageNavigationRecord>(file.getHeader<::lrit::ImageNavigationRecord>());

            // Adapt navigation if needed (to shift between segments!)
            if (file.all_headers.count(msg::SegmentIdentificationHeader::TYPE) && file.all_headers.count(lrit::ImageStructureRecord::TYPE) && i.image_navigation_record)
            {
                auto segment_id_header = file.getHeader<msg::SegmentIdentificationHeader>();
                auto image_structure_record = file.getHeader<lrit::ImageStructureRecord>();
                i.image_navigation_record->line_offset =
                    i.image_navigation_record->line_offset + (segment_id_header.segment_sequence_number - segment_id_header.planned_start_segment) * image_structure_record.lines_count;
            }

            return true;
        }

        bool identifyMSGFile(XRITFileInfo &i, lrit::LRITFile &file)
        {
            i.filename = file.filename;

            auto parts = splitString(i.filename, '-');

            if (parts.size() != 8)
                return false;

            if ((parts[0] != "H" && parts[0] != "L") || parts[1] != "000")
                return false;

            int sat_num;
            if (sscanf(parts[2].c_str(), "MSG%1d", &sat_num) != 1)
                return false;

            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf(parts[6].c_str(), "%4d%2d%2d%2d%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min) != 5)
                return false;

            i.type = XRIT_MSG_SEVIRI;

            timeS.tm_year -= 1900;
            timeS.tm_mon -= 1;

            i.timestamp = timegm(&timeS);
            i.instrument_id = "seviri";
            i.instrument_name = "SEVIRI";
            i.satellite_name = "MSG-" + std::to_string(sat_num);
            i.satellite_short_name = "MSG" + std::to_string(sat_num);

            i.groupid = std::to_string(sat_num) + "_" + std::to_string((time_t)i.timestamp);

            if (file.all_headers.count(msg::SegmentIdentificationHeader::TYPE))
            {
                i.channel = std::to_string(file.getHeader<msg::SegmentIdentificationHeader>().channel_id);
                i.seg_groupid = i.groupid + "_" + i.channel;
            }

            if (file.all_headers.count(lrit::ImageStructureRecord::TYPE))
                i.bit_depth = file.getHeader<lrit::ImageStructureRecord>().bit_per_pixel;

            if (file.all_headers.count(lrit::ImageNavigationRecord::TYPE))
                i.image_navigation_record = std::make_shared<lrit::ImageNavigationRecord>(file.getHeader<lrit::ImageNavigationRecord>());

            // Adapt navigation if needed (to shift between segments!)
            if (file.all_headers.count(msg::SegmentIdentificationHeader::TYPE) && file.all_headers.count(lrit::ImageStructureRecord::TYPE) && i.image_navigation_record)
            {
                auto segment_id_header = file.getHeader<msg::SegmentIdentificationHeader>();
                auto image_structure_record = file.getHeader<lrit::ImageStructureRecord>();
                i.image_navigation_record->line_offset =
                    i.image_navigation_record->line_offset + (segment_id_header.segment_sequence_number - segment_id_header.planned_start_segment) * image_structure_record.lines_count;
            }

            return true;
        }

        bool identifyGOESFile(XRITFileInfo &i, lrit::LRITFile &file)
        {
            ::lrit::PrimaryHeader primary_header = file.getHeader<::lrit::PrimaryHeader>();

            if (!file.hasHeader<goes::NOAALRITHeader>())
                return false;
            goes::NOAALRITHeader noaa_header = file.getHeader<goes::NOAALRITHeader>();

            if (primary_header.file_type_code == 0 && file.hasHeader<::lrit::ImageStructureRecord>())
            {
                ::lrit::ImageStructureRecord image_structure_record = file.getHeader<::lrit::ImageStructureRecord>();
                i.bit_depth = image_structure_record.bit_per_pixel;

                if (!file.hasHeader<lrit::TimeStampRecord>())
                    return false;
                ::lrit::TimeStampRecord timestamp_record = file.getHeader<lrit::TimeStampRecord>();
                std::tm *timeReadable = gmtime(&timestamp_record.timestamp);

                // GOES-R Data, from GOES-16 to 19.
                // Once again peeked in goestools for the meso detection, sorry :-)
                if (primary_header.file_type_code == 0 && (noaa_header.product_id == 16 || noaa_header.product_id == 17 || noaa_header.product_id == 18 || noaa_header.product_id == 19))
                {
                    i.type = XRIT_GOES_ABI;
                    i.instrument_id = "abi";
                    i.instrument_name = "ABI";
                    i.satellite_name = "GOES-" + std::to_string(noaa_header.product_id);
                    i.satellite_short_name = "G" + std::to_string(noaa_header.product_id);

                    std::vector<std::string> cutFilename = satdump::splitString(file.filename, '-');
                    if (cutFilename.size() < 4)
                        return false;

                    int mode = -1;
                    int channel_buf = -1;
                    if (sscanf(cutFilename[3].c_str(), "M%dC%02d", &mode, &channel_buf) != 2 && sscanf(cutFilename[3].c_str(), "M%d_", &mode) != 1)
                        return false;

                    if (channel_buf == -1)
                    {
                        i.channel = cutFilename[2];
                        if (i.channel.back() == 'F' || i.channel.back() == 'C')
                            i.channel.pop_back();
                        else if (i.channel.find_last_of('M') != std::string::npos)
                            i.channel = i.channel.substr(0, i.channel.find_last_of('M'));
                    }
                    else
                        i.channel = std::to_string(channel_buf);

                    if (file.hasHeader<goes::AncillaryTextRecord>())
                    {
                        goes::AncillaryTextRecord ancillary_record = file.getHeader<goes::AncillaryTextRecord>();

                        // Parse Region
                        if (ancillary_record.meta.count("Region") > 0)
                        {
                            std::string regionName = ancillary_record.meta["Region"];

                            if (regionName == "Full Disk")
                            {
                                i.region = "Full Disk";
                            }
                            else if (regionName == "Mesoscale")
                            {
                                if (cutFilename[2] == "CMIPM1")
                                    i.region = "Mesoscale 1";
                                else if (cutFilename[2] == "CMIPM2")
                                    i.region = "Mesoscale 2";
                                else
                                    i.region = "Mesoscale";
                            }
                        }

                        // Parse scan time
                        std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                        if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                        {
                            std::string scanTime = ancillary_record.meta["Time of frame start"];
                            strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                            i.timestamp = timegm(&scanTimestamp);
                        }
                    }
                    else
                    {
                        logger->error("No ancilliary record!");
                        return false;
                    }

                    // Group parsing
                    if (i.channel == "")
                        i.groupid = i.channel + "_" + std::to_string(noaa_header.product_id) + "_" + std::to_string((time_t)i.timestamp);
                    else
                        i.groupid = std::to_string(noaa_header.product_id) + "_" + std::to_string((time_t)i.timestamp);

                    if (file.all_headers.count(goes::SegmentIdentificationHeader::TYPE))
                        i.seg_groupid = i.channel + "_" + std::to_string(noaa_header.product_id) + "_" + std::to_string((time_t)i.timestamp);
                }
                // GOES-N Data, from GOES-13 to 15.
                else if (primary_header.file_type_code == 0 && (noaa_header.product_id == 13 || noaa_header.product_id == 14 || noaa_header.product_id == 15))
                {
                    i.type = XRIT_GOESN_IMAGER;
                    i.instrument_id = "goesn_imager";
                    i.instrument_name = "IMAGER";
                    i.satellite_name = "GOES-" + std::to_string(noaa_header.product_id);
                    i.satellite_short_name = "G" + std::to_string(noaa_header.product_id);
                    //  lmeta.is_goesn = true;

                    // Parse channel
                    if (noaa_header.product_subid <= 10)
                        i.channel = "4";
                    else if (noaa_header.product_subid <= 20)
                        i.channel = "1";
                    else if (noaa_header.product_subid <= 30)
                        i.channel = "3";

                    // Parse Region. Had to peak in goestools again...
                    if (noaa_header.product_subid % 10 == 1)
                        i.region = "Full Disk";
                    else if (noaa_header.product_subid % 10 == 2)
                        i.region = "Northern Hemisphere";
                    else if (noaa_header.product_subid % 10 == 3)
                        i.region = "Southern Hemisphere";
                    else if (noaa_header.product_subid % 10 == 4)
                        i.region = "United States";
                    else
                    {
                        char buf[32];
                        size_t len;
                        int num = (noaa_header.product_subid % 10) - 5;
                        len = snprintf(buf, 32, "Special Interest %d", num);
                        i.region = std::string(buf, len);
                    }

                    // Parse scan time
                    goes::AncillaryTextRecord ancillary_record = file.getHeader<goes::AncillaryTextRecord>();
                    std::tm scanTimestamp = *timeReadable;                      // Default to CCSDS timestamp normally...
                    if (ancillary_record.meta.count("Time of frame start") > 0) // ...unless we have a proper scan time
                    {
                        std::string scanTime = ancillary_record.meta["Time of frame start"];
                        strptime(scanTime.c_str(), "%Y-%m-%dT%H:%M:%S", &scanTimestamp);
                        i.timestamp = timegm(&scanTimestamp);
                    }

                    // Group parsing
                    i.groupid = std::to_string(noaa_header.product_id) + "_" + std::to_string((time_t)i.timestamp);

                    if (file.all_headers.count(goes::SegmentIdentificationHeader::TYPE))
                        i.seg_groupid = i.channel + "_" + std::to_string(noaa_header.product_id) + "_" + std::to_string((time_t)i.timestamp);
                }
                // Himawari rebroadcast
                else if (primary_header.file_type_code == 0 && noaa_header.product_id == goes::ID_HIMAWARI)
                {
                    i.type = XRIT_GOES_ABI;
                    i.instrument_id = "ahi";
                    i.instrument_name = "AHI";
                    i.satellite_name = "Himawari";
                    i.satellite_short_name = "HIM";
                    i.channel = std::to_string(noaa_header.product_subid);

                    // Translate to real numbers
                    if (i.channel == "3")
                        i.channel = "13";
                    else if (i.channel == "7")
                        i.channel = "8";
                    else if (i.channel == "1")
                        i.channel = "3";

                    // Apparently the timestamp is in there for Himawari-8 data
                    lrit::AnnotationRecord annotation_record = file.getHeader<lrit::AnnotationRecord>();

                    std::vector<std::string> strParts = satdump::splitString(annotation_record.annotation_text, '_');
                    if (strParts.size() > 3)
                    {
                        strptime(strParts[2].c_str(), "%Y%m%d%H%M", timeReadable);
                        i.timestamp = timegm(timeReadable);
                    }

                    // Group parsing
                    i.groupid = "him_" + std::to_string((time_t)i.timestamp);

                    if (file.all_headers.count(goes::SegmentIdentificationHeader::TYPE))
                        i.seg_groupid = i.channel + "_him_" + std::to_string((time_t)i.timestamp);
                }
                else
                    return false;

                //////////////////

                if (file.all_headers.count(lrit::ImageNavigationRecord::TYPE))
                {
                    i.image_navigation_record = std::make_shared<lrit::ImageNavigationRecord>(file.getHeader<lrit::ImageNavigationRecord>());

                    bool used_ancillary_proj = false;

                    if (file.hasHeader<goes::AncillaryTextRecord>())
                    {
                        goes::AncillaryTextRecord ancillary_record = file.getHeader<goes::AncillaryTextRecord>();

                        // On GOES-R HRIT, the projection information in the Image Navigation Header is not accurate enough. Use the data
                        // in the Ancillary record instead
                        if (ancillary_record.meta.count("perspective_point_height") > 0 && ancillary_record.meta.count("y_add_offset") > 0 && ancillary_record.meta.count("y_scale_factor") > 0)
                        {
                            used_ancillary_proj = true; // TODOREWORK????
                            double scale_factor = std::stod(ancillary_record.meta["y_scale_factor"]);
                            i.image_navigation_record->line_scalar = std::abs(std::stod(ancillary_record.meta["perspective_point_height"]) * scale_factor);
                            i.image_navigation_record->line_offset = -std::stod(ancillary_record.meta["y_add_offset"]) / scale_factor;

                            // Avoid upstream rounding errors on smaller images
                            if (image_structure_record.columns_count < 5424)
                                i.image_navigation_record->line_offset = std::ceil(i.image_navigation_record->line_offset);
                        }
                        if (ancillary_record.meta.count("perspective_point_height") > 0 && ancillary_record.meta.count("x_add_offset") > 0 && ancillary_record.meta.count("x_scale_factor") > 0)
                        {
                            double scale_factor = std::stod(ancillary_record.meta["x_scale_factor"]);
                            i.image_navigation_record->column_scalar = std::abs(std::stod(ancillary_record.meta["perspective_point_height"]) * scale_factor);
                            i.image_navigation_record->column_offset = -std::stod(ancillary_record.meta["x_add_offset"]) / scale_factor;

                            // Avoid upstream rounding errors on smaller images
                            if (image_structure_record.columns_count < 5424)
                                i.image_navigation_record->column_offset = std::ceil(i.image_navigation_record->column_offset);
                        }
                    }

                    if (file.all_headers.count(goes::SegmentIdentificationHeader::TYPE))
                    {
                        auto segment_id_header = file.getHeader<goes::SegmentIdentificationHeader>();
                        if (i.image_navigation_record && noaa_header.product_id != goes::ID_HIMAWARI && !used_ancillary_proj)
                            i.image_navigation_record->line_offset =
                                i.image_navigation_record->line_offset + segment_id_header.segment_sequence_number * (segment_id_header.max_row / segment_id_header.max_segment);
                    }
                }

                if (file.hasHeader<lrit::ImageDataFunctionRecord>())
                    i.image_data_function_record = std::make_shared<lrit::ImageDataFunctionRecord>(file.getHeader<lrit::ImageDataFunctionRecord>());
            }

            return true;
        }

        bool identifyGK2AFile(XRITFileInfo &i, lrit::LRITFile &file)
        {
            i.filename = file.filename;

            auto parts = splitString(i.filename, '_');

            if (parts.size() != 7)
                return false;

            // Only FDs are supposed to come down. We only handle other headers in this situation.
            if (parts[0] != "IMG" || parts[1] != "FD")
                return false;

            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf((parts[4] + parts[5]).c_str(), "%4d%2d%2d%2d%2d%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec) != 6)
                return false;

            i.type = XRIT_GK2A_AMI;

            timeS.tm_year -= 1900;
            timeS.tm_mon -= 1;

            i.timestamp = timegm(&timeS);
            i.instrument_id = "ami";
            i.instrument_name = "AMI";
            i.satellite_name = "GK-2A";
            i.satellite_short_name = "GK2A";

            i.groupid = std::to_string((time_t)i.timestamp);

            i.channel = parts[3];
            i.seg_groupid = i.channel + "_" + i.groupid;

            if (file.all_headers.count(lrit::ImageStructureRecord::TYPE))
                i.bit_depth = file.getHeader<lrit::ImageStructureRecord>().bit_per_pixel;

            // Try to parse navigation
            if (file.all_headers.count(lrit::ImageNavigationRecord::TYPE))
            {
                i.image_navigation_record = std::make_shared<::lrit::ImageNavigationRecord>(file.getHeader<::lrit::ImageNavigationRecord>());
                i.image_navigation_record->line_scaling_factor = -i.image_navigation_record->line_scaling_factor; // Little GK-2A Quirk...
            }

            // Try parse calibration
            if (file.hasHeader<lrit::ImageDataFunctionRecord>())
                i.image_data_function_record = std::make_shared<lrit::ImageDataFunctionRecord>(file.getHeader<lrit::ImageDataFunctionRecord>());

            return true;
        }

        bool identifyHimawariFile(XRITFileInfo &i, lrit::LRITFile &file)
        {
            i.filename = file.filename;

            auto parts = splitString(i.filename, '_');

            if (parts.size() != 4)
                return false;

            // Only FDs are supposed to come down. We only handle other headers in this situation.
            if (parts[0] != "IMG" || parts[1].find("DK01") == std::string::npos)
                return false;

            std::tm timeS;
            memset(&timeS, 0, sizeof(std::tm));
            if (sscanf(file.filename.substr(12, 12).c_str(), "%4d%2d%2d%2d%2d", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min) != 5)
                return false;

            i.type = XRIT_HIMAWARI_AHI;

            timeS.tm_year -= 1900;
            timeS.tm_mon -= 1;

            i.timestamp = timegm(&timeS);
            i.instrument_id = "ahi";
            i.instrument_name = "AHI";
            i.satellite_name = "Himawari";
            i.satellite_short_name = "HIM";

            i.groupid = std::to_string((time_t)i.timestamp);

            std::string channel_name = file.filename.substr(4, 7);
            // Parse channel number
            if (channel_name == "DK01VIS")
                i.channel = "3";
            else if (channel_name == "DK01IR4")
                i.channel = "7";
            else if (channel_name == "DK01IR3")
                i.channel = "8";
            else if (channel_name == "DK01IR1")
                i.channel = "13";
            else if (channel_name == "DK01IR2")
                i.channel = "15";
            else if (channel_name == "DK01B04")
                i.channel = "4";
            else if (channel_name == "DK01B05")
                i.channel = "5";
            else if (channel_name == "DK01B06")
                i.channel = "6";
            else if (channel_name == "DK01B07")
                i.channel = "7";
            else if (channel_name == "DK01B08")
                i.channel = "8";
            else if (channel_name == "DK01B09")
                i.channel = "9";
            else if (channel_name == "DK01B10")
                i.channel = "10";
            else if (channel_name == "DK01B11")
                i.channel = "11";
            else if (channel_name == "DK01B12")
                i.channel = "12";
            else if (channel_name == "DK01B13")
                i.channel = "13";
            else if (channel_name == "DK01B14")
                i.channel = "14";
            else if (channel_name == "DK01B15")
                i.channel = "15";
            else if (channel_name == "DK01B16")
                i.channel = "16";

            i.seg_groupid = i.channel + "_" + i.groupid;

            if (file.all_headers.count(lrit::ImageStructureRecord::TYPE))
                i.bit_depth = file.getHeader<lrit::ImageStructureRecord>().bit_per_pixel;

            // Try to parse navigation
            if (file.all_headers.count(lrit::ImageNavigationRecord::TYPE))
                i.image_navigation_record = std::make_shared<::lrit::ImageNavigationRecord>(file.getHeader<::lrit::ImageNavigationRecord>());

            // Try parse calibration
            if (file.hasHeader<lrit::ImageDataFunctionRecord>())
                i.image_data_function_record = std::make_shared<lrit::ImageDataFunctionRecord>(file.getHeader<lrit::ImageDataFunctionRecord>());

            return true;
        }

        ////////////////////////////////////////
        //  Generic functions
        ////////////////////////////////////////

        XRITFileInfo identifyXRITFIle(lrit::LRITFile &file)
        {
            XRITFileInfo i;

            if (file.lrit_data.size() == 0)
                return i;

            if (identifyElektroFile(i, file))
                return i;
            else if (identifyMSGFile(i, file))
                return i;
            else if (identifyGOESFile(i, file))
                return i;
            else if (identifyGK2AFile(i, file))
                return i;
            else if (identifyHimawariFile(i, file))
                return i;
            return XRITFileInfo();
        }
    } // namespace xrit
} // namespace satdump