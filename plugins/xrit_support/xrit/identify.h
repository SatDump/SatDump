#pragma once

#include "common/lrit/lrit_file.h"
#include <memory>
#include <string>

namespace satdump
{
    namespace xrit
    {
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
        };

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

            std::shared_ptr<lrit::ImageNavigationRecord> image_navigation_record;
            std::shared_ptr<::lrit::ImageDataFunctionRecord> image_data_function_record;

            XRITFileInfo() : type(XRIT_UNKNOWN) {}
        };

        bool identifyElektroFile(XRITFileInfo &i, lrit::LRITFile &file);
        bool identifyMSGFile(XRITFileInfo &i, lrit::LRITFile &file);
        bool identifyGOESFile(XRITFileInfo &i, lrit::LRITFile &file);
        bool identifyGK2AFile(XRITFileInfo &i, lrit::LRITFile &file);
        bool identifyHimawariFile(XRITFileInfo &i, lrit::LRITFile &file);

        XRITFileInfo identifyXRITFIle(lrit::LRITFile &file);
    } // namespace xrit
} // namespace satdump