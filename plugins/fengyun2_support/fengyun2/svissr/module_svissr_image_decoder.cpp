#include "module_svissr_image_decoder.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "logger.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>

#define FRAME_SIZE 44356

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace fengyun_svissr
{
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

    void SVISSRImageDecoderModule::writeImages(SVISSRBuffer &buffer)
    {
        writingImage = true;

        logger->info("Found SCID " + std::to_string(buffer.scid));

        if (buffer.timestamp == 0)
        {
            logger->warn("No timestamps were pulled! Was the reception too short? Defaulting to system time");
            buffer.timestamp = time(0);
        }
        // Sanity check, if the timestamp isn't between 2000 and 2050, consider it to be incorrect
        // (I don't think the Fengyun 2 satellites will live for another 25 years)
        else if (buffer.timestamp < 946681200 || buffer.timestamp > 2524604400)
        {
            logger->warn("The pulled timestamp looks erroneous! Was the SNR too low? Defaulting to system time");
            buffer.timestamp = time(0);
        }

        const time_t timevalue = buffer.timestamp;
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

        // Save products
        satdump::products::ImageProduct svissr_product;

        sat_name = "";

        // Get the sat name
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


        svissr_product.instrument_name = "fy2-svissr";
        svissr_product.set_product_timestamp(buffer.timestamp);

        // Raw Images
        svissr_product.images.push_back({0, getSvissrFilename(&timeReadable, "1"), "1", buffer.image5, 10, satdump::ChannelTransform().init_none()});
        svissr_product.images.push_back({1, getSvissrFilename(&timeReadable, "2"), "2", buffer.image4, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
        svissr_product.images.push_back({2, getSvissrFilename(&timeReadable, "3"), "3", buffer.image3, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
        svissr_product.images.push_back({3, getSvissrFilename(&timeReadable, "4"), "4", buffer.image1, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});
        svissr_product.images.push_back({4, getSvissrFilename(&timeReadable, "5"), "5", buffer.image2, 10, satdump::ChannelTransform().init_affine(4, 4, 0, 0)});

        // Set the channel wavelengths
        svissr_product.set_channel_wavenumber(0, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 0.65e-6));
        svissr_product.set_channel_wavenumber(1, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 3.75e-6));
        svissr_product.set_channel_wavenumber(2, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 7.15e-6));
        svissr_product.set_channel_wavenumber(3, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 10.8e-6));
        svissr_product.set_channel_wavenumber(4, freq_to_wavenumber(SPEED_OF_LIGHT_M_S / 12e-6));

        // TODOREWORK: Add back composite autogeneration

        svissr_product.save(disk_folder);

        buffer.image1.clear();
        buffer.image2.clear();
        buffer.image3.clear();
        buffer.image4.clear();
        buffer.image5.clear();

        writingImage = false;
    }

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

            // Do the actual work
            {
                // Parse counter
                int counter = frame[67] << 8 | frame[68];

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
                // 8 times every 200 lines, the retransmission counter is at the 195th byte
                int group_id = frame[193];

                // First 6 bytes from the Orbit and Attitude header, which is sent in 25 100-byte pieces
                // We need the first piece -> GID = 0
                if (group_id == 0)
                {

                    // R6*8 -> Big endian 6 byte integer, needs 10^-8 to read the value
                    uint64_t raw =
                        ((uint64_t)frame[296] << 40 | (uint64_t)frame[297] << 32 | (uint64_t)frame[298] << 24 | (uint64_t)frame[299] << 16 | (uint64_t)frame[300] << 8 | (uint64_t)frame[301]);

                    // note for future developers that will save you 6 hours of your life debugging
                    // 10x10^-8 != 10^-8
                    // i hate myself
                    double timestamp = raw * 1e-8;

                    // Converts the MJD timestamp to a unix timestamp
                    timestamp_stats.push_back(((timestamp - 40587) * 86400));
                }

                // Parse SCID
                int scid = frame[91];

                scid_stats.push_back(scid);

                // Safeguard
                if (counter > 2500)
                    continue;

                // Parse scan status
                int status = frame[3] & 0b11; // Decoder scan status

                // We only want forward scan data
                backwardScan = status == 0;

                memmove(last_status, &last_status[1], 19);
                last_status[19] = backwardScan;

                // std::cout << counter << std::endl;

                // Try to detect a new scan
                uint8_t is_back = satdump::most_common(&last_status[0], &last_status[20], 0);

                // Ensures the counter doesn't lock during rollback
                // Situation:
                //   Image ends -> Rollback starts -> Corrector erroneously locks during rollback
                //   -> Corrector shows "LOCKED" until 40 rollback lines are scanned
                if (is_back && valid_lines < 5)
                {
                    counter_locked = false;
                }

                if (is_back && valid_lines > 40)
                {
                    logger->info("Full disk end detected!");

                    // Image has ended, unlock the corrector (if it was enabled)
                    counter_locked = false;

                    std::shared_ptr<SVISSRBuffer> buffer = std::make_shared<SVISSRBuffer>();

                    // Backup images
                    buffer->image1 = vissrImageReader.getImageIR1();
                    buffer->image2 = vissrImageReader.getImageIR2();
                    buffer->image3 = vissrImageReader.getImageIR3();
                    buffer->image4 = vissrImageReader.getImageIR4();
                    buffer->image5 = vissrImageReader.getImageVIS();

                    buffer->scid = satdump::most_common(scid_stats.begin(), scid_stats.end(), 0);
                    scid_stats.clear();

                    // TODOREWORK majority law would be incredibly useful here, but needs N-element
                    // implementation because we might sync less frames than intended
                    // Please note that the timestamps are already converted to unix timestamps,
                    // this would have to happen on the raw JD one (see where the the stats are pushed to)

                    buffer->timestamp = satdump::most_common(timestamp_stats.begin(), timestamp_stats.end(), 0);
                    timestamp_stats.clear();

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

            // TODOREWORK majority law would be incredibly useful here, but needs N-element
            // implementation because we might sync less frames than intended
            // Please note that the timestamps are already converted to unix timestamps,
            // this would have to happen on the raw JD one (see where the the stats are pushed to)

            buffer->timestamp = satdump::most_common(timestamp_stats.begin(), timestamp_stats.end(), 0);
            timestamp_stats.clear();

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

            // TODOREWORK majority law would be incredibly useful here, but needs N-element
            // implementation because we might sync less frames than intended
            // Please note that the timestamps are already converted to unix timestamps,
            // this would have to happen on the raw JD one (see where the the stats are pushed to)

            buffer->timestamp = satdump::most_common(timestamp_stats.begin(), timestamp_stats.end(), 0);
            timestamp_stats.clear();

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
