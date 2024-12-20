#include "himawari_ahi.h"

#include "common/image/image.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "core/exception.h"
#include "libs/bzlib_utils.h"
#include "logger.h"

#include <set>
#include <filesystem>

namespace hsd2pro
{
    struct ParsedHimawariAHI
    {
        bool header_parsed = false;
        image::Image img;
        uint16_t channel;
        double longitude;
        int cfac, lfac;
        float coff, loff;
        double altitude;
        double wavenumber;
        double calibration_scale;
        double calibration_offset;
        double kappa;
        time_t timestamp;
        std::string sat_name;
    };

    enum HSDBlocks
    {
        HSD_BASIC_INFO,
        HSD_DATA_INFO,
        HSD_PROJ_INFO,
        HSD_NAV_INFO,
        HSD_CAL_INFO,
        HSD_INTER_CAL_INFO,
        HSD_SEGMENT_INFO,
        HSD_NAV_CORRECTION_INFO,
        HSD_OBS_TIME_INFO,
        HSD_ERROR_INFO,
        HSD_SPARE_BLOCK,
        HSD_DATA_BLOCK
    };

    void parse_himawari_ahi_hsd(ParsedHimawariAHI &image_out, std::vector<uint8_t> &data)
    {
        size_t source_offset = 0, dest_offset = 0;
        const unsigned int buffer_size = 100000000;
        unsigned int dest_length;
        uint8_t *decomp_dest = new uint8_t[buffer_size];
        int bz2_result;

        // Decompress
        do
        {
            dest_length = buffer_size - dest_offset;
            unsigned int consumed;
            bz2_result = BZ2_bzBuffToBuffDecompress_M((char*)decomp_dest + dest_offset, &dest_length,
                (char*)data.data() + source_offset, data.size() - source_offset, &consumed, 0, 0);
            source_offset += consumed;
            dest_offset += dest_length;
        } while (bz2_result == 0 && data.size() > source_offset);

        //Free up memory
        std::vector<uint8_t>().swap(data);

        // Process if decompression was successful
        if (bz2_result == BZ_OK)
        {
            // Load starting offset of all 12 HSD blocks
            size_t block_offsets[12];
            block_offsets[0] = 0;
            for (int i = 1; i < 12; i++)
            {
                block_offsets[i] = block_offsets[i - 1] + (uint16_t)(decomp_dest[block_offsets[i - 1] + 2] << 8 | decomp_dest[block_offsets[i - 1] + 1]);

                // Basic sanity checks
                if (block_offsets[i] > dest_length || (i < 11 && decomp_dest[block_offsets[i]] != i + 1))
                {
                    logger->error("Invalid HSD File!");
                    delete[] decomp_dest;
                    return;
                }
            }

            // HSD can also have the data block compressed. NOAA doesn't distribute files like this, so we're
            // not handling it here
            if (decomp_dest[block_offsets[HSD_DATA_INFO] + 9] != 0)
            {
                logger->error("Cannot open HSD files where data block is BZipped/GZipped!");
                delete[] decomp_dest;
                return;
            }

            // Parse Header Information
            size_t num_columns = decomp_dest[block_offsets[HSD_DATA_INFO] + 6] << 8 | decomp_dest[block_offsets[HSD_DATA_INFO] + 5];
            size_t num_lines_per_segment = decomp_dest[block_offsets[HSD_DATA_INFO] + 8] << 8 | decomp_dest[block_offsets[HSD_DATA_INFO] + 7];
            uint8_t bit_depth = decomp_dest[block_offsets[HSD_CAL_INFO] + 13]; //Is a 16-bit value, but the value is always below 16, so...

            size_t pixel_offset = num_columns *
                ((decomp_dest[block_offsets[HSD_SEGMENT_INFO] + 6] << 8 | decomp_dest[block_offsets[HSD_SEGMENT_INFO] + 5]) - 1);

            if (!image_out.header_parsed)
            {
                image_out.sat_name = std::string((char*)&decomp_dest[block_offsets[HSD_BASIC_INFO] + 6]);

                double mtd_timestamp;
                memcpy(&mtd_timestamp, &decomp_dest[block_offsets[HSD_BASIC_INFO] + 46], 8);
                image_out.timestamp = ((mtd_timestamp - 40587) * 86400);

                memcpy(&image_out.longitude, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 3], 8);
                memcpy(&image_out.cfac, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 11], 4);
                memcpy(&image_out.lfac, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 15], 4);
                memcpy(&image_out.coff, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 19], 4);
                memcpy(&image_out.loff, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 23], 4);

                double distance_earth_center, earth_equitorial_radius;
                memcpy(&distance_earth_center, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 27], 8);
                memcpy(&earth_equitorial_radius, &decomp_dest[block_offsets[HSD_PROJ_INFO] + 35], 8);
                image_out.altitude = (distance_earth_center - earth_equitorial_radius) * 1000;

                memcpy(&image_out.channel, &decomp_dest[block_offsets[HSD_CAL_INFO] + 3], 2);
                memcpy(&image_out.wavenumber, &decomp_dest[block_offsets[HSD_CAL_INFO] + 5], 8);
                memcpy(&image_out.calibration_scale, &decomp_dest[block_offsets[HSD_CAL_INFO] + 19], 8);
                memcpy(&image_out.calibration_offset, &decomp_dest[block_offsets[HSD_CAL_INFO] + 27], 8);

                image_out.wavenumber = 1e4 / image_out.wavenumber;
                if (image_out.channel < 7)
                    memcpy(&image_out.kappa, &decomp_dest[block_offsets[HSD_CAL_INFO] + 35], 8);
                else
                    image_out.kappa = -999;

                uint8_t num_segs = decomp_dest[block_offsets[HSD_SEGMENT_INFO] + 3];
                image_out.img.init(16, num_columns, num_lines_per_segment * num_segs, 1);
                image_out.calibration_scale /= pow(2, 16 - bit_depth);
                image_out.header_parsed = true;
            }

            // Get the image itself
            for (size_t i = 0; i < num_columns * num_lines_per_segment; i++)
            {
                uint16_t px_val = decomp_dest[block_offsets[HSD_DATA_BLOCK] + (i * 2) + 1] << 8 | decomp_dest[block_offsets[HSD_DATA_BLOCK] + (i * 2)];
                if (px_val >= 65534) // Error and outside scan range
                    px_val = 0;
                image_out.img.set(pixel_offset + i, px_val << (16 - bit_depth));
            }
        }

        // Handle errors on decompression
        else if (bz2_result == BZ_MEM_ERROR)
            logger->error("Bzip2 Memory Error!");
        else if (bz2_result == BZ_OUTBUFF_FULL)
            logger->error("Bzip2 output buffer full! Open an issue about this on GitHub");
        else if (bz2_result == BZ_DATA_ERROR)
            logger->error("Data integrity error detected in BZip2 file!");
        else if (bz2_result == BZ_DATA_ERROR_MAGIC)
            logger->error("Incorrect BZip2 magic bytes!");
        else if (bz2_result == BZ_UNEXPECTED_EOF)
            logger->error("Compressed BZip2 data ends unexpectedly!");
        else
            logger->error("Unknown BZip2 error");

        delete[] decomp_dest;
    }

	void process_himawari_ahi(std::string hsd_file, std::string pro_output_file, double *progress)
	{
        std::vector<ParsedHimawariAHI> all_images;
        std::map<std::string, std::set<std::string>> files_to_parse; // Channel, paths
        {
            auto ori_splt = splitString(std::filesystem::path(hsd_file).stem().string(), '_');
            if (ori_splt.size() != 8)
                throw satdump_exception("Invalid Himawari filename!");

            // Find all input files for this product
            std::filesystem::recursive_directory_iterator pluginIterator(std::filesystem::path(hsd_file).parent_path());
            std::error_code iteratorError;
            while (pluginIterator != std::filesystem::recursive_directory_iterator())
            {
                std::string path = pluginIterator->path().string();
                if (!std::filesystem::is_regular_file(pluginIterator->path()) || pluginIterator->path().extension() != ".bz2")
                    goto skip_this;

                {
                    std::string cleanname = std::filesystem::path(path).stem().string();
                    auto splt = splitString(cleanname, '_');

                    if (splt.size() == 8 &&
                        ori_splt[0] == splt[0] &&
                        ori_splt[1] == splt[1] &&
                        ori_splt[2] == splt[2] &&
                        ori_splt[3] == splt[3] &&
                        ori_splt[5] == splt[5])
                    {
                        files_to_parse[splt[4]].insert(path);
                    }
                }

            skip_this:
                pluginIterator.increment(iteratorError);
                if (iteratorError)
                    logger->critical(iteratorError.message());
            }

            // Parse all files, by channel
            int parsed_count = 0;
            for (auto &channel_files : files_to_parse)
            {
                ParsedHimawariAHI image_out;
                for (auto &this_file : channel_files.second)
                {
                    std::vector<uint8_t> bz2_file;
                    std::ifstream input_file(this_file, std::ios::binary);
                    input_file.seekg(0, std::ios::end);
                    const size_t bz2_size = input_file.tellg();
                    bz2_file.resize(bz2_size);
                    input_file.seekg(0, std::ios::beg);
                    input_file.read((char*)&bz2_file[0], bz2_size);
                    input_file.close();

                    logger->info("Processing " + this_file);
                    parse_himawari_ahi_hsd(image_out, bz2_file);
                    *progress = double(++parsed_count) / double(files_to_parse.size() * files_to_parse.begin()->second.size());
                }

                all_images.emplace_back(image_out);
            }

            // Saving
            satdump::ImageProducts ahi_products;
            ahi_products.instrument_name = "ahi";
            ahi_products.has_timestamps = false;
            ahi_products.bit_depth = 16;

            time_t prod_timestamp = 0;
            std::string sat_name = all_images[0].sat_name;
            size_t largest_width = 0, largest_height = 0;
            double center_longitude = 0;
            ParsedHimawariAHI largestImg;

            for (auto &img : all_images)
            {
                ahi_products.images.push_back({ "AHI-" + std::to_string(img.channel), std::to_string(img.channel), img.img });

                bool waslarger = false;
                if (img.img.width() > largest_width)
                {
                    largest_width = img.img.width();
                    waslarger = true;
                }
                if (img.img.height() > largest_height)
                {
                    largest_height = img.img.height();
                    waslarger = true;
                }

                img.img.clear();
                if (waslarger)
                    largestImg = img;

                prod_timestamp = img.timestamp;
                sat_name = img.sat_name;
                center_longitude = img.longitude;
            }

            ahi_products.set_product_timestamp(prod_timestamp);
            ahi_products.set_product_source(sat_name);

            {
                nlohmann::json proj_cfg;

                constexpr double k = 624597.0334223134;
                double scalar_x = (pow(2.0, 16.0) / double(largestImg.cfac)) * k;
                double scalar_y = (pow(2.0, 16.0) / double(largestImg.lfac)) * k;

                proj_cfg["type"] = "geos";
                proj_cfg["lon0"] = center_longitude;
                proj_cfg["sweep_x"] = false;
                proj_cfg["scalar_x"] = scalar_x;
                proj_cfg["scalar_y"] = -scalar_y;
                proj_cfg["offset_x"] = double(largestImg.coff) * -scalar_x;
                proj_cfg["offset_y"] = double(largestImg.loff) * scalar_y;
                proj_cfg["width"] = largest_width;
                proj_cfg["height"] = largest_height;
                proj_cfg["altitude"] = largestImg.altitude;
                ahi_products.set_proj_cfg(proj_cfg);
            }

            nlohmann::json calib_cfg;
            calib_cfg["calibrator"] = "goes_nc_abi";
            for (size_t i = 0; i < all_images.size(); i++)
            {
                calib_cfg["vars"]["scale"][i] = all_images[i].calibration_scale;
                calib_cfg["vars"]["offset"][i] = all_images[i].calibration_offset;
                calib_cfg["vars"]["kappa"][i] = all_images[i].kappa;
            }
            ahi_products.set_calibration(calib_cfg);

            const float himawari_ahi_radiance_ranges_table[16][2] = {
                {0, 1},
                {0, 1},
                {0, 1},
                {0, 1},
                {0, 1},
                {0, 1},
                {0.01, 2.20}, // 3900,
                {0, 6},       // 6190,
                {0, 16},      // 6950,
                {0.02, 20},   // 7340,
                {5, 100},     // 8500,
                {10, 120},    // 9610,
                {10, 120},    // 10350,
                {10, 150},    // 11200,
                {20, 150},    // 12300,
                {20, 150},    // 13300,
            };

            for (int i = 0; i < (int)all_images.size(); i++)
            {
                ahi_products.set_wavenumber(i, all_images[i].wavenumber);
                ahi_products.set_calibration_default_radiance_range(i, himawari_ahi_radiance_ranges_table[all_images[i].channel - 1][0],
                    himawari_ahi_radiance_ranges_table[all_images[i].channel - 1][1]);

                if (all_images[i].kappa > 0)
                    ahi_products.set_calibration_type(i, satdump::ImageProducts::CALIB_REFLECTANCE);
                else
                    ahi_products.set_calibration_type(i, satdump::ImageProducts::CALIB_RADIANCE);
            }

            if (!std::filesystem::exists(pro_output_file))
                std::filesystem::create_directories(pro_output_file);
            ahi_products.save(pro_output_file);
        }
	}
}