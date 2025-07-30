#pragma once

/**
 * @file identify.h
 * @brief Contains functions to identify and process xRIT files
 */

#include "xrit_file.h"
#include <memory>
#include <string>

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief Identifier to differentiate different
         * format of xRIT files (do note some are duplicate!)
         */
        enum xrit_file_type_t
        {
            XRIT_UNKNOWN,
            XRIT_ELEKTRO_MSUGS,
            XRIT_MSG_SEVIRI,
            XRIT_GOES_ABI,
            XRIT_GOES_HIMAWARI_AHI,
            XRIT_GOESN_IMAGER,
            XRIT_GK2A_AMI,
            XRIT_HIMAWARI_AHI,
            XRIT_FY4_AGRI,
        };

        /**
         * @brief Struct holding all identifying information
         * on a xRIT files being processed.
         *
         * @param filename the xRIT filename, with or without extension
         * @param type file type, see the struct
         * @param instrument_id the instrument this applies is from
         * @param instrument_name human name for the instrument
         * @param satellite_name human name for the satellite
         * @param satellite_short_name short ID for the satellite (eg, G16)
         * @param region region the file covers, if applicable (eg, full disk or CONUS)
         * @param timestamp usually, creation of start of scan time
         * @param groupid an unique identifier of files that belong to a same group,
         * that is usually all channels of a single imager scan
         * @param seg_groupid an unique ID of files that are segments of a same image
         * @param channel instrument channel name. Can be NULL for metadata files (eg, MSG/ELEKTRO)
         * @param bit_depth if applicable, bit depth of the channel
         * @param image_navigation_record parsed navigation header (if present)
         * @param image_data_function_record parsed calibration header (if present)
         */
        struct XRITFileInfo
        {
            std::string filename;
            xrit_file_type_t type;
            std::string instrument_id;
            std::string instrument_name;
            std::string satellite_name;
            std::string satellite_short_name;
            std::string region;
            double timestamp = -1;
            std::string groupid;
            std::string seg_groupid;
            std::string channel;
            int bit_depth = -1;

            ///// std::string channel_str; // TODOREWORK?

            std::shared_ptr<ImageNavigationRecord> image_navigation_record;
            std::shared_ptr<ImageDataFunctionRecord> image_data_function_record;

            XRITFileInfo() : type(XRIT_UNKNOWN) {}
        };

        bool identifyElektroFile(XRITFileInfo &i, XRITFile &file);
        bool identifyMSGFile(XRITFileInfo &i, XRITFile &file);
        bool identifyGOESFile(XRITFileInfo &i, XRITFile &file);
        bool identifyGK2AFile(XRITFileInfo &i, XRITFile &file);
        bool identifyHimawariFile(XRITFileInfo &i, XRITFile &file);
        bool identifyFY4File(XRITFileInfo &i, XRITFile &file);

        /**
         * @brief Identify a xRIT file, in order to know
         * what further processing to apply. This calls each
         * satellite-specific function.
         *
         * @param file xRIT file to ID
         * @return identifying struct
         */
        XRITFileInfo identifyXRITFIle(XRITFile &file);
    } // namespace xrit
} // namespace satdump