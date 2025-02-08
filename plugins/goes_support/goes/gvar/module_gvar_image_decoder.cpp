#include "module_gvar_image_decoder.h"
#include "logger.h"
#include "common/codings/differential/nrzs.h"
#include "imgui/imgui.h"
#include "gvar_deframer.h"
#include "gvar_derand.h"
#include "imgui/imgui_image.h"
#include <filesystem>
#include "gvar_headers.h"
#include "common/utils.h"
#include "resources.h"
#include "common/image/hue_saturation.h"
#include "common/image/brightness_contrast.h"
#include "common/thread_priority.h"
#include "common/image/io.h"

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
    PrimaryBlockHeader get_header(uint8_t *frame) {

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
        for (int byteIndex = 0; byteIndex < 30; ++byteIndex) {
            uint8_t majority = 0;

            // Process each bit in the byte
            for (int bitIndex = 0; bitIndex < 8; ++bitIndex) {
                // Extract bits from each byte
                uint8_t bit_a = (a[byteIndex] >> bitIndex) & 1;
                uint8_t bit_b = (b[byteIndex] >> bitIndex) & 1;
                uint8_t bit_c = (c[byteIndex] >> bitIndex) & 1;

                // Count the number of 1s
                int count_ones = bit_a + bit_b + bit_c;

                // Majority rule: if at least two are 1, set the result bit
                if (count_ones >= 2) {
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
    uint32_t parse_counters(const std::vector<Block>& frame_buffer) {
        uint32_t result = 0;
        std::vector<uint32_t> counters;

        for (size_t i = 0; i < frame_buffer.size(); ++i) {
            counters.push_back(frame_buffer[i].original_counter & 0x7FF);
        }

        const size_t majority_threshold = (counters.size() / 2) + 1;

        for (int bit = 0; bit < 11; ++bit) {
            size_t count_ones = 0;
            for (size_t i = 0; i < counters.size(); ++i) {
                if (counters[i] & (1 << bit)) {
                    ++count_ones;
                }
            }
            if (count_ones >= majority_threshold) {
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
            markerBit = getBit<uint32_t>(spare, i);
            uint32_t zero = 0;
            testBit = getBit<uint32_t>(zero, i);
            if (markerBit != testBit)
                errors++;
        }
        // This can be adjusted to allow the spare to be more degraded than usual
        // >5 bits should be incredibly rare after bit-level majority law, at that point
        // the block ID is almost guaranteed to be incorrect messing the frame up anyways
        if (errors > 5) {
            return false;
        } else {
            return true;
        }
    }

        /**
         * Decodes imagery from the frame buffer while attempting block ID recovery and counter correction
         */
        void GVARImageDecoderModule::process_frame_buffer(std::vector<Block> &frame_buffer) {
            // The most common counter from this series
            uint32_t final_counter = parse_counters(frame_buffer);

            // Are we decoding a new image?
            // If this series is 1, or 1 wasn't decoded and we are at 2, save the image
            if (final_counter == 1 || (imageFrameCount > 2 && final_counter == 2)) { 
                logger->info("Image start detected!");

                imageFrameCount = 0;

                if (isImageInProgress)
                {
                    if (writeImagesAync)
                    {
                        logger->debug("Saving Async...");
                        isImageInProgress = false;
                        isSavingInProgress = true;
                        imageVectorMutex.lock();
                        imagesVector.push_back({infraredImageReader1.getImage1(),
                                                infraredImageReader1.getImage2(),
                                                infraredImageReader2.getImage1(),
                                                infraredImageReader2.getImage2(),
                                                visibleImageReader.getImage(),
                                                most_common(scid_stats.begin(), scid_stats.end(), 0),
                                                most_common(vis_width_stats.begin(), vis_width_stats.end(), 0)});
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
                                            most_common(scid_stats.begin(), scid_stats.end(), 0),
                                            most_common(vis_width_stats.begin(), vis_width_stats.end(), 0)};
                        writeImages(images, directory);
                        isSavingInProgress = false;
                    }

                    scid_stats.clear();
                    vis_width_stats.clear();
                    ir_width_stats.clear();

                    // Reset readers
                    infraredImageReader1.startNewFullDisk();
                    infraredImageReader2.startNewFullDisk();
                    visibleImageReader.startNewFullDisk();
                }

            }
            else {
                imageFrameCount++;

                // Minimum of 10 block series to write (80 VIS pixels)
                if (imageFrameCount > 10 && !isImageInProgress)
                    isImageInProgress = true;
            }

            // Processes all frames
            for (size_t i = 0; i < frame_buffer.size(); i++) {

                uint8_t *current_frame = frame_buffer[i].frame;
                PrimaryBlockHeader cur_block_header = get_header(current_frame);


                // Tries to recover the block ID in case it is damaged within a series
                // Junk will never pass through this, as those frames are thrown out before
                // being added to the frame buffer
                if (i > 1 && i < 10 ) {
                    uint32_t prev_counter = frame_buffer[i-1].block_id;
                    uint32_t next_counter = frame_buffer[i+1].block_id;

                    if (next_counter-prev_counter == 2) {
                        // The previous and next blocks suggest we should be inbetween them
                        cur_block_header.block_id = (frame_buffer[i-1].block_id)+1;
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
                    if (final_counter == 0) {
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
                        vis_width_stats.push_back(line_header.pixel_count);

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
                        int current_words = most_common(ir_width_stats.begin(), ir_width_stats.end(), 0);

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
            }

            frame_buffer.clear();  

        }

        void GVARImageDecoderModule::writeSounder()
        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);
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

        void GVARImageDecoderModule::writeImages(GVARImages &images, std::string directory)
        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));

            // Get stats. This is done over a lot of data to allow decoding at low SNR
            int sat_number = images.sat_number;
            int vis_width = images.vis_width;

            std::string dir_name = "GOES-" + std::to_string(sat_number) + "/" + timestamp;
            logger->info("Full disk finished, saving at " + dir_name + "...");

            std::filesystem::create_directories(directory + "/" + dir_name);

            std::string disk_folder = directory + "/" + dir_name;

            logger->info("Resizing...");
            images.image1.resize(images.image1.width(), images.image1.height() * 1.75);
            images.image2.resize(images.image2.width(), images.image2.height() * 1.75);
            images.image3.resize(images.image3.width(), images.image3.height() * 1.75);
            images.image4.resize(images.image4.width(), images.image4.height() * 1.75);
            images.image5.resize(images.image5.width(), images.image5.height() * 1.75);

            // logger->trace("VIS 1 size before " + std::to_string(images.image5.width()) + "x" + std::to_string(images.image5.height()));
            // logger->trace("IR size before " + std::to_string(images.image1.width()) + "x" + std::to_string(images.image1.height()));

            // VIS-1 height
            int vis_height = images.image5.height();

            if (vis_width == 13216) // Some partial scan
                vis_height = 9500;
            else if (vis_width == 11416) // Some partial scan
                vis_height = 6895;
            else if (vis_width == 8396) // Some partial scan
                vis_height = 4600;
            else if (vis_width == 11012) // Some partial scan
                vis_height = 7456;
            else if (vis_width == 4976) // Some partial scan
                vis_height = 4194;
            else if (vis_width == 20836) // Full disk
                vis_height = 18956;
            else if (vis_width == 20824) // Full disk
                vis_height = 18956;

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

            image::save_img(images.image5, std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "1")).c_str(), false);
            image::save_img(images.image1, std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "2")).c_str());
            image::save_img(images.image2, std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "3")).c_str());
            image::save_img(images.image3, std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "4")).c_str());
            image::save_img(images.image4, std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "5")).c_str());

            // Let plugins do something
            satdump::eventBus->fire_event<events::GVARSaveChannelImagesEvent>({images, timeReadable, timevalue, disk_folder});

            // We are done with all channels but 1 and 4. Clear others to free up memory!
            images.image1.clear();
            images.image2.clear();
            images.image4.clear();

            // If we can, generate false color
            if (resources::resourceExists("lut/goes/gvar/lut.png"))
            {
                logger->trace("Scale Ch1 to 8-bits...");
                image::Image channel1(8, images.image5.width(), images.image5.height(), 1);
                for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
                    channel1.set(i, images.image5.get(i) / 255);
                images.image5.clear(); // We're done with Ch1. Free up memory

                logger->trace("Scale Ch4 to 8-bits...");
                image::Image channel4(8, images.image3.width(), images.image3.height(), 1);
                for (size_t i = 0; i < channel4.width() * channel4.height(); i++)
                    channel4.set(i, images.image3.get(i) / 255);
                images.image3.clear(); // We're done with Ch4. Free up memory

                logger->trace("Resize images...");
                channel4.resize(channel1.width(), channel1.height());

                logger->trace("Loading LUT...");
                image::Image lutImage;
                image::load_png(lutImage, resources::getResourcePath("lut/goes/gvar/lut.png").c_str());
                lutImage.resize(256, 256);

                logger->trace("Loading correction curve...");
                image::Image curveImage;
                image::load_png(curveImage, resources::getResourcePath("lut/goes/gvar/curve_goesn.png").c_str());

                image::Image compoImage(8, channel1.width(), channel1.height(), 3);

                logger->trace("Applying LUT...");
                for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
                {
                    uint8_t x = 255 - curveImage.get(channel1.get(i)) / 1.5;
                    uint8_t y = channel4.get(i);

                    for (int c = 0; c < 3; c++)
                        compoImage.set(c * compoImage.width() * compoImage.height() + i, lutImage.get(c * lutImage.width() * lutImage.height() + x * lutImage.width() + y));
                }

                logger->trace("Contrast correction...");
                image::brightness_contrast(compoImage, -10.0f / 127.0f, 24.0f / 127.0f);

                logger->trace("Hue shift...");
                image::HueSaturation hueTuning;
                hueTuning.hue[image::HUE_RANGE_MAGENTA] = 133.0 / 180.0;
                hueTuning.overlap = 100.0 / 100.0;
                image::hue_saturation(compoImage, hueTuning);
                image::save_img(compoImage, std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "FC")).c_str());

                // Let plugins do something
                satdump::eventBus->fire_event<events::GVARSaveFCImageEvent>({compoImage, images.sat_number, timeReadable, timevalue, disk_folder});
            }
            else
            {
                logger->warn("lut/goes/gvar/lut.png LUT is missing! False Color will not be generated");
            }
        }

        GVARImageDecoderModule::GVARImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
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
        }

        std::vector<ModuleDataType> GVARImageDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GVARImageDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
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
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);

            if (input_data_type != DATA_FILE)
                writeImagesAync = true;

            // Start thread
            if (writeImagesAync)
            {
                imageSavingThread = std::thread(&GVARImageDecoderModule::writeImagesThread, this);
                setLowestThreadPriority(imageSavingThread); // Low priority to avoid sampledrop
            }

            directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGE";
            directory_snd = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SOUNDER";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            if (!std::filesystem::exists(directory_snd))
                std::filesystem::create_directory(directory_snd);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            int last_val = 0;

            // Series of consecutive imagery blocks are saved here
            std::vector<Block> frame_buffer;


            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)frame, FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)frame, FRAME_SIZE);


                PrimaryBlockHeader block_header = get_header(frame);

                // Ensures we aren't looking at junk
                if (!blockIsActualData(block_header)) {
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
                            if (x_pos != last_val)
                                writeSounder();
                            last_val = x_pos;
                        }
                        sounderReader.pushFrame(frame, 0);
                    }
                }

                // Are we ready to process our frame buffer?
                // Processes if we already have a series saved (10 frames are inside the buffer)
                // OR if the last frame we looked at was 10 (the last one)
                if (frame_buffer.size() > 9 || (!frame_buffer.empty() && frame_buffer.back().block_id == 10))
                {
                    process_frame_buffer(frame_buffer);             
                }


                // Saves imagery blocks for processing
                if (block_header.block_id >= 1 && block_header.block_id <= 10) {
                    LineDocumentationHeader line_header(&frame[8 + 30 * 3]);

                    Block current_block;
                    current_block.block_id = block_header.block_id;
                    current_block.original_counter = line_header.relative_scan_count;
                    current_block.frame = new uint8_t[FRAME_SIZE];
                    std::memcpy(current_block.frame, frame, FRAME_SIZE);
                    
                    frame_buffer.push_back(
                        current_block
                    );
                }


                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                // Update module stats
                module_stats["full_disk_progress"] = approx_progess;

                    
                if (time(NULL) % 10 == 0 && lastTime != time(NULL)) {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) +
                                "%%, Full Disk Progress : " + std::to_string(round(((float)approx_progess / 100.0f) * 1000.0f) / 10.0f) + "%%");
                }

            }

            // Ensures we don't leave frames behind
            if (!frame_buffer.empty()) {
                process_frame_buffer(frame_buffer);
            }


            if (input_data_type == DATA_FILE)
                data_in.close();

            //TODO: Maybe don't write the sounder if it's empty?
            writeSounder();

            if (writeImagesAync)
            {
                logger->info("Exit async thread...");
                writeImagesAync = false;
                if (imageSavingThread.joinable())
                    imageSavingThread.join();
            }

            logger->info("Dump remaining data...");
            if (isImageInProgress)
            {
                isImageInProgress = false;
                isSavingInProgress = true;
                // Backup images
                GVARImages images = {infraredImageReader1.getImage1(),
                                    infraredImageReader1.getImage2(),
                                    infraredImageReader2.getImage1(),
                                    infraredImageReader2.getImage2(),
                                    visibleImageReader.getImage(),
                                    most_common(scid_stats.begin(), scid_stats.end(), 0),
                                    most_common(vis_width_stats.begin(), vis_width_stats.end(), 0)};
                // Write those
                writeImages(images, directory);
            }
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

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

            ImGui::End();
        }

        std::string GVARImageDecoderModule::getID()
        {
            return "goes_gvar_image_decoder";
        }

        std::vector<std::string> GVARImageDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GVARImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GVARImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    }
}