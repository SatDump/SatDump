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
                    output_stream.write((char *)&leofile.projection_type, 1);

                    if (leofile.projection_type == TIMESTAMP_PER_SCANLINE)
                    {
                        output_stream.write((char *)&leofile.scanline.scan_angle, 8);
                        output_stream.write((char *)&leofile.scanline.roll_offset, 8);
                        output_stream.write((char *)&leofile.scanline.pitch_offset, 8);
                        output_stream.write((char *)&leofile.scanline.yaw_offset, 8);
                        output_stream.write((char *)&leofile.scanline.time_offset, 8);
                        output_stream.write((char *)&leofile.scanline.image_width, 4);
                        output_stream.write((char *)&leofile.scanline.invert_scan, 1);

                        leofile.scanline.timestamp_count = leofile.scanline.utc_timestamps.size();
                        output_stream.write((char *)&leofile.scanline.timestamp_count, 8);
                        for (int i = 0; i < (int)leofile.scanline.timestamp_count; i++)
                            output_stream.write((char *)&leofile.scanline.utc_timestamps[i], 8);
                    }
                    else if (leofile.projection_type == TIMESTAMP_PER_IFOV)
                    {
                        output_stream.write((char *)&leofile.ifov.scan_angle, 8);
                        output_stream.write((char *)&leofile.ifov.ifov_x_scan_angle, 8);
                        output_stream.write((char *)&leofile.ifov.ifov_y_scan_angle, 8);
                        output_stream.write((char *)&leofile.ifov.roll_offset, 8);
                        output_stream.write((char *)&leofile.ifov.pitch_offset, 8);
                        output_stream.write((char *)&leofile.ifov.yaw_offset, 8);
                        output_stream.write((char *)&leofile.ifov.time_offset, 8);
                        output_stream.write((char *)&leofile.ifov.ifov_count, 4);
                        output_stream.write((char *)&leofile.ifov.ifov_x_size, 4);
                        output_stream.write((char *)&leofile.ifov.ifov_y_size, 4);
                        output_stream.write((char *)&leofile.ifov.image_width, 4);
                        output_stream.write((char *)&leofile.ifov.invert_scan, 1);

                        leofile.ifov.timestamp_count = leofile.ifov.utc_timestamps.size();
                        output_stream.write((char *)&leofile.ifov.timestamp_count, 8);
                        for (int i = 0; i < (int)leofile.ifov.timestamp_count; i++)
                            for (int y = 0; y < (int)leofile.ifov.ifov_count; y++)
                                output_stream.write((char *)&leofile.ifov.utc_timestamps[i][y], 8);
                    }
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

                    input_stream.read((char *)&leofile.projection_type, 1);

                    if (leofile.projection_type == TIMESTAMP_PER_SCANLINE)
                    {
                        input_stream.read((char *)&leofile.scanline.scan_angle, 8);
                        input_stream.read((char *)&leofile.scanline.roll_offset, 8);
                        input_stream.read((char *)&leofile.scanline.pitch_offset, 8);
                        input_stream.read((char *)&leofile.scanline.yaw_offset, 8);
                        input_stream.read((char *)&leofile.scanline.time_offset, 8);
                        input_stream.read((char *)&leofile.scanline.image_width, 4);
                        input_stream.read((char *)&leofile.scanline.invert_scan, 1);

                        input_stream.read((char *)&leofile.scanline.timestamp_count, 8);
                        for (int i = 0; i < (int)leofile.scanline.timestamp_count; i++)
                        {
                            double timestamp;
                            input_stream.read((char *)&timestamp, 8);
                            leofile.scanline.utc_timestamps.push_back(timestamp);
                        }
                    }
                    else if (leofile.projection_type == TIMESTAMP_PER_IFOV)
                    {
                        input_stream.read((char *)&leofile.ifov.scan_angle, 8);
                        input_stream.read((char *)&leofile.ifov.ifov_x_scan_angle, 8);
                        input_stream.read((char *)&leofile.ifov.ifov_y_scan_angle, 8);
                        input_stream.read((char *)&leofile.ifov.roll_offset, 8);
                        input_stream.read((char *)&leofile.ifov.pitch_offset, 8);
                        input_stream.read((char *)&leofile.ifov.yaw_offset, 8);
                        input_stream.read((char *)&leofile.ifov.time_offset, 8);
                        input_stream.read((char *)&leofile.ifov.ifov_count, 4);
                        input_stream.read((char *)&leofile.ifov.ifov_x_size, 4);
                        input_stream.read((char *)&leofile.ifov.ifov_y_size, 4);
                        input_stream.read((char *)&leofile.ifov.image_width, 4);
                        input_stream.read((char *)&leofile.ifov.invert_scan, 1);

                        input_stream.read((char *)&leofile.ifov.timestamp_count, 8);
                        for (int i = 0; i < (int)leofile.ifov.timestamp_count; i++)
                        {
                            std::vector<double> scanLine;
                            for (int y = 0; y < (int)leofile.ifov.ifov_count; y++)
                            {
                                double timestamp;
                                input_stream.read((char *)&timestamp, 8);
                                scanLine.push_back(timestamp);
                            }
                            leofile.ifov.utc_timestamps.push_back(scanLine);
                        }
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

            LEO_GeodeticReferenceFile leoRefFileFromProjector(int norad, std::shared_ptr<LEOScanProjectorSettings> projector_settings)
            {
                projection::proj_file::LEO_GeodeticReferenceFile geofile;

                if (projector_settings->type == TIMESTAMP_PER_SCANLINE)
                {
                    LEOScanProjectorSettings_SCANLINE *settings = (LEOScanProjectorSettings_SCANLINE *)projector_settings.get();
                    double rough_timestamp = average_common(settings->utc_timestamps.begin(), settings->utc_timestamps.end());

                    geofile.utc_timestamp_seconds = floor(rough_timestamp);
                    geofile.norad = norad;
                    geofile.tle_line1_data = settings->sat_tle.line1;
                    geofile.tle_line2_data = settings->sat_tle.line2;
                    geofile.projection_type = TIMESTAMP_PER_SCANLINE;

                    geofile.scanline.scan_angle = settings->scan_angle;
                    geofile.scanline.roll_offset = settings->roll_offset;
                    geofile.scanline.pitch_offset = settings->pitch_offset;
                    geofile.scanline.yaw_offset = settings->yaw_offset;
                    geofile.scanline.time_offset = settings->time_offset;
                    geofile.scanline.image_width = settings->image_width;
                    geofile.scanline.invert_scan = settings->invert_scan;
                    geofile.scanline.utc_timestamps = settings->utc_timestamps;
                }
                else if (projector_settings->type == TIMESTAMP_PER_IFOV)
                {
                    LEOScanProjectorSettings_IFOV *settings = (LEOScanProjectorSettings_IFOV *)projector_settings.get();
                    std::vector<double> all_timestamps;
                    for (std::vector<double> &scan_timestamps : settings->utc_timestamps)
                        all_timestamps.insert(all_timestamps.end(), scan_timestamps.begin(), scan_timestamps.end());
                    double rough_timestamp = average_common(all_timestamps.begin(), all_timestamps.end());
                    all_timestamps.clear();

                    geofile.utc_timestamp_seconds = floor(rough_timestamp);
                    geofile.norad = norad;
                    geofile.tle_line1_data = settings->sat_tle.line1;
                    geofile.tle_line2_data = settings->sat_tle.line2;
                    geofile.projection_type = TIMESTAMP_PER_IFOV;

                    geofile.ifov.scan_angle = settings->scan_angle;
                    geofile.ifov.ifov_x_scan_angle = settings->ifov_x_scan_angle;
                    geofile.ifov.ifov_y_scan_angle = settings->ifov_y_scan_angle;
                    geofile.ifov.roll_offset = settings->roll_offset;
                    geofile.ifov.pitch_offset = settings->pitch_offset;
                    geofile.ifov.yaw_offset = settings->yaw_offset;
                    geofile.ifov.time_offset = settings->time_offset;
                    geofile.ifov.ifov_count = settings->ifov_count;
                    geofile.ifov.ifov_x_size = settings->ifov_x_size;
                    geofile.ifov.ifov_y_size = settings->ifov_y_size;
                    geofile.ifov.image_width = settings->image_width;
                    geofile.ifov.invert_scan = settings->invert_scan;
                    geofile.ifov.utc_timestamps = settings->utc_timestamps;
                }

                return geofile;
            }

            std::shared_ptr<LEOScanProjectorSettings> leoProjectionRefFile(LEO_GeodeticReferenceFile geofile)
            {
                std::shared_ptr<LEOScanProjectorSettings> settings;

                if (geofile.projection_type == TIMESTAMP_PER_SCANLINE)
                {
                    LEOScanProjectorSettings_SCANLINE projector_settings(
                        (double)geofile.scanline.scan_angle,
                        (double)geofile.scanline.roll_offset,
                        (double)geofile.scanline.pitch_offset,
                        (double)geofile.scanline.yaw_offset,
                        (double)geofile.scanline.time_offset,
                        (int)geofile.scanline.image_width,
                        (bool)geofile.scanline.invert_scan,
                        {(int)geofile.norad, "UNKNOWN", geofile.tle_line1_data, geofile.tle_line2_data},
                        geofile.scanline.utc_timestamps);

                    settings = std::make_shared<LEOScanProjectorSettings_SCANLINE>(projector_settings);
                }
                else if (geofile.projection_type == TIMESTAMP_PER_IFOV)
                {
                    LEOScanProjectorSettings_IFOV projector_settings(
                        (double)geofile.ifov.scan_angle,
                        (double)geofile.ifov.ifov_x_scan_angle,
                        (double)geofile.ifov.ifov_y_scan_angle,
                        (double)geofile.ifov.roll_offset,
                        (double)geofile.ifov.pitch_offset,
                        (double)geofile.ifov.yaw_offset,
                        (double)geofile.ifov.time_offset,
                        (int)geofile.ifov.ifov_count,
                        (int)geofile.ifov.ifov_x_size,
                        (int)geofile.ifov.ifov_y_size,
                        (int)geofile.ifov.image_width,
                        (bool)geofile.ifov.invert_scan,
                        {(int)geofile.norad, "UNKNOWN", geofile.tle_line1_data, geofile.tle_line2_data},
                        geofile.ifov.utc_timestamps);

                    settings = std::make_shared<LEOScanProjectorSettings_IFOV>(projector_settings);
                }

                return settings;
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
                                       geofile.vertical_offset,
                                       geofile.proj_sweep_x);
                return projector;
            }
        };
    };
};