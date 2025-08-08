#include "ahi_hsd.h"
#include "core/resources.h"
#include "libs/bzlib_utils.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "projection/standard/proj_json.h"

extern "C"
{
    void register_MTG_FILTER();
}

namespace satdump
{
    namespace official
    {
        void AHIHsdProcessor::parse_himawari_ahi_hsd(std::vector<uint8_t> &data)
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
                bz2_result = BZ2_bzBuffToBuffDecompress_M((char *)decomp_dest + dest_offset, &dest_length, (char *)data.data() + source_offset, data.size() - source_offset, &consumed, 0, 0);
                source_offset += consumed;
                dest_offset += dest_length;
            } while (bz2_result == 0 && data.size() > source_offset);

            // Free up memory
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
                uint8_t bit_depth = decomp_dest[block_offsets[HSD_CAL_INFO] + 13]; // Is a 16-bit value, but the value is always below 16, so...

                size_t pixel_offset = num_columns * ((decomp_dest[block_offsets[HSD_SEGMENT_INFO] + 6] << 8 | decomp_dest[block_offsets[HSD_SEGMENT_INFO] + 5]) - 1);

                // Parse channel to know where to put the data
                uint16_t channel;
                memcpy(&channel, &decomp_dest[block_offsets[HSD_CAL_INFO] + 3], 2);
                channel--;
                if (channel < 0 || channel >= 16)
                {
                    logger->error("Channel out of range!");
                    delete[] decomp_dest;
                    return;
                }
                ParsedHimawariAHI &image_out = all_images[channel];

                if (!image_out.header_parsed)
                {
                    image_out.sat_name = std::string((char *)&decomp_dest[block_offsets[HSD_BASIC_INFO] + 6]);

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

        void AHIHsdProcessor::ingestFile(std::vector<uint8_t> vec) { parse_himawari_ahi_hsd(vec); }

        std::shared_ptr<satdump::products::Product> AHIHsdProcessor::getProduct()
        {
            // Saving
            std::shared_ptr<satdump::products::ImageProduct> ahi_products = std::make_shared<satdump::products::ImageProduct>();
            ahi_products->instrument_name = "ahi";

            time_t prod_timestamp = 0;
            std::string sat_name = all_images[0].sat_name;
            size_t largest_width = 0, largest_height = 0;
            double center_longitude = 0;
            ParsedHimawariAHI largestImg;

            for (auto &img : all_images)
            {
                ahi_products->images.push_back({img.channel - 1, "AHI-" + std::to_string(img.channel), std::to_string(img.channel), img.img, 16});

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

            ahi_products->set_product_timestamp(prod_timestamp);
            ahi_products->set_product_source(sat_name);

            {
                nlohmann::json proj_cfg;

                constexpr double k = 624597.0334223134;
                double scalar_x = (pow(2.0, 16.0) / double(largestImg.cfac)) * k;
                double scalar_y = (pow(2.0, 16.0) / double(largestImg.lfac)) * k;

                for (auto &img : ahi_products->images)
                    img.ch_transform = satdump::ChannelTransform().init_affine(double(largest_width) / img.image.width(), double(largest_height) / img.image.height(), 0, 0);

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
                ahi_products->set_proj_cfg(proj_cfg);
            }

            nlohmann::json calib_cfg;
            for (size_t i = 0; i < 16; i++)
            {
                int channel_id = all_images[i].channel - 1;
                calib_cfg["vars"]["scale"][channel_id] = all_images[i].calibration_scale;
                calib_cfg["vars"]["offset"][channel_id] = all_images[i].calibration_offset;
                calib_cfg["vars"]["kappa"][channel_id] = all_images[i].kappa;
            }
            calib_cfg["vars"]["spectral"] = true;
            ahi_products->set_calibration("goes_nc_abi", calib_cfg);

            for (int i = 0; i < 16; i++)
            {
                int channel_id = all_images[i].channel - 1;
                ahi_products->set_channel_wavenumber(channel_id, all_images[i].wavenumber);

                if (all_images[i].kappa > 0)
                    ahi_products->set_channel_unit(channel_id, CALIBRATION_ID_ALBEDO); // TODOREWORK reflective radiance
                else
                    ahi_products->set_channel_unit(channel_id, CALIBRATION_ID_EMISSIVE_RADIANCE);
            }

            return ahi_products;
        }
    } // namespace official
} // namespace satdump