#include "proj_file.h"
#include <fstream>
#include "common/utils.h"
#include "tle.h"
#include <cmath>
#include <cstring>
#include "logger.h"

namespace geodetic
{
    namespace projection
    {
        namespace proj_file
        {
            void writeReferenceFile(GeodeticReferenceFile &geofile, std::string output_file)
            {
                std::ofstream output_stream(output_file, std::ios::binary);

                if (geofile.file_type == GEO_TYPE)
                {
                    GEO_GeodeticReferenceFile &gsofile = (GEO_GeodeticReferenceFile &)geofile;

                    // Write main header
                    output_stream.write((char *)&gsofile.file_type, 1);
                    output_stream.write((char *)&gsofile.utc_timestamp_seconds, 8);

                    // Write GEO header
                    output_stream.write((char *)&gsofile.norad, 4);
                    output_stream.write((char *)&gsofile.position_longitude, 8);
                    output_stream.write((char *)&gsofile.position_height, 8);
                    output_stream.write((char *)&gsofile.projection_type, 1);
                    output_stream.write((char *)&gsofile.image_width, 4);
                    output_stream.write((char *)&gsofile.image_height, 4);
                    output_stream.write((char *)&gsofile.horizontal_scale, 8);
                    output_stream.write((char *)&gsofile.vertical_scale, 8);
                    output_stream.write((char *)&gsofile.horizontal_offset, 8);
                    output_stream.write((char *)&gsofile.vertical_offset, 8);
                    output_stream.write((char *)&gsofile.proj_sweep_x, 1);
                }
                else if (geofile.file_type == LEO_TYPE)
                {
                    LEO_GeodeticReferenceFile &leofile = (LEO_GeodeticReferenceFile &)geofile;

                    // Write main header
                    output_stream.write((char *)&leofile.file_type, 1);
                    output_stream.write((char *)&leofile.utc_timestamp_seconds, 8);

                    // Compute header size and TLE sizes
                    leofile.tle_line1_length = leofile.tle_line1_data.size();
                    leofile.tle_line2_length = leofile.tle_line2_data.size();

                    // Write LEO header
                    output_stream.write((char *)&leofile.norad, 4);
                    output_stream.write((char *)&leofile.tle_line1_length, 2);
                    output_stream.write((char *)leofile.tle_line1_data.data(), leofile.tle_line1_length);
                    output_stream.write((char *)&leofile.tle_line2_length, 2);
                    output_stream.write((char *)leofile.tle_line2_data.data(), leofile.tle_line2_length);

                    output_stream.write((char *)&leofile.scan_angle, 8);
                    output_stream.write((char *)&leofile.roll_offset, 8);
                    output_stream.write((char *)&leofile.pitch_offset, 8);
                    output_stream.write((char *)&leofile.yaw_offset, 8);
                    output_stream.write((char *)&leofile.time_offset, 8);
                    output_stream.write((char *)&leofile.image_width, 4);
                    output_stream.write((char *)&leofile.invert_scan, 1);

                    leofile.timestamp_count = leofile.utc_timestamps.size();
                    output_stream.write((char *)&leofile.timestamp_count, 8);
                    for (int i = 0; i < (int)leofile.timestamp_count; i++)
                        output_stream.write((char *)&leofile.utc_timestamps[i], 8);
                }

                output_stream.close();
            }

