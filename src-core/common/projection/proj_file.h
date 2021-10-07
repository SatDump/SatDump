#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "leo_projection.h"
#include <memory>

namespace projection
{
    namespace proj_file
    {
        enum GeoReferenceFileTypes
        {
            GEO_TYPE = 1,
            LEO_TYPE = 2,
        };

        /*
         Class used to write & read projection files used in SatDump for geo-referencing
         They contain the following data :
        
         Header :
         - File type marker :
             1 = GEO Sat Data
             2 = LEO Sat Data
           Different data types (GEO, LEO, etc) require different data types,
           indicated by that marker. More may come later.
        
         - 64-bit UTC seconds timestamps :
           The timestamp is meant to give a rough time of acquisition. It
           can either be an average for LEO satellites, or the time of
           acquisition for GEO satellites.
        */
        struct GeodeticReferenceFile
        {
            uint8_t file_type;              // File type
            uint64_t utc_timestamp_seconds; // Timestamp. Depending on the type it may be approximate or not
        };

        /*
         GEO Type 1 file.
        */
        struct GEO_GeodeticReferenceFile : public GeodeticReferenceFile
        {
            // Type 1
            uint32_t norad;            // NORAD ID of the GEO Satellite
            double position_longitude; // Longitude of the satellite at the time of aquisition
            double position_height;    // Orbit height at the time of acquisition
            uint8_t projection_type;   // 0 is full disk, 1 is cropped, etc
            uint32_t image_width;      // Width of the image
            uint32_t image_height;     // Height of the image
            double horizontal_scale;   // Horizontal scaling
            double vertical_scale;     // Vertical scaling
            double horizontal_offset;  // Horizontal offset
            double vertical_offset;    // Vertical offset
            bool proj_sweep_x;         // Scan axis

            GEO_GeodeticReferenceFile()
            {
                projection_type = 0;
                file_type = GEO_TYPE;
            }
        };

        /*
         LEO Type 2 file.
        */
        struct LEO_GeodeticReferenceFile : public GeodeticReferenceFile
        {
            // Type 2
            uint32_t norad;             // NORAD ID of the LEO Satellite
            uint16_t tle_line1_length;  // Size of TLE Line 1
            std::string tle_line1_data; // TLE Line 1 data
            uint16_t tle_line2_length;  // Size of TLE Line 1
            std::string tle_line2_data; // TLE Line 1 data

            enum LEOProjectionType
            {
                SINGLE_SCANLINE,
            };

            uint8_t projection_type; // Projection type

            double proj_offset;
            uint32_t correction_swath;
            double correction_res;
            double correction_height;
            double instrument_swath;
            double proj_scale;
            double az_offset;
            double tilt_offset;
            double time_offset;
            uint32_t image_width;
            bool invert_scan;
            uint64_t timestamp_count;
            std::vector<double> utc_timestamps; // Timestamps. Must match each scanline of the image you will be working with

            LEO_GeodeticReferenceFile()
            {
                projection_type = SINGLE_SCANLINE;
                file_type = LEO_TYPE;
            }
        };

        // Functions
        void writeReferenceFile(GeodeticReferenceFile &geofile, std::string output_file);
        std::shared_ptr<GeodeticReferenceFile> readReferenceFile(std::string input_file);
        LEO_GeodeticReferenceFile leoRefFileFromProjector(int norad, LEOScanProjectorSettings projector_settings);
        LEOScanProjectorSettings leoProjectionRefFile(LEO_GeodeticReferenceFile geofile);
    };
};