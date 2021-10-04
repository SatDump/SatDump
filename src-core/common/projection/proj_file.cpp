#include "proj_file.h"
#include <fstream>
#include "common/utils.h"
#include "tle.h"
#include <cmath>
#include <cstring>
#include "logger.h"

namespace projection
{
    namespace proj_file
    {
        void writeReferenceFile(GeodeticReferenceFile &geofile, std::string output_file)
        {
            std::ofstream output_stream(output_file, std::ios::binary);

            if (geofile.file_type == GEO_TYPE)
            {
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

                output_stream.write((char *)&leofile.proj_offset, 8);
                output_stream.write((char *)&leofile.correction_swath, 4);
                output_stream.write((char *)&leofile.correction_res, 8);
                output_stream.write((char *)&leofile.correction_height, 8);
                output_stream.write((char *)&leofile.instrument_swath, 8);
                output_stream.write((char *)&leofile.proj_scale, 8);
                output_stream.write((char *)&leofile.az_offset, 8);
                output_stream.write((char *)&leofile.tilt_offset, 8);
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

        LEO_GeodeticReferenceFile readLEOReferenceFile(std::string input_file)
        {
            std::ifstream input_stream(input_file, std::ios::binary);

            uint8_t file_type;
            input_stream.read((char *)&file_type, 1);

            if (file_type == LEO_TYPE)
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

                input_stream.read((char *)&leofile.proj_offset, 8);
                input_stream.read((char *)&leofile.correction_swath, 4);
                input_stream.read((char *)&leofile.correction_res, 8);
                input_stream.read((char *)&leofile.correction_height, 8);
                input_stream.read((char *)&leofile.instrument_swath, 8);
                input_stream.read((char *)&leofile.proj_scale, 8);
                input_stream.read((char *)&leofile.az_offset, 8);
                input_stream.read((char *)&leofile.tilt_offset, 8);
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
                return leofile;
            }
            else
            {
                // Unknown
                return LEO_GeodeticReferenceFile();
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

            geofile.proj_offset = projector_settings.proj_offset;
            geofile.correction_swath = projector_settings.correction_swath;
            geofile.correction_res = projector_settings.correction_res;
            geofile.correction_height = projector_settings.correction_height;
            geofile.instrument_swath = projector_settings.instrument_swath;
            geofile.proj_scale = projector_settings.proj_scale;
            geofile.az_offset = projector_settings.az_offset;
            geofile.tilt_offset = projector_settings.tilt_offset;
            geofile.time_offset = projector_settings.time_offset;
            geofile.image_width = projector_settings.image_width;
            geofile.invert_scan = projector_settings.invert_scan;
            geofile.utc_timestamps = projector_settings.utc_timestamps;

            return geofile;
        }

        LEOScanProjectorSettings leoProjectionRefFile(LEO_GeodeticReferenceFile geofile)
        {
            LEOScanProjectorSettings projector_settings = {
                geofile.proj_offset,
                geofile.correction_swath,
                geofile.correction_res,
                geofile.correction_height,
                geofile.instrument_swath,
                geofile.proj_scale,
                geofile.az_offset,
                geofile.tilt_offset,
                geofile.time_offset,
                geofile.image_width,
                geofile.invert_scan,
                (tle::TLE){geofile.norad, "UNKNOWN", geofile.tle_line1_data, geofile.tle_line2_data},
                geofile.utc_timestamps};

            return projector_settings;
        }
    };
};