            std::shared_ptr<GeodeticReferenceFile> readReferenceFile(std::string input_file)
            {
                std::ifstream input_stream(input_file, std::ios::binary);

                uint8_t file_type;
                input_stream.read((char *)&file_type, 1);
                if (file_type == GEO_TYPE)
                {
                    GEO_GeodeticReferenceFile geofile;

                    // Main header
                    input_stream.read((char *)&geofile.utc_timestamp_seconds, 8);

                    // Read GEO header
                    input_stream.read((char *)&geofile.norad, 4);
                    input_stream.read((char *)&geofile.position_longitude, 8);
                    input_stream.read((char *)&geofile.position_height, 8);
                    input_stream.read((char *)&geofile.projection_type, 1);
                    input_stream.read((char *)&geofile.image_width, 4);
                    input_stream.read((char *)&geofile.image_height, 4);
                    input_stream.read((char *)&geofile.horizontal_scale, 8);
                    input_stream.read((char *)&geofile.vertical_scale, 8);
                    input_stream.read((char *)&geofile.horizontal_offset, 8);
                    input_stream.read((char *)&geofile.vertical_offset, 8);
                    input_stream.read((char *)&geofile.proj_sweep_x, 1);
                    input_stream.close();
                    return std::make_shared<GEO_GeodeticReferenceFile>(geofile);
                }
                else if (file_type == LEO_TYPE)
                {
                    LEO_GeodeticReferenceFile leofile;

                    // Main header
                    input_stream.read((char *)&leofile.utc_timestamp_seconds, 8);

                    // Read LEO header
                    input_stream.read((char *)&leofile.norad, 4);
                    input_stream.read((char *)&leofile.tle_line1_length, 2);
                    {
                        char *str = new char[leofile.tle_line1_length];
                        input_stream.read(str, leofile.tle_line1_length);
                        leofile.tle_line1_data = std::string(str);
                        delete[] str;
                    }
                    input_stream.read((char *)&leofile.tle_line2_length, 2);
                    {
                        char *str = new char[leofile.tle_line2_length];
                        input_stream.read(str, leofile.tle_line2_length);
                        leofile.tle_line2_data = std::string(str);
                        delete[] str;
                    }

                    input_stream.read((char *)&leofile.scan_angle, 8);
                    input_stream.read((char *)&leofile.roll_offset, 8);
                    input_stream.read((char *)&leofile.pitch_offset, 8);
                    input_stream.read((char *)&leofile.yaw_offset, 8);
                    input_stream.read((char *)&leofile.time_offset, 8);
                    input_stream.read((char *)&leofile.image_width, 4);
                    input_stream.read((char *)&leofile.invert_scan, 1);
                    input_stream.read((char *)&leofile.timestamp_count, 8);
                    for (int i = 0; i < (int)leofile.timestamp_count; i++)
                    {
                        double timestamp;
                        input_stream.read((char *)&timestamp, 8);
                        leofile.utc_timestamps.push_back(timestamp);
                    }

                    input_stream.close();
                    return std::make_shared<LEO_GeodeticReferenceFile>(leofile);
                }
                else
                {
                    // Unknown
                    return std::make_shared<GeodeticReferenceFile>();
                }
            }

            LEO_GeodeticReferenceFile leoRefFileFromProjector(int norad, LEOScanProjectorSettings projector_settings)
            {
                projection::proj_file::LEO_GeodeticReferenceFile geofile;

                double rough_timestamp = average_common(projector_settings.utc_timestamps.begin(), projector_settings.utc_timestamps.end());

                geofile.utc_timestamp_seconds = floor(rough_timestamp);
                geofile.norad = norad;
                geofile.tle_line1_data = projector_settings.sat_tle.line1;
                geofile.tle_line2_data = projector_settings.sat_tle.line2;

                geofile.scan_angle = projector_settings.scan_angle;
                geofile.roll_offset = projector_settings.roll_offset;
                geofile.pitch_offset = projector_settings.pitch_offset;
                geofile.yaw_offset = projector_settings.yaw_offset;
                geofile.time_offset = projector_settings.time_offset;
                geofile.image_width = projector_settings.image_width;
                geofile.invert_scan = projector_settings.invert_scan;
                geofile.utc_timestamps = projector_settings.utc_timestamps;

                return geofile;
            }

            LEOScanProjectorSettings leoProjectionRefFile(LEO_GeodeticReferenceFile geofile)
            {
                LEOScanProjectorSettings projector_settings = {
                    (double)geofile.scan_angle,
                    (double)geofile.roll_offset,
                    (double)geofile.pitch_offset,
                    (double)geofile.yaw_offset,
                    (double)geofile.time_offset,
                    (int)geofile.image_width,
                    (bool)geofile.invert_scan,
                    {(int)geofile.norad, "UNKNOWN", geofile.tle_line1_data, geofile.tle_line2_data},
                    geofile.utc_timestamps};

                return projector_settings;
            }

            GEOProjector geoProjectionRefFile(GEO_GeodeticReferenceFile geofile)
            {
                GEOProjector projector(geofile.position_longitude,
                                       geofile.position_height,
                                       geofile.image_width,
                                       geofile.image_height,
                                       geofile.horizontal_scale,
                                       geofile.vertical_scale,
                                       geofile.horizontal_offset,
                                       geofile.horizontal_offset,
                                       geofile.proj_sweep_x);
                return projector;
            }
        };
    };
};