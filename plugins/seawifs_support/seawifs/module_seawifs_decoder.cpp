#include "module_seawifs_decoder.h"

#include <sys/types.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

#include "core/resources.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/image/channel_transform.h"
#include "products/image_product.h"

// File structure:
//  - 512 byte prelude (file metadata)
//  - 790 bytes of header (SC/ID, telemetry, bit errors...)
//  - 10356 big-endian 10-bit words with imagery of either
//     - LAC format
//        - 44 words telemetry
//        - 8 words gain
//        - 10304 words image data (1 scan line -> 1285 px per scan line)
//           - There is some padding in here! 1288 px total, 32 bytes have to be skipped to get
//             rid of it. Maybe it is something else? god knows but the 32 bytes makes it work
//     - GAC format
//        - 10080 words image data (5 scan lines -> 248 px per scan line)
//        - 220 words telemetry (5x for each scan line, 44 words each)
//        - 56 words spare
//  - 2 bytes to fill out to 21 kiB
//
// Total frame size: 21504 bytes

#define PRELUDE_SIZE 512
#define FRAME_SIZE 21504
#define DATA_OFFSET 790      // Offset of LAC/GAC data from beginning of data (+32 rogue bytes)
#define LAC_VIDEO_OFFSET 52  // 52 words (2-byte, 10-bit) before video data starts
#define LAC_WORD_COUNT 10356 // Total amount of 2-byte, 10-bit words in LAC
#define LAC_PIXEL_COUNT 1285 // SeaWiFS samples per line. GAC only has 285
#define VIDEO_SIZE 10280     // Combined size of samples for all channels within line (1285*8)

namespace seawifs
{

