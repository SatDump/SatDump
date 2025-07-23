#include "module_gvar_image_decoder.h"
#include "common/physics_constants.h"
#include "common/utils.h"
#include "core/config.h"
#include "core/resources.h"
#include "crc_table.h"
#include "gvar_headers.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "products/image/calibration_units.h"
#include "products/image_product.h"
#include "products/product_process.h"
#include "utils/binary.h"
#include "utils/stats.h"
#include "utils/thread_priority.h"
#include <filesystem>

#define FRAME_SIZE 32786

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace gvar
    {
        std::string GVARImageDecoderModule::getGvarFilename(int sat_number, std::tm *timeReadable, std::string channel)
        {
            std::string utc_filename = "G" + std::to_string(sat_number) + "_" + channel + "_" +                                                                     // Satellite name and channel
                                       std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
            return utc_filename;
        }

        /**
         *  Gets the triple redundant header while applying majority law
         */
        PrimaryBlockHeader get_header(uint8_t *frame)
        {

            uint8_t *result = (uint8_t *)malloc(30 * sizeof(uint8_t));

            // Every header is 30 bytes long
            uint8_t a[30];
            uint8_t b[30];
            uint8_t c[30];

            // Transmitted right after each other
            memcpy(a, frame + 8, 30);
            memcpy(b, frame + 38, 30);
            memcpy(c, frame + 68, 30);

            // First four bits are never 1, mask 'em
            a[0] &= 0xF;
            b[0] &= 0xF;
            c[0] &= 0xF;

            // Process each byte
            for (int byteIndex = 0; byteIndex < 30; ++byteIndex)
            {
                uint8_t majority = 0;

                // Process each bit in the byte
                for (int bitIndex = 0; bitIndex < 8; ++bitIndex)
                {
                    // Extract bits from each byte
                    uint8_t bit_a = (a[byteIndex] >> bitIndex) & 1;
                    uint8_t bit_b = (b[byteIndex] >> bitIndex) & 1;
                    uint8_t bit_c = (c[byteIndex] >> bitIndex) & 1;

                    // Count the number of 1s
                    int count_ones = bit_a + bit_b + bit_c;

                    // Majority rule: if at least two are 1, set the result bit
                    if (count_ones >= 2)
                    {
                        majority |= (1 << bitIndex);
                    }
                }

                // Store the majority byte
                result[byteIndex] = majority;
            }

            return *((PrimaryBlockHeader *)result);
        }

        /**
         * Gets the most common counter using majority law at the bit level from a vector of counters
         */
        uint32_t parse_counters(const std::vector<Block> &frame_buffer)
        {
            uint32_t result = 0;
            std::vector<uint32_t> counters;

            for (size_t i = 0; i < frame_buffer.size(); ++i)
            {
                counters.push_back(frame_buffer[i].original_counter & 0x7FF);
            }

            const size_t majority_threshold = (counters.size() / 2) + 1;

            for (int bit = 0; bit < 11; ++bit)
            {
                size_t count_ones = 0;
                for (size_t i = 0; i < counters.size(); ++i)
                {
                    if (counters[i] & (1 << bit))
                    {
                        ++count_ones;
                    }
                }
                if (count_ones >= majority_threshold)
                {
                    result |= (1 << bit);
                }
            }

            return result;
        }

        /**
         * Checks if the spare (Always 0) has more than 5 mismatched bits, returns true if not
         * This helps ensure we are looking at valid data and not junk
         */
        bool blockIsActualData(PrimaryBlockHeader cur_block_header)
        {
            uint32_t spare = cur_block_header.spare2;
            int errors = 0;
            for (int i = 31; i >= 0; i--)
            {
                bool markerBit, testBit;
                markerBit = satdump::getBit<uint32_t>(spare, i);
                uint32_t zero = 0;
                testBit = satdump::getBit<uint32_t>(zero, i);
                if (markerBit != testBit)
                    errors++;
            }
            // This can be adjusted to allow the spare to be more degraded than usual
            // >5 bits should be incredibly rare after bit-level majority law, at that point
            // the block ID is almost guaranteed to be incorrect messing the frame up anyways
            if (errors > 4)
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        /**
         * CRC Implementation from LRIT-Missin-Specific-Document.pdf
         */
        uint16_t computeCRC(const uint8_t *data, int size)
        {
            uint16_t crc = 0xffff;
            for (int i = 0; i < size; i++)
                crc = (crc << 8) ^ crc_table[(crc >> 8) ^ (uint16_t)data[i]];
            return crc;
        }
        uint8_t computeXOR(const uint8_t *data, int size)
        {
            uint8_t crc = 0;
            for (int i = 0; i < size; i++)
                crc = crc ^ data[i];
            return crc;
        }

        void GVARImageDecoderModule::saveReceivedImage(time_t &image_time)
        {
            // Full disk size. Default
            int img_off_x = -1;
            int img_off_y = -1;
            int img_size_x = 20836; // 20944
            int img_size_y = 18956;
            float subsat_lon = 61.5;

            if (isImageInProgress)
            {
                // Checks if time is set
                if (image_time != 0)
                {
                    logger->debug("Using time from imagery frames");
                }
                // No image block headers passed CRC!
                else
                {
                    // Did we get any valid Block 0 headers we can use instead?
                    if (block_zero_timestamps.size() > 0)
                    {
                        logger->debug("Using time from satellite info block");
                        image_time = satdump::most_common(block_zero_timestamps.begin(), block_zero_timestamps.end(), 0);
                    }
                    // There were no valid Block 0s received, use fallback
                    else
                    {
                        logger->warn("Signal was too weak to get the timestamp! Using current system time instead");
                        image_time = image_time_fallback;
                    }
                }

                if (block_zero_x_size.size() > 0)
                {
                    logger->debug("Using image size from satellite info block");

                    img_size_x = satdump::most_common(block_zero_x_size.begin(), block_zero_x_size.end(), 0);
                    img_size_y = satdump::most_common(block_zero_y_size.begin(), block_zero_y_size.end(), 0);

                    img_off_x = satdump::most_common(block_zero_x_offset.begin(), block_zero_x_offset.end(), -1);
                    img_off_y = satdump::most_common(block_zero_y_offset.begin(), block_zero_y_offset.end(), -1);

                    subsat_lon = satdump::most_common(block_zero_subsat_lon.begin(), block_zero_subsat_lon.end(), 0) / 1000.0;
                }
                else
                {
                    logger->warn("Signal was too weak to get imagery size/offset info! Using FD defaults instead!");
                }

                // Store new fallback time. Image start was detected NOW, so save NOW to use when saving this image.
                image_time_fallback = time(0);

                if (writeImagesAync)
                {
                    logger->debug("Saving Async...");
                    isImageInProgress = false;
                    isSavingInProgress = true;
                    imageVectorMutex.lock();
                    imagesVector.push_back({infraredImageReader1.getImage1(), infraredImageReader1.getImage2(), infraredImageReader2.getImage1(), infraredImageReader2.getImage2(),
                                            visibleImageReader.getImage(), satdump::most_common(scid_stats.begin(), scid_stats.end(), 0), img_size_x, img_size_y, image_time, img_off_x, img_off_y,
                                            subsat_lon});
                    imageVectorMutex.unlock();
                    isSavingInProgress = false;
                }
                else
                {
                    logger->debug("Saving...");
                    isImageInProgress = false;
                    isSavingInProgress = true;
                    GVARImages images = {infraredImageReader1.getImage1(),
                                         infraredImageReader1.getImage2(),
                                         infraredImageReader2.getImage1(),
                                         infraredImageReader2.getImage2(),
                                         visibleImageReader.getImage(),
                                         satdump::most_common(scid_stats.begin(), scid_stats.end(), 0),
                                         img_size_x, //   most_common(vis_width_stats.begin(), vis_width_stats.end(), 0),
                                         img_size_y,
                                         image_time,
                                         img_off_x,
                                         img_off_y,
                                         subsat_lon};
                    writeImages(images, directory);
                    isSavingInProgress = false;
                }

                // Resets image times
                image_time = 0;
                block_zero_timestamps.clear();

                block_zero_x_offset.clear();
                block_zero_y_offset.clear();
                block_zero_x_size.clear();
                block_zero_y_size.clear();
                block_zero_subsat_lon.clear();

                scid_stats.clear();
                ir_width_stats.clear();

                // Reset readers
                infraredImageReader1.startNewFullDisk();
                infraredImageReader2.startNewFullDisk();
                visibleImageReader.startNewFullDisk();
            }
        }

        /**
         * Decodes imagery from the frame buffer while attempting block ID recovery and counter correction
         */
        void GVARImageDecoderModule::process_frame_buffer(std::vector<Block> &frame_buffer, time_t &image_time)
        {
            // The most common counter from this series
            uint32_t final_counter = parse_counters(frame_buffer);

            // Are we decoding a new image?
            // If this series' counter is 1, or 1 wasn't decoded and we are at 2, save the current image
            if (final_counter == 1 || (imageFrameCount > 2 && final_counter == 2))
            {
                logger->info("Image start detected!");

                imageFrameCount = 0;

                saveReceivedImage(image_time);
            }
            else
            {
                imageFrameCount++;

                // Minimum of 10 block series to write (80 VIS pixels)
                if (imageFrameCount > 10 && !isImageInProgress)
                    isImageInProgress = true;
            }

            // Processes all frames
            size_t fsize = frame_buffer.size();
            for (size_t i = 0; i < fsize; i++)
            {
                uint8_t *current_frame = frame_buffer[i].frame;
                PrimaryBlockHeader cur_block_header = get_header(current_frame);

                // Tries to recover the block ID in case it is damaged within a series
                // Junk will never pass through this, as those frames are thrown out before
                // being added to the frame buffer
                if (i > 1 && i < (fsize - 1))
                {
                    uint32_t prev_counter = frame_buffer[i - 1].block_id;
                    uint32_t next_counter = frame_buffer[i + 1].block_id;

                    if (next_counter - prev_counter == 2)
                    {
                        // The previous and next blocks suggest we should be inbetween them
                        cur_block_header.block_id = (frame_buffer[i - 1].block_id) + 1;
                    }
                }

                // Is this imagery? Blocks 1 to 10 are imagery
                if (cur_block_header.block_id >= 1 && cur_block_header.block_id <= 10)
                {
                    // This is imagery, so we can parse the line information header
                    LineDocumentationHeader line_header(&current_frame[8 + 30 * 3]);

                    // SCID Stats
                    scid_stats.push_back(line_header.sc_id);

                    // Ensures the final counter correction didn't break, if it did, don't use it
                    // (This should NEVER be 0)
                    if (final_counter == 0)
                    {
                        // Masks first five bits, because they are NEVER 1 (Max = 1974 = 0b0000011110110110)
                        final_counter = (line_header.relative_scan_count &= 0x7ff);
                    }

                    // Internal line counter should NEVER be over 1974. Discard frame if it is
                    // Though we know the max in normal operations is 1354 for a full disk, so we use that instead
                    if (final_counter > 1354)
                        continue;

                    // Is this VIS Channel 1?
                    if (cur_block_header.block_id >= 3 && cur_block_header.block_id <= 10)
                    {
                        // Push width stats
                        // vis_width_stats.push_back(line_header.pixel_count);

                        // Push into decoder
                        visibleImageReader.pushFrame(current_frame, cur_block_header.block_id, final_counter);
                    }
                    // Is this IR?
                    else if (cur_block_header.block_id == 1 || cur_block_header.block_id == 2)
                    {
                        // Easy way of showing an approximate progress percentage
                        approx_progess = round(((float)final_counter / 1353.0f) * 1000.0f) / 10.0f;

                        // Push frame size stats, masks first 3 bits because they are NEVER 1
                        // Max = 6565 = 0b0001100110100101
                        ir_width_stats.push_back(line_header.word_count &= 0x1fff);

                        // Get current stats
                        int current_words = satdump::most_common(ir_width_stats.begin(), ir_width_stats.end(), 0);

                        // Safeguard
                        if (current_words > 6565)
                            current_words = 6530; // Default to fulldisk size

                        // Is this IR Channel 1-2?
                        if (cur_block_header.block_id == 1)
                            infraredImageReader1.pushFrame(&current_frame[8 + 30 * 3], final_counter, current_words);
                        else if (cur_block_header.block_id == 2)
                            infraredImageReader2.pushFrame(&current_frame[8 + 30 * 3], final_counter, current_words);
                    }
                }

                delete[] current_frame;
            }

            // Clear buffer out so new frames can be appended
            frame_buffer.clear();
        }

        /**
         * Writes the sounder imagery
         * The image_time has to be passed as there is no Sounder object that gets created
         * that could have it saved instead, the scan time *should* be the same
         */
        void GVARImageDecoderModule::writeSounder(time_t image_time)
        {
            std::tm *timeReadable = gmtime(&image_time);
            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));

            logger->info("New sounder scan starting, saving at " + timestamp + "...");

            std::filesystem::create_directories(directory_snd + "/" + timestamp);
            std::string directory_d = directory_snd + "/" + timestamp;

            for (int i = 0; i < 19; i++)
            {
                auto img = sounderReader.getImage(i);
                image::save_img(img, directory_d + "/Sounder_" + std::to_string(i + 1));
            }

            sounderReader.clear();
        }

        // Load up calibration LUT
        std::array<std::array<float, 1024>, 4> readLUTValues(std::ifstream LUT)
        {
            std::array<std::array<float, 1024>, 4> values;
            std::string tmp;
            // skip first 7 lines
            for (int i = 0; i < 8; i++)
            {
                std::getline(LUT, tmp);
            }
            // read LUTs
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 1024; j++)
                {
                    std::getline(LUT, tmp);
                    values[i][j] = std::stof(tmp.substr(18, 7));
                }
                if (i != 3)
                {
                    // skip det2 for first 3 channels (no det2 for ch6)
                    for (int j = 0; j < 1030; j++)
                        std::getline(LUT, tmp);
                }
            }
            return values;
        }

        void GVARImageDecoderModule::writeImages(GVARImages &images, std::string directory)
        {
            const time_t timevalue = images.image_time;
            // Copies the gmtime result since it gets modified elsewhere
            std::tm timeReadable = *gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable.tm_year + 1900) + "-" +
                                    (timeReadable.tm_mon + 1 > 9 ? std::to_string(timeReadable.tm_mon + 1) : "0" + std::to_string(timeReadable.tm_mon + 1)) + "-" +
                                    (timeReadable.tm_mday > 9 ? std::to_string(timeReadable.tm_mday) : "0" + std::to_string(timeReadable.tm_mday)) + "_" +
                                    (timeReadable.tm_hour > 9 ? std::to_string(timeReadable.tm_hour) : "0" + std::to_string(timeReadable.tm_hour)) + "-" +
                                    (timeReadable.tm_min > 9 ? std::to_string(timeReadable.tm_min) : "0" + std::to_string(timeReadable.tm_min));

            // Get stats. This is done over a lot of data to allow decoding at low SNR
            int sat_number = images.sat_number;
            int vis_width = images.vis_width;
            int vis_height = images.vis_height;

            // Avoid imagery bugs
            if (images.image5.width() < vis_width)
                vis_width = images.image5.width();
            if (images.image5.height() < vis_height)
                vis_height = images.image5.height();

            std::string dir_name = "GOES-" + std::to_string(sat_number) + "/" + timestamp;
            logger->info("Full disk finished, saving at " + dir_name + "...");

            std::string disk_folder = directory + "/" + dir_name;
            std::filesystem::create_directories(disk_folder);

            logger->info("Resizing...");
            images.image1.resize(images.image1.width(), images.image1.height() * 1.75);
            images.image2.resize(images.image2.width(), images.image2.height() * 1.75);
            images.image3.resize(images.image3.width(), images.image3.height() * 1.75);
            images.image4.resize(images.image4.width(), images.image4.height() * 1.75);
            images.image5.resize(images.image5.width(), images.image5.height() * 1.75);

            // logger->trace("VIS 1 size before " + std::to_string(images.image5.width()) + "x" + std::to_string(images.image5.height()));
            // logger->trace("IR size before " + std::to_string(images.image1.width()) + "x" + std::to_string(images.image1.height()));

            // VIS-1 height
            vis_height *= 1.75;

            // IR height
            int ir1_width = vis_width / 4;
            int ir1_height = vis_height / 4;

            logger->info("Cropping to transmited size...");
            logger->debug("VIS 1 size " + std::to_string(vis_width) + "x" + std::to_string(vis_height));
            images.image5.crop(0, 0, vis_width, vis_height);
            logger->debug("IR size " + std::to_string(ir1_width) + "x" + std::to_string(ir1_height));
            images.image1.crop(0, 0, ir1_width, ir1_height);
            images.image2.crop(0, 0, ir1_width, ir1_height);
            images.image3.crop(0, 0, ir1_width, ir1_height);
            images.image4.crop(0, 0, ir1_width, ir1_height);

            // Save product
            satdump::products::ImageProduct imager_product;
            imager_product.instrument_name = "goesn_imager";
            imager_product.set_product_source("GOES-" + std::to_string(sat_number));
            imager_product.set_product_timestamp(images.image_time);

            // Raw Images
            imager_product.images.push_back({0, getGvarFilename(sat_number, &timeReadable, "1"), "1", images.image5, 10, satdump::ChannelTransform().init_none()});
            imager_product.images.push_back({1, getGvarFilename(sat_number, &timeReadable, "2"), "2", images.image1, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
            imager_product.images.push_back({2, getGvarFilename(sat_number, &timeReadable, "3"), "3", images.image2, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
            imager_product.images.push_back({3, getGvarFilename(sat_number, &timeReadable, "4"), "4", images.image3, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
            imager_product.images.push_back({4, getGvarFilename(sat_number, &timeReadable, "5"), "5", images.image4, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});

            // Projection
            if (images.vis_xoff != -1 && images.vis_yoff != -1)
            {
                nlohmann::json proj_cfg;

                double final_offset_x = 9496.0 - (images.vis_xoff - 5789);
                double final_offset_y = 9508.0 - (images.vis_yoff - 2485) * 1.75;

                // It must just be special?
                if (images.sat_number == 13)
                {
                    final_offset_x += 69.6943231441048;
                    final_offset_y += 249.7816593886463;
                }

                double scalar_x = 2290 / 4.;
                double scalar_y = 2290 / 4.;
                proj_cfg["type"] = "geos";
                proj_cfg["lon0"] = images.subsatlon;
                proj_cfg["sweep_x"] = true;
                proj_cfg["scalar_x"] = scalar_x;
                proj_cfg["scalar_y"] = -scalar_y;
                proj_cfg["offset_x"] = final_offset_x * -scalar_x;
                proj_cfg["offset_y"] = final_offset_y * scalar_y;
                proj_cfg["width"] = images.vis_width;   // img.width();
                proj_cfg["height"] = images.vis_height; // img.height();
                proj_cfg["altitude"] = 35786023.00;

                imager_product.set_proj_cfg(proj_cfg);
            }
            else
            {
                logger->warn("Cannot add projection info! SNR too low to parse them?");
            }

            // Calibration
            imager_product.set_channel_wavenumber(0, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.65e-6));
            imager_product.set_channel_wavenumber(1, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 3.90e-6));
            imager_product.set_channel_wavenumber(2, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 6.55e-6));
            imager_product.set_channel_wavenumber(3, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 10.7e-6));
            imager_product.set_channel_wavenumber(4, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 13.35e-6));

            std::string filename = "lut/goes/gvar/goes" + std::to_string(images.sat_number) + "_gvar_lut.txt";
            if (resources::resourceExists(filename))
            {
                imager_product.set_channel_unit(0, CALIBRATION_ID_ALBEDO);
                imager_product.set_channel_unit(1, CALIBRATION_ID_EMISSIVE_RADIANCE);
                imager_product.set_channel_unit(2, CALIBRATION_ID_EMISSIVE_RADIANCE);
                imager_product.set_channel_unit(3, CALIBRATION_ID_EMISSIVE_RADIANCE);
                imager_product.set_channel_unit(4, CALIBRATION_ID_EMISSIVE_RADIANCE);

                std::array<std::array<float, 1024>, 4> LUTs = readLUTValues(std::ifstream(resources::getResourcePath(filename)));
                nlohmann::json calib_cfg;
                calib_cfg["ch2_lut"] = LUTs[0];
                calib_cfg["ch3_lut"] = LUTs[1];
                calib_cfg["ch4_lut"] = LUTs[2];
                calib_cfg["ch5_lut"] = LUTs[3];
                imager_product.set_calibration("gvar_imager", calib_cfg);
            }
            else
            {
                logger->warn("Resource " + filename + " doesn't exist. Can't calibrate!");
            }

            // Save
            imager_product.save(disk_folder);

            // Generate composites, if enabled
            if (satdump::satdump_cfg.shouldAutoprocessProducts()) // TODOREWORK. Per pipeline?
                satdump::products::process_product_with_handler(&imager_product, disk_folder);
        }

        GVARImageDecoderModule::GVARImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            frame = new uint8_t[FRAME_SIZE];
            isImageInProgress = false;
            isSavingInProgress = false;
            approx_progess = 0;

            // Reset readers
            infraredImageReader1.startNewFullDisk();
            infraredImageReader2.startNewFullDisk();
            visibleImageReader.startNewFullDisk();

            imageFrameCount = 0;

            fsfsm_enable_output = false;
        }

        GVARImageDecoderModule::~GVARImageDecoderModule()
        {
            delete[] frame;

            if (textureID != 0)
            {
                delete[] textureBuffer;
                // deleteImageTexture(textureID);
            }
        }

        void GVARImageDecoderModule::writeImagesThread()
        {
            logger->info("Started saving thread...");
            while (writeImagesAync)
            {
                imageVectorMutex.lock();
                int imagesCount = imagesVector.size();
                if (imagesCount > 0)
                {
                    writeImages(imagesVector[0], directory);
                    imagesVector.erase(imagesVector.begin(), imagesVector.begin() + 1);
                }
                imageVectorMutex.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        void GVARImageDecoderModule::process()
        {
            if (input_data_type != satdump::pipeline::DATA_FILE)
                writeImagesAync = true;

            // Start thread
            if (writeImagesAync)
            {
                imageSavingThread = std::thread(&GVARImageDecoderModule::writeImagesThread, this);
                satdump::setLowestThreadPriority(imageSavingThread); // Low priority to avoid sampledrop
                // TODOREWORK namespace remove
            }

            directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGE";
            directory_snd = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SOUNDER";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            if (!std::filesystem::exists(directory_snd))
                std::filesystem::create_directory(directory_snd);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t last_frame_time = 0;
            time_t image_time = 0;
            // Time when processing started, used as a backup if no other time is decoded
            image_time_fallback = time(0);

            // Used to check if the block header's time is valid
            bool crc_valid = false;
            int last_sounder_pos = 0;

            // Series of consecutive imagery blocks are saved here
            std::vector<Block> frame_buffer;

            while (should_run())
            {
                // Read a buffer
                read_data((uint8_t *)frame, FRAME_SIZE);

                PrimaryBlockHeader block_header = get_header(frame);

                // Gets the time from the header if it passes a CRC check
                block_header.header_crc = ~block_header.header_crc;
                crc_valid = (computeCRC((uint8_t *)&block_header, 30) == 0);

                // If this header passed CRC but had no image date set, we use the time from the header.
                // This is a fallback in case no Block 0's are found.
                if (crc_valid && image_time == 0)
                {
                    tm time = block_header.time_code_bcd;
                    image_time = timegm(&time); // + time.tm_gmtoff;
                }

                // Ensures we aren't looking at junk
                if (!blockIsActualData(block_header))
                {
                    continue;
                }

                // Sounder doesn't need special handling
                if (block_header.block_id == 11)
                {
                    uint8_t *sad_header = &frame[8 + 30 * 3];
                    int dataid = sad_header[2];

                    if (dataid == 35)
                    {
                        int x_pos = frame[6120] << 8 | frame[6121];
                        if (sad_header[3])
                        {
                            if (x_pos != last_sounder_pos)
                                writeSounder(image_time); // Image time has to be passed manually, see func docstring
                            last_sounder_pos = x_pos;
                        }
                        sounderReader.pushFrame(frame, 0);
                    }
                }
                else if (block_header.block_id == 0)
                {
                    Block0Header block_header0 = *((Block0Header *)&frame[8 + 30 * 3]);

                    // Basic XOR passed. Will only detect single bit errors. Not foolproof.
                    if (computeXOR((uint8_t *)&block_header0, 278) == 0xff)
                    {
                        tm block0_current_time = block_header0.TCURR;
                        tm block0_image_time = block_header0.CIFST;
                        time_t current_time = timegm(&block0_current_time);
                        time_t image_time = timegm(&block0_image_time);
                        float time_diff = difftime(current_time, image_time);

                        // Image start time and current header time is within one hour, set image time.
                        // Just a sanity check.
                        // If this is valid, also try to parse projection/frame info!
                        if (time_diff < 3600 && time_diff > -60)
                        {
                            block_zero_timestamps.push_back(timegm(&block0_image_time) /*+ block0_image_time.tm_gmtoff*/);

                            int x_off = std::min<int>(block_header0.IWFPX, block_header0.IEFPX);
                            int y_off = std::min<int>(block_header0.INFLN, block_header0.ISFLN);
                            int x_size = abs(block_header0.IWFPX - block_header0.IEFPX);
                            int y_size = abs(block_header0.INFLN - block_header0.ISFLN);
                            float subsatlon = block_header0.SUBLO;

                            // logger->critical("X Offset %d , Y Offset %d, X Size %d, Y Size %d, LON %f", x_off, y_off, x_size, y_size, subsatlon);

                            block_zero_x_offset.push_back(x_off);
                            block_zero_y_offset.push_back(y_off);

                            block_zero_x_size.push_back(x_size);
                            block_zero_y_size.push_back(y_size);

                            block_zero_subsat_lon.push_back(round(subsatlon * 1000));
                        }
                    }
                }

                // Are we ready to process our frame buffer?
                // Processes if we already have a series saved (10 frames are inside the buffer)
                // OR if the last frame we looked at was 10 (the last one)
                if (frame_buffer.size() > 9 || (!frame_buffer.empty() && frame_buffer.back().block_id == 10))
                {
                    process_frame_buffer(frame_buffer, image_time);
                }

                // Saves imagery blocks for processing
                if (block_header.block_id >= 1 && block_header.block_id <= 10)
                {
                    LineDocumentationHeader line_header(&frame[8 + 30 * 3]);

                    Block current_block;
                    current_block.block_id = block_header.block_id;
                    current_block.original_counter = line_header.relative_scan_count;
                    current_block.frame = new uint8_t[FRAME_SIZE];
                    std::memcpy(current_block.frame, frame, FRAME_SIZE);

                    frame_buffer.push_back(current_block);
                }
            }

            // Ensures we don't leave frames behind
            if (!frame_buffer.empty())
            {
                process_frame_buffer(frame_buffer, image_time);
            }

            cleanup();

            // TODO: Maybe don't write the sounder if it's empty?

            // Image time has to be passed manually, see func docstring
            if (sounderReader.frames > 2)
                writeSounder(image_time);

            if (writeImagesAync)
            {
                logger->info("Exit async thread...");
                writeImagesAync = false;
                if (imageSavingThread.joinable())
                    imageSavingThread.join();
            }

            logger->info("Dump remaining data...");

            if (isImageInProgress)
                saveReceivedImage(image_time);
        }

        nlohmann::json GVARImageDecoderModule::getModuleStats()
        {
            auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
            v["full_disk_progress"] = approx_progess;
            return v;
        }

        void GVARImageDecoderModule::drawUI(bool window)
        {
            if (textureID == 0)
            {
                textureID = makeImageTexture();
                textureBuffer = new uint32_t[1354 * 2 * 5236];
                memset(textureBuffer, 0, sizeof(uint32_t) * 1354 * 2 * 5236);
            }

            ImGui::Begin("GVAR Image Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            // This is outer crap...
            ImGui::BeginGroup();
            {
                ushort_to_rgba(infraredImageReader1.imageBuffer1, textureBuffer, 5236 * 1354 * 2);
                updateImageTexture(textureID, textureBuffer, 5236, 1354 * 2);
                ImGui::Image((void *)(intptr_t)textureID, {200 * ui_scale, 200 * ui_scale});
            }
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            {
                ImGui::Button("Full Disk Progress", {200 * ui_scale, 20 * ui_scale});
                ImGui::ProgressBar((float)approx_progess / 100.0f, ImVec2(200 * ui_scale, 20 * ui_scale));
                ImGui::Text("State : ");
                ImGui::SameLine();
                if (isSavingInProgress)
                    ImGui::TextColored(style::theme.green, "Writing images...");
                else if (isImageInProgress)
                    ImGui::TextColored(style::theme.orange, "Receiving...");
                else
                    ImGui::TextColored(style::theme.red, "IDLE");
            }
            ImGui::EndGroup();

            drawProgressBar();

            ImGui::End();
        }

        std::string GVARImageDecoderModule::getID() { return "goes_gvar_image_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GVARImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GVARImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace gvar
} // namespace goes