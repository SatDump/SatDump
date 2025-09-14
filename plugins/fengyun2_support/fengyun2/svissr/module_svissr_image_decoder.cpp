#include "module_svissr_image_decoder.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "common/majority_law.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "products/image/calibration_units.h"
#include "products/image_product.h"
#include "utils/stats.h"

#define FRAME_SIZE 44356 /* Standard S-VISSR frame in bytes */

// Sector ID (2) + S/C & CDAS block (126) + Constants (64) + Subcom ID (4)
#define SUBCOM_START_OFFSET 196 /* Offset from start of frame to the Subcom section*/

// Simplified mapping (100) + Orbit & attitude (128) + MANAM (410) + Calib (256+1024) + Spare (179)
#define SUBCOM_GROUP_SIZE 2097 /* Size of the repeated (Subcommunication) section of the documentation sector */

std::map<fengyun_svissr::SVISSRSubCommunicaitonBlockType, fengyun_svissr::SVISSRSubcommunicationBlock> subcom_blocks = {
    {fengyun_svissr::Simplified_mapping, {0, 99}}, {fengyun_svissr::Orbit_and_attitude, {100, 227}}, {fengyun_svissr::MANAM, {228, 637}},
    {fengyun_svissr::Calibration_1, {638, 893}},   {fengyun_svissr::Calibration_2, {894, 1917}},     {fengyun_svissr::Spare, {1918, 2096}}};

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace fengyun_svissr
{
    SVISSRImageDecoderModule::SVISSRImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        frame = new uint8_t[FRAME_SIZE * 2];

        // Counter related vars
        counter_locked = false;
        global_counter = 0;
        apply_correction = parameters.contains("apply_correction") ? parameters["apply_correction"].get<bool>() : false;
        backwardScan = false;

        fsfsm_enable_output = false;

        vissrImageReader.reset();
    }

    SVISSRImageDecoderModule::~SVISSRImageDecoderModule()
    {
        delete[] frame;

        if (textureID != 0)
        {
            delete[] textureBuffer;
            // deleteImageTexture(textureID);
        }

        subcommunication_frames.clear();
        current_subcom_frame.clear();
        group_retransmissions.clear();
    }

    std::string SVISSRImageDecoderModule::getSvissrFilename(std::tm *timeReadable, std::string channel)
    {
        std::string utc_filename = sat_name + "_" + channel + "_" +                                                                                             // Satellite name and channel
                                   std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                   (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                   (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                   (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                   (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                   (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
        return utc_filename;
    }

    /**
     * Extracts a specific block from the subcommunication minor frame
     * @param subcom_frame The minor frame to get the block from
     * @param block_type The SVISSRSubCommunicaitonBlockType to get
     */
    std::vector<uint8_t> get_subcom_block(MinorFrame subcom_frame, SVISSRSubCommunicaitonBlockType block_type)
    {
        // Subcommunication frames are ALWAYS the same size, if we don't have the size, something went very wrong!
        if (subcom_frame.size() != SUBCOM_GROUP_SIZE * 25)
        {
            logger->critical("SUBCOM FRAME SIZE IS NOT CORRECT!!! Was a group missed? ABORTING!");
            abort();
        }

        std::vector<uint8_t> output;

        // Data is transmitted in 25 groups, all of which have a piece of every block type
        for (int group = 0; group < 25; group++)
        {
            for (int byte = subcom_blocks[block_type].start_offset; byte <= subcom_blocks[block_type].end_offset; byte++)
            {
                output.push_back(subcom_frame[group * SUBCOM_GROUP_SIZE + byte]);
            }
        }

        return output;
    };

    /**
     * Countrs the amount of errors in a subcommunication frame's 179-byte spare. Helps us ensure we are not looking at junk.
     * This works because it is supposed to be all zeroes. Helpful for determining SNR (ruoughly)
     * Max errors: 35800 (theory, 179*25*8), 17900 (realistic for BPSK, 50% chance)
     *
     *  @param subcom_Frame The subcommunication frame to check the spare of
     */
    int get_spare_errors(MinorFrame subcom_frame)
    {
        std::vector<uint8_t> spare = get_subcom_block(subcom_frame, Spare);
        int errors = 0;

        // The spare is 4475 bytes long (179 * 25 groups)
        for (int byte = 0; byte < 4475; byte++)
        {
            uint8_t cur_byte = spare[byte];
            for (int bit = 0; bit < 8; bit++)
            {
                if (((cur_byte >> bit) & 0x1) == 1)
                {
                    // All bits should be zero
                    errors++;
                }
            }
        }

        return errors;
    }

    void SVISSRImageDecoderModule::save_minor_frame()
    {
        if (!group_retransmissions.empty() && current_subcom_frame.size() / SUBCOM_GROUP_SIZE == 24)
        {

            // Process the last group
            Group last_group = majority_law(group_retransmissions, false);
            group_retransmissions.clear();

            // Add group to the frame
            current_subcom_frame.insert(current_subcom_frame.end(), last_group.begin(), last_group.end());

            // Save the frame
            logger->debug("Saved a subcom frame!");
            subcommunication_frames.push_back(current_subcom_frame);
            current_subcom_frame.clear();
        }
        else if (group_retransmissions.empty())
        {
            logger->debug("Failed to get the last group!");
            current_subcom_frame.clear();
        }
        else
        {
            logger->debug("Minor frame size is not valid, skipping! %d bytes, %f groups", current_subcom_frame.size(), current_subcom_frame.size() / SUBCOM_GROUP_SIZE);
            current_subcom_frame.clear();
        }
    }

    void SVISSRImageDecoderModule::writeImages(SVISSRBuffer &buffer)
    {
        writingImage = true;
        // Save products
        satdump::products::ImageProduct svissr_product;
        svissr_product.instrument_name = "fy2-svissr";
        double unix_timestamp = 0;

        switch (buffer.scid)
        {
        case (40):
            sat_name = "FY-2H";
            svissr_product.set_product_source("FengYun-2H");
            break;
        case (39):
            sat_name = "FY-2G";
            svissr_product.set_product_source("FengYun-2G");
            break;
        // Below ones are inferred due to the lack of sample SVISSR data
        case (38):
            sat_name = "FY-2F";
            svissr_product.set_product_source("FengYun-2F");
            break;
        case (37):
            sat_name = "FY-2E";
            svissr_product.set_product_source("FengYun-2E");
            break;
        case (36):
            sat_name = "FY-2D";
            svissr_product.set_product_source("FengYun-2D");
            break;
        case (35):
            sat_name = "FY-2C";
            svissr_product.set_product_source("FengYun-2C");
            break;
        case (34):
            sat_name = "FY-2B";
            svissr_product.set_product_source("FengYun-2B");
            break;
        case (33):
            sat_name = "FY-2A";
            svissr_product.set_product_source("FengYun-2A");
            break;
        default:
            sat_name = "Unknown FY-2";
            svissr_product.set_product_source("Unknown FengYun-2");
        }

        // -> SUBCOMMUNICATION BLOCK HANDLING <-

        MinorFrame final_subcom_frame; /* Result of majority law between all subcommunication frames */

        std::array<std::array<float, 1024>, 5> LUTs; /* Calibration LUTs */
        bool process_subcom_data = false;

        // Ensures we can and should decode it in the first place
        if (!subcommunication_frames.empty())
        {
            logger->debug("Pulled %d subcommunication frames", subcommunication_frames.size());
            final_subcom_frame = majority_law(subcommunication_frames, false);

            // - Integrity check -
            int errors = get_spare_errors(final_subcom_frame);
            logger->debug("Final subcom data errors: %d/35800", errors);

            // 32 decided arbitrarily - we are good if less than 32 out of 35800 bits are flipped
            if (errors < 32)
            {
                process_subcom_data = true;
            }
        }

        if (process_subcom_data)
        {
            // ----> Orbit & Attitude block <----

            std::vector<uint8_t> orbit_attitude_block = get_subcom_block(final_subcom_frame, Orbit_and_attitude);

            // - Timestamp handling -

            // R6*8 -> Big endian 6 byte integer, needs 10^-8 to read the value
            uint64_t raw_timestamp = ((uint64_t)orbit_attitude_block[0] << 40 | (uint64_t)orbit_attitude_block[1] << 32 | (uint64_t)orbit_attitude_block[2] << 24 |
                                      (uint64_t)orbit_attitude_block[3] << 16 | (uint64_t)orbit_attitude_block[4] << 8 | (uint64_t)orbit_attitude_block[5]);

            unix_timestamp = ((raw_timestamp * 1e-8) - 40587) * 86400;

            // ----> Simplified mapping (GCP) <----

            // TODO: GCP loading not implemented yet
            /*
            std::vector<uint8_t> simplified_mapping = get_subcom_block(minor_frame, Simplified_mapping);
            nlohmann::json proj_cfg;

            std::vector<satdump::projection::GCP> raw; // debug
            nlohmann::json gcps;
            int gcp_count = 0;

            for (int index = 0, longitude = 45, latitude = 60; index < simplified_mapping.size(); index += 4)
            {
                // LONGITUDE 45°E through 165°E
                // LATITUDE 60°N through 60°S

                uint16_t x_offset = (uint16_t)simplified_mapping[index] << 8 | (uint16_t)simplified_mapping[index + 1];
                uint16_t y_offset = (uint16_t)simplified_mapping[index + 2] << 8 | (uint16_t)simplified_mapping[index + 3];

                gcps[gcp_count]["x"] = static_cast<double>(x_offset);
                gcps[gcp_count]["y"] = static_cast<double>(y_offset);
                gcps[gcp_count]["lat"] = static_cast<double>(latitude);
                gcps[gcp_count]["lon"] = static_cast<double>(longitude);
                gcp_count++;

                // just for debugging
                raw.push_back({static_cast<double>(x_offset), static_cast<double>(y_offset), static_cast<double>(longitude), static_cast<double>(latitude)});

                // End of a line
                if (longitude == 165)
                {
                    latitude -= 5;
                    longitude = 45;
                }
                else
                {
                    longitude += 5;
                }
            }
            proj_cfg["type"] = "gcps_timestamps_line";
            proj_cfg["gcp_cnt"] = gcp_count;
            proj_cfg["gcps"] = gcps;
            proj_cfg["gcp_spacing_x"] = 200;
            proj_cfg["gcp_spacing_y"] = 200;


            // SHIM

            std::vector<double> timestamps;
            timestamps.insert(timestamps.end(), buffer.image1.height(), unix_timestamp);
            proj_cfg["timestamps"] = timestamps;
            svissr_product.set_proj_cfg(proj_cfg);
            */

            // ----> MANAM <----

            std::vector<uint8_t> manam_data = get_subcom_block(final_subcom_frame, MANAM);

            std::string manam_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MANAM";

            if (!std::filesystem::exists(manam_directory))
                std::filesystem::create_directory(manam_directory);

            const time_t timevalue = unix_timestamp;
            std::tm timeReadable = *gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable.tm_year + 1900) + "-" +
                                    (timeReadable.tm_mon + 1 > 9 ? std::to_string(timeReadable.tm_mon + 1) : "0" + std::to_string(timeReadable.tm_mon + 1)) + "-" +
                                    (timeReadable.tm_mday > 9 ? std::to_string(timeReadable.tm_mday) : "0" + std::to_string(timeReadable.tm_mday)) + "_" +
                                    (timeReadable.tm_hour > 9 ? std::to_string(timeReadable.tm_hour) : "0" + std::to_string(timeReadable.tm_hour)) + "-" +
                                    (timeReadable.tm_min > 9 ? std::to_string(timeReadable.tm_min) : "0" + std::to_string(timeReadable.tm_min));

            std::string manam_path = manam_directory + "/MANAM_" + timestamp + ".txt";
            std::ofstream outfile(manam_path, std::ios::out | std::ios::binary);

            // Writes MANAM
            outfile.write(reinterpret_cast<const char *>(&manam_data[0]), 10250);
            outfile.close();

            // Calibration 1 has the same data, just a lower resolution for IR - no point in getting it
            // ----> Calibration 2 <----

            std::vector<uint8_t> Calib_2_block = get_subcom_block(final_subcom_frame, Calibration_2);

            // - VIS calibration -

            for (int byte_index = 256; byte_index < 512; byte_index += 4)
            {
                uint32_t raw_value = ((uint32_t)Calib_2_block[byte_index] << 24 | (uint32_t)Calib_2_block[byte_index + 1] << 16 | (uint32_t)Calib_2_block[byte_index + 2] << 8 |
                                      (uint32_t)Calib_2_block[byte_index + 3]);

                float value = raw_value * 1e-8;
                std::string strvalue = "word: " + std::to_string(byte_index) + " value: " + std::to_string(value) + "\n";

                // 64 values, we interpolate 15 between them to get values for all 1024 counts
                int lut_index = ((byte_index - 256) / 4) * 16;
                LUTs[0][lut_index] = value;

                // No interpolation for the first value
                if (lut_index == 0)
                    continue;

                float last_value = LUTs[0][lut_index - 16];
                float diff = value - last_value;

                for (int interpolation_count = 1; interpolation_count < 16; interpolation_count++)
                {
                    // Interpolates the previous 15 values
                    LUTs[0][lut_index - 16 + interpolation_count] = last_value + (diff * interpolation_count / 15.0f);
                }
            }

            // Invalidate first 16 counts - It would refer to 0 albedo (true white) which is impossible to reach
            for (int i = 0; i < 16; i++){
                LUTs[0][i] = 0;
            } 
            // Same for the last 16 counts - just with true black
            for (int i = 63*16; i < 64*16; i++){
                LUTs[0][i] = 0;
            } 

            // - IR calibration -

            // Starts at zero to account for offsets more nicely
            for (int ir_channel = 0; ir_channel < 4; ir_channel++)
            {
                int start_byte = 1280 + 4096 * ir_channel;

                for (int byte_index = start_byte; byte_index < start_byte + 4096; byte_index += 4)
                {
                    uint32_t raw_value = ((uint32_t)Calib_2_block[byte_index] << 24 | (uint32_t)Calib_2_block[byte_index + 1] << 16 | (uint32_t)Calib_2_block[byte_index + 2] << 8 |
                                          (uint32_t)Calib_2_block[byte_index + 3]);

                    float value = raw_value * 1e-3;

                    int lut_index = (byte_index - start_byte) / 4;

                    // Values are inverted for some reason, docs say they shouldn't be but whatever
                    LUTs[ir_channel + 1][1023 - lut_index] = value;
                }
            }
        }
        // Either we didn't pull any subcom frames or they were too damaged
        else
        {
            logger->warn("Reception was too short or SNR was too low: timestamps, projections, and calibration will be disabled!");
            // Defaults timestamp to system time
            unix_timestamp = time(0);
        }

        // Sanity check, if the timestamp isn't between 2000 and 2050, consider it to be incorrect
        // (I don't think the Fengyun 2 satellites will live for another 25 years)
        if (unix_timestamp < 946681200 || unix_timestamp > 2524604400)
        {
            logger->warn("The pulled timestamp looks erroneous! Was the SNR too low? Defaulting to system time");
            unix_timestamp = time(0);
        }

        svissr_product.set_product_timestamp(unix_timestamp);
        const time_t timevalue = unix_timestamp;

        // Copies the gmtime result since it gets modified elsewhere
        std::tm timeReadable = *gmtime(&timevalue);
        std::string timestamp = std::to_string(timeReadable.tm_year + 1900) + "-" +
                                (timeReadable.tm_mon + 1 > 9 ? std::to_string(timeReadable.tm_mon + 1) : "0" + std::to_string(timeReadable.tm_mon + 1)) + "-" +
                                (timeReadable.tm_mday > 9 ? std::to_string(timeReadable.tm_mday) : "0" + std::to_string(timeReadable.tm_mday)) + "_" +
                                (timeReadable.tm_hour > 9 ? std::to_string(timeReadable.tm_hour) : "0" + std::to_string(timeReadable.tm_hour)) + "-" +
                                (timeReadable.tm_min > 9 ? std::to_string(timeReadable.tm_min) : "0" + std::to_string(timeReadable.tm_min));

        logger->info("Full disk finished, saving at " + timestamp + "...");

        std::filesystem::create_directory(buffer.directory + "/" + timestamp);

        std::string disk_folder = buffer.directory + "/" + timestamp;

        // Raw Images
        svissr_product.images.push_back({0, getSvissrFilename(&timeReadable, "1"), "1", buffer.image1, 10, satdump::ChannelTransform().init_none()});
        svissr_product.images.push_back({1, getSvissrFilename(&timeReadable, "2"), "2", buffer.image2, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
        svissr_product.images.push_back({2, getSvissrFilename(&timeReadable, "3"), "3", buffer.image3, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
        svissr_product.images.push_back({3, getSvissrFilename(&timeReadable, "4"), "4", buffer.image4, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
        svissr_product.images.push_back({4, getSvissrFilename(&timeReadable, "5"), "5", buffer.image5, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});

        svissr_product.set_channel_wavenumber(0, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.7e-6));
        svissr_product.set_channel_wavenumber(1, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 3.75e-6));
        svissr_product.set_channel_wavenumber(2, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 6.75e-6));
        svissr_product.set_channel_wavenumber(3, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 10.8e-6));
        svissr_product.set_channel_wavenumber(4, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 12e-6));

        // TODOREWORK: Add back composite autogeneration

        // Calibrate if we got the calibration data
        if (process_subcom_data)
        {
            logger->trace("Got calibration info");

            nlohmann::json calib_cfg;

            // LUTs transmitted in order: VIS -> IR1 -> IR2 -> IR3 -> IR4
            // Ergo Visible (0.7 μm) -> LWIR (10.8 μm) -> Split window (12 μm) -> Water vapour (6.75 μm) -> MWIR (3.75 μm)
            // We order them based on the wavelength.
            calib_cfg["ch1_lut"] = LUTs[0];
            calib_cfg["ch2_lut"] = LUTs[4];
            calib_cfg["ch3_lut"] = LUTs[3];
            calib_cfg["ch4_lut"] = LUTs[1];
            calib_cfg["ch5_lut"] = LUTs[2];

            svissr_product.set_calibration("fy2_svissr", calib_cfg);

            svissr_product.set_channel_unit(0, CALIBRATION_ID_ALBEDO);
            svissr_product.set_channel_unit(1, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
            svissr_product.set_channel_unit(2, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
            svissr_product.set_channel_unit(3, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
            svissr_product.set_channel_unit(4, CALIBRATION_ID_BRIGHTNESS_TEMPERATURE);
        }
        svissr_product.save(disk_folder);

        buffer.image1.clear();
        buffer.image2.clear();
        buffer.image3.clear();
        buffer.image4.clear();
        buffer.image5.clear();

        // Ensures we have no leftovers from subcom handling
        subcommunication_frames.clear();
        current_subcom_frame.clear();
        group_retransmissions.clear();
        final_subcom_frame.clear();

        writingImage = false;
    }

    void SVISSRImageDecoderModule::process()
    {
        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGE";

        if (!std::filesystem::exists(directory))
            std::filesystem::create_directory(directory);

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        uint8_t last_status[20];
        memset(last_status, 0, 20);

        int last_group_id = 0;

        valid_lines = 0;

        bool is_live = input_data_type != satdump::pipeline::DATA_FILE;

        if (is_live)
        {
            images_thread_should_run = true;
            images_queue_thread = std::thread(&SVISSRImageDecoderModule::image_saving_thread_f, this);
        }

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)frame, FRAME_SIZE);

            // Parse counter, masked since first four bits should NEVER be 0
            int counter = (frame[67] << 8 | frame[68]) & 0x0fff;

            // Does correction logic if specified by the user
            if (apply_correction)
            {
                // Unlocks if we are starting a new series
                if (counter_locked && (counter == 1 || counter == 2))
                {
                    counter_locked = false;
                }
                else if (!counter_locked)
                {
                    // Can we lock?
                    if (counter == global_counter + 1)
                    {
                        // LOCKED!
                        logger->debug("Counter correction LOCKED! Counter: " + std::to_string(counter));
                        counter_locked = true;
                    }
                    else
                    {
                        // We can't lock, save this counter for a check on the next one
                        global_counter = counter;
                    }
                }

                // We are locked, assume this frame's counter is the previous one plus one.
                if (counter_locked)
                {
                    counter = global_counter + 1;
                    global_counter = counter;

                    // The counter used in the decoding process is pulled from the frame,
                    // so we rewrite it here.
                    frame[67] = (counter >> 8) & 0xFF;
                    frame[68] = counter & 0xFF;
                }
            }

            // ID of the block group: Simplified mapping, Orbit and attitude data, MANAM,
            // Calibration block 1 and 2 are all sent in 25 separate groups because of their
            // size. The group ID defines which group is sent. Every group gets transmitted
            // 8 times every 200 lines
            // Masked since max GID is 25. Increases reliability
            int group_id = frame[193] & 0x1f;

            // Each group is transmitted 8 subsequent times
            // int repeat_id = frame[195];

            if (group_id != last_group_id)
            {

                // Subcom frame finished, we should have 24 saved (1 still in buffer)
                if (group_id == 0 && last_group_id == 24)
                {
                    save_minor_frame();
                }
                // We have finished this group, save it. If we didn't get the previous groups, do NOT Proceed!!!
                else if (group_retransmissions.size() < 9 && (current_subcom_frame.size() / SUBCOM_GROUP_SIZE) == group_id - 1 //&& group_id-last_group_id==1
                )
                {
                    // TODO: how to avoid junk? if we lose sync within a group we screw the whole thing over
                    // has to be more robust when it comes to frame drops too!!!
                    if (!group_retransmissions.empty())
                    { // The group is finished, push back the result of majority law to the minor frame set
                        Group group = majority_law(group_retransmissions, false);
                        current_subcom_frame.insert(current_subcom_frame.end(), group.begin(), group.end());

                        group_retransmissions.clear();
                    }
                    else
                    {
                        // Can happen if we sync 1/8 frames
                        logger->debug("TRIED SAVING EMPTY RETRANSMISSION GROUP!!!");
                    }
                }
                else if (group_retransmissions.size() > 8)
                {
                    logger->critical("GROUP RETRANSMISSIONS OVERFLOW!!!!");
                    group_retransmissions.clear();
                }
            }
            else
            {
                // We are in the middle of a group repetition, save it

                // If we have a group saved, we are in a new frame!!! Save it too
                // ONLY save it if we got the previous ones already
                if ((current_subcom_frame.size() / SUBCOM_GROUP_SIZE) == group_id)
                {
                    Group current_retransmission(frame + SUBCOM_START_OFFSET, frame + SUBCOM_START_OFFSET + SUBCOM_GROUP_SIZE);
                    group_retransmissions.push_back(current_retransmission);
                }
                else
                {
                    // We missed the beginning of the frame, discard the rest as it is useless
                    group_retransmissions.clear();
                }
            }

            // Store the last GID to make sure we can parse the retransmission groups
            last_group_id = group_id;

            // Parse Spacecraft ID (SC/ID)
            int scid = frame[91];

            scid_stats.push_back(scid);

            // Safeguard
            if (counter > 2500)
            {
                // Unlocks since we are locked at an impossible value.. somehow
                counter_locked = false;
                continue;
            }

            // We only want forward scan data
            backwardScan = (frame[3] & 0b11) == 0;

            memmove(last_status, &last_status[1], 19);
            last_status[19] = backwardScan;

            // Try to detect a new scan
            uint8_t backward_scanning = satdump::most_common(&last_status[0], &last_status[20], 0);
            // Ensures the counter doesn't lock during rollback
            // Situation:
            //   Image ends -> Rollback starts -> Corrector erroneously locks during rollback
            //   -> Corrector shows "LOCKED" until 40 rollback lines are scanned
            if (backward_scanning && valid_lines < 5)
            {
                counter_locked = false;
            }

            if (backward_scanning && valid_lines > 40)
            {
                logger->info("Full disk end detected!");

                // Image has ended, unlock the corrector (if it was enabled)
                counter_locked = false;

                std::shared_ptr<SVISSRBuffer> buffer = std::make_shared<SVISSRBuffer>();

                // Backup images
                buffer->image1 = vissrImageReader.getImageVIS();
                buffer->image2 = vissrImageReader.getImageIR4();
                buffer->image3 = vissrImageReader.getImageIR3();
                buffer->image4 = vissrImageReader.getImageIR1();
                buffer->image5 = vissrImageReader.getImageIR2();

                buffer->scid = satdump::most_common(scid_stats.begin(), scid_stats.end(), 0);
                scid_stats.clear();

                // TODOREWORK majority law would be incredibly useful here, but needs N-element
                // implementation because we might sync less frames than intended
                // Please note that the timestamps are already converted to unix timestamps,
                // this would have to happen on the raw JD one (see where the the stats are pushed to)

                buffer->directory = directory;

                // Write those
                if (is_live)
                {
                    images_queue_mtx.lock();
                    images_queue.push_back(buffer);
                    images_queue_mtx.unlock();
                }
                else
                {
                    writeImages(*buffer);
                }

                // Reset readers
                vissrImageReader.reset();
                valid_lines = 0;
            }

            if (backwardScan)
                continue;

            // Process it
            vissrImageReader.pushFrame(frame);
            valid_lines++;
            approx_progess = round(((float)counter / 2500.0f) * 1000.0f) / 10.0f;
        }

        cleanup();

        if (is_live)
        {
            logger->info("Full disk end detected!");

            std::shared_ptr<SVISSRBuffer> buffer = std::make_shared<SVISSRBuffer>();

            // Backup images
            buffer->image1 = vissrImageReader.getImageIR1();
            buffer->image2 = vissrImageReader.getImageIR2();
            buffer->image3 = vissrImageReader.getImageIR3();
            buffer->image4 = vissrImageReader.getImageIR4();
            buffer->image5 = vissrImageReader.getImageVIS();

            buffer->scid = satdump::most_common(scid_stats.begin(), scid_stats.end(), 0);
            scid_stats.clear();

            buffer->directory = directory;

            images_queue_mtx.lock();
            images_queue.push_back(buffer);
            images_queue_mtx.unlock();

            images_thread_should_run = false;
            if (images_queue_thread.joinable())
                images_queue_thread.join();
        }
        else if (valid_lines > 40)
        {
            logger->info("Full disk end detected!");

            std::shared_ptr<SVISSRBuffer> buffer = std::make_shared<SVISSRBuffer>();

            // Backup images
            buffer->image1 = vissrImageReader.getImageIR1();
            buffer->image2 = vissrImageReader.getImageIR2();
            buffer->image3 = vissrImageReader.getImageIR3();
            buffer->image4 = vissrImageReader.getImageIR4();
            buffer->image5 = vissrImageReader.getImageVIS();

            buffer->scid = satdump::most_common(scid_stats.begin(), scid_stats.end(), 0);
            scid_stats.clear();

            buffer->directory = directory;

            writeImages(*buffer);
        }
    }

    nlohmann::json SVISSRImageDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["full_disk_progress"] = approx_progess;
        return v;
    }

    void SVISSRImageDecoderModule::drawUI(bool window)
    {
        if (textureID == 0)
        {
            textureID = makeImageTexture();
            textureBuffer = new uint32_t[2501 * 2291];
            memset(textureBuffer, 0, sizeof(uint32_t) * 2501 * 2291);
        }

        ImGui::Begin("S-VISSR Image Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // This is outer crap...
        ImGui::BeginGroup();
        {
            ushort_to_rgba(vissrImageReader.imageBufferIR1, textureBuffer, 2501 * 2291);
            updateImageTexture(textureID, textureBuffer, 2291, 2501);
            ImGui::Image((void *)(intptr_t)textureID, {200 * ui_scale, 200 * ui_scale});
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        // Writes out a simple GUI for the corrector status if it was enabled by the user
        if (apply_correction)
        {
            ImGui::Button("Correction", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Counter correction status: ");

                ImGui::SameLine();

                if (global_counter == 0)
                    ImGui::TextColored(style::theme.red, "WAITING");
                else if (global_counter != 0 && !counter_locked)
                    ImGui::TextColored(style::theme.orange, "LOCKING");
                else
                {
                    ImGui::TextColored(style::theme.green, "LOCKED");
                }
            }
        }

        {
            ImGui::Button("Full Disk Progress", {200 * ui_scale, 20 * ui_scale});
            ImGui::ProgressBar((float)approx_progess / 100.0f, ImVec2(200 * ui_scale, 20 * ui_scale));
            ImGui::Text("State : ");
            ImGui::SameLine();
            if (writingImage)
                ImGui::TextColored(style::theme.green, "Writing images...");
            else if (backwardScan)
                ImGui::TextColored(style::theme.red, "Imager rollback...");
            else
                ImGui::TextColored(style::theme.orange, "Receiving...");
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string SVISSRImageDecoderModule::getID() { return "fengyun_svissr_image_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> SVISSRImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SVISSRImageDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun_svissr