    /**
     * Unpacks 10-bit packed right-justified words starting from *input* for *length* bytes
     * Stored within a 16-bit container!!!
     * uint8_t input: Pointer to beginning of data
     * int8_t word_count: How many 10-bit word (16-bit containers) to process
     * uint16_t output_buffer: Where to write image data to
     */
    void SeaWIFsProcessingModule::repack_words_to_16(uint8_t *input, int word_count, uint16_t *output_buffer)
    {
        int cur_word = 0;
        while (cur_word < word_count)
        {
            // Big endian order!
            uint16_t value = (((uint16_t)input[cur_word * 2] << 8) | (uint16_t)input[cur_word * 2 + 1]);

            // Masks to 10 bits
            output_buffer[cur_word] = value & 0x3ff;
            cur_word++;
        }
    }
    /**
     * Writesthe received imagery from imageBuffer
     */
    void SeaWIFsProcessingModule::write_images(uint32_t reception_timestamp)
    {
        if (imageBuffer.empty())
        {
            logger->error("No imagery has been decoded!");
            return;
        }

        int line_count = imageBuffer.size() / VIDEO_SIZE;

        if (line_count != floor(line_count))
        {
            logger->error("Data type seems invalid, can't reliably determine amount of frames");
            return;
        }

        // At this point we know that we know that we have a valid amount of frames, decode em
        std::vector<uint16_t> channel1_data;
        std::vector<uint16_t> channel2_data;
        std::vector<uint16_t> channel3_data;
        std::vector<uint16_t> channel4_data;
        std::vector<uint16_t> channel5_data;
        std::vector<uint16_t> channel6_data;
        std::vector<uint16_t> channel7_data;
        std::vector<uint16_t> channel8_data;

        for (int cur_line = 0; cur_line < line_count; cur_line++)
        {
            for (int cur_px = 0; cur_px < LAC_PIXEL_COUNT; cur_px++)
            {
                channel1_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8]);
                channel2_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 1]);
                channel3_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 2]);
                channel4_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 3]);
                channel5_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 4]);
                channel6_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 5]);
                channel7_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 6]);
                channel8_data.push_back(imageBuffer[VIDEO_SIZE * cur_line + cur_px * 8 + 7]);
            }
        }

        // Every channel is now neatly loaded into its own vector

        image::Image channel1 = image::Image(channel1_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel2 = image::Image(channel2_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel3 = image::Image(channel3_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel4 = image::Image(channel4_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel5 = image::Image(channel5_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel6 = image::Image(channel6_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel7 = image::Image(channel7_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);
        image::Image channel8 = image::Image(channel8_data.data(), 16, LAC_PIXEL_COUNT, line_count, 1);

        // We can now deal with making it a product
        satdump::products::ImageProduct seawifs_product;
        seawifs_product.set_product_source("OrbView-2 (SeaStar)");
        seawifs_product.set_product_timestamp(reception_timestamp);
        seawifs_product.instrument_name = "seawifs";

        // Set for geo correction
        seawifs_product.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/seastar_seawifs.json")));

        // TODOREWORK: Projections are possible! Seadas OCSSW can process l0 to l1a, the satellite
        // gives everything needed for every line - gps coords, tilt info...
        // src: https://github.com/fengqiaogh/ocssw_src/blob/main/ocssw_src/src/l1aextract_seawifs/extract_sub.c

        seawifs_product.images.push_back({0, "SeaWiFS-ch1", "1", channel1, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({1, "SeaWiFS-ch2", "2", channel2, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({2, "SeaWiFS-ch3", "3", channel3, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({3, "SeaWiFS-ch4", "4", channel4, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({4, "SeaWiFS-ch5", "5", channel5, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({5, "SeaWiFS-ch6", "6", channel6, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({6, "SeaWiFS-ch7", "7", channel7, 10, satdump::ChannelTransform().init_none()});
        seawifs_product.images.push_back({7, "SeaWiFS-ch8", "8", channel8, 10, satdump::ChannelTransform().init_none()});

        seawifs_product.set_channel_wavenumber(0, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.412e-6));
        seawifs_product.set_channel_wavenumber(1, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.443e-6));
        seawifs_product.set_channel_wavenumber(2, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.490e-6));
        seawifs_product.set_channel_wavenumber(3, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.510e-6));
        seawifs_product.set_channel_wavenumber(4, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.555e-6));
        seawifs_product.set_channel_wavenumber(5, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.670e-6));
        seawifs_product.set_channel_wavenumber(6, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.765e-6));
        seawifs_product.set_channel_wavenumber(7, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.865e-6));

        logger->info("Saving imagery to " + directory);
        seawifs_product.save(directory);

        // Clears the buffers (is this needed? unsure but I will rather do this than not)
        channel1_data.clear();
        channel2_data.clear();
        channel3_data.clear();
        channel4_data.clear();
        channel5_data.clear();
        channel6_data.clear();
        channel7_data.clear();
        channel8_data.clear();
    };

    SeaWIFsProcessingModule::SeaWIFsProcessingModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        prelude = new uint8_t[PRELUDE_SIZE];
        frame = new uint8_t[FRAME_SIZE];

        std::vector<uint16_t> imageBuffer;

        fsfsm_enable_output = false;
    }

    SeaWIFsProcessingModule::~SeaWIFsProcessingModule()
    {
        delete[] prelude;
        delete[] frame;
        imageBuffer.clear();
    }

    void SeaWIFsProcessingModule::process()
    {
        directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "SeaWIFs";

        if (!std::filesystem::exists(directory))
            std::filesystem::create_directory(directory);

        int file_size;
        file_size = d_input_file.size();

        /* Get the data from the file */

        // Save the archival metadata, only present once in the whole file
        read_data((uint8_t *)prelude, PRELUDE_SIZE);

        int frame_count = floor(file_size / FRAME_SIZE);

        if (frame_count != file_size / FRAME_SIZE)
        {
            logger->error("Inconsistent frame length with file, aborting!");
            return;
        }

        while (should_run())
        {
            // Read a frame
            read_data((uint8_t *)frame, FRAME_SIZE);

            // 10-bit big endian data, has to be repacked
            uint16_t lac_data[LAC_WORD_COUNT];

            // 32 bytesare  added since there is some weird padding and this is the only way it works
            // WARNING: INSTRUMENT AND ANCILLARY TELEMETRY IS NOT SAVED! It starts at
            // 790 bytes into the file, but we start 32 bytes AHEAD of that to get rid of
            // the padding. Figure the padding (stripes on left/right side of image) out to remove
            // the arbitrary offset
            repack_words_to_16(&frame[DATA_OFFSET + 32], LAC_WORD_COUNT, lac_data);

            const uint16_t *start = &lac_data[LAC_VIDEO_OFFSET];
            imageBuffer.insert(imageBuffer.end(), start, start + VIDEO_SIZE);
        }

        uint32_t timestamp = *(uint32_t *)(&prelude[333]);
        if (timestamp == 0)
            logger->warn("File doesn't have a timestamp! Does it use the old format? Not setting!");

        // We are done processing, write the imagery
        write_images(timestamp);
        return;
    }

    void SeaWIFsProcessingModule::drawUI(bool window)
    {
        ImGui::Begin("SeaWiFS Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        drawProgressBar();

        ImGui::End();
    }

    std::shared_ptr<satdump::pipeline::ProcessingModule> SeaWIFsProcessingModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SeaWIFsProcessingModule>(input_file, output_file_hint, parameters);
    }

} // namespace seawifs