#include "module_svissr_image_decoder.h"
#include "logger.h"
#include "common/codings/differential/nrzs.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include <filesystem>
#include "resources.h"
#include "common/utils.h"

#define FRAME_SIZE 44356

// Return filesize
size_t getFilesize(std::string filepath);

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

        const time_t timevalue = buffer.timestamp; // time(0);
        std::tm *timeReadable = gmtime(&timevalue);
        std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));

        logger->info("Full disk finished, saving at " + timestamp + "...");

        std::filesystem::create_directory(buffer.directory + "/" + timestamp);

        std::string disk_folder = buffer.directory + "/" + timestamp;

        logger->info("Channel 1... " + getSvissrFilename(timeReadable, "1") + ".png");
        buffer.image5.save_png(std::string(disk_folder + "/" + getSvissrFilename(timeReadable, "1") + ".png").c_str());

        logger->info("Channel 2... " + getSvissrFilename(timeReadable, "2") + ".png");
        buffer.image1.save_png(std::string(disk_folder + "/" + getSvissrFilename(timeReadable, "2") + ".png").c_str());

        logger->info("Channel 3... " + getSvissrFilename(timeReadable, "3") + ".png");
        buffer.image2.save_png(std::string(disk_folder + "/" + getSvissrFilename(timeReadable, "3") + ".png").c_str());

        logger->info("Channel 4... " + getSvissrFilename(timeReadable, "4") + ".png");
        buffer.image3.save_png(std::string(disk_folder + "/" + getSvissrFilename(timeReadable, "4") + ".png").c_str());

        logger->info("Channel 5... " + getSvissrFilename(timeReadable, "5") + ".png");
        buffer.image4.save_png(std::string(disk_folder + "/" + getSvissrFilename(timeReadable, "5") + ".png").c_str());

        // We are done with all channels but 1 and 4. Clear others to free up memory!
        buffer.image1.clear();
        buffer.image2.clear();
        buffer.image3.clear();

        // If we can, generate false color
        if (resources::resourceExists("fy2/svissr/lut.png"))
        {
            logger->trace("Scale Ch1 to 8-bits...");
            image::Image<uint16_t> channel1(buffer.image5.width(), buffer.image5.height(), 1);
            for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
                channel1[i] = buffer.image5[i] / 255;
            buffer.image5.clear(); // We're done with Ch1. Free up memory

            logger->trace("Scale Ch4 to 8-bits...");
            image::Image<uint16_t> channel5(buffer.image4.width(), buffer.image4.height(), 1);
            for (size_t i = 0; i < channel5.width() * channel5.height(); i++)
                channel5[i] = buffer.image4[i] / 255;
            buffer.image4.clear(); // We're done with Ch4. Free up memory

            logger->trace("Resize images...");
            channel5.resize(channel1.width(), channel1.height());

            logger->trace("Loading LUT...");
            image::Image<uint16_t> lutImage;
            lutImage.load_png(resources::getResourcePath("fy2/svissr/lut.png").c_str());
            lutImage.resize(256, 256);

            image::Image<uint16_t> compoImage = image::Image<uint16_t>(channel1.width(), channel1.height(), 3);

            logger->trace("Applying LUT...");
            for (size_t i = 0; i < channel1.width() * channel1.height(); i++)
            {
                uint8_t x = 255 - channel1[i];
                uint8_t y = channel5[i];

                for (int c = 0; c < 3; c++)
                    compoImage[c * compoImage.width() * compoImage.height() + i] = lutImage[c * lutImage.width() * lutImage.height() + x * lutImage.width() + y];
            }

            logger->info("False color... " + getSvissrFilename(timeReadable, "FC") + ".png");
            compoImage.save_png(std::string(disk_folder + "/" + getSvissrFilename(timeReadable, "FC") + ".png").c_str());
        }
        else
        {
            logger->warn("fy2/svissr/lut.png LUT is missing! False Color will not be generated");
        }

        writingImage = false;
    }

    SVISSRImageDecoderModule::SVISSRImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                          sat_name(parameters["satname"])
    {
        frame = new uint8_t[FRAME_SIZE];

        // Counters and so
        backwardScan = false;

        vissrImageReader.reset();
    }

    std::vector<ModuleDataType> SVISSRImageDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> SVISSRImageDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    SVISSRImageDecoderModule::~SVISSRImageDecoderModule()
    {
        delete[] frame;

        if (textureID != 0)
        {
            delete[] textureBuffer;
            deleteImageTexture(textureID);
        }
    }

    void SVISSRImageDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGE";

        if (!std::filesystem::exists(directory))
            std::filesystem::create_directory(directory);

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        time_t lastTime = 0;

        uint8_t last_status[20];
        memset(last_status, 0, 20);

        valid_lines = 0;

        bool is_live = input_data_type != DATA_FILE;

        if (is_live)
        {
            images_thread_should_run = true;
            images_queue_thread = std::thread(&SVISSRImageDecoderModule::image_saving_thread_f, this);
        }

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)frame, FRAME_SIZE);
            else
                input_fifo->read((uint8_t *)frame, FRAME_SIZE);

            // Do the actual work
            {
                // Parse counter
                int counter = frame[67] << 8 | frame[68];

                // Parse SCID
                int scid = frame[89];

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
                uint8_t is_back = most_common(&last_status[0], &last_status[20]);

                if (is_back && valid_lines > 40)
                {
                    logger->info("Full disk end detected!");

                    std::shared_ptr<SVISSRBuffer> buffer = std::make_shared<SVISSRBuffer>();

                    // Backup images
                    buffer->image1 = vissrImageReader.getImageIR1();
                    buffer->image2 = vissrImageReader.getImageIR2();
                    buffer->image3 = vissrImageReader.getImageIR3();
                    buffer->image4 = vissrImageReader.getImageIR4();
                    buffer->image5 = vissrImageReader.getImageVIS();

                    buffer->scid = most_common(scid_stats.begin(), scid_stats.end());
                    scid_stats.clear();

                    buffer->timestamp = time(0);
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

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            // Update module stats
            module_stats["full_disk_progress"] = approx_progess;

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) +
                             "%, Full Disk Progress : " + std::to_string(round(((float)approx_progess / 100.0f) * 1000.0f) / 10.0f) + "%");
            }
        }

        if (input_data_type == DATA_FILE)
            data_in.close();

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

            buffer->scid = most_common(scid_stats.begin(), scid_stats.end());
            scid_stats.clear();

            buffer->timestamp = time(0);
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

            buffer->scid = most_common(scid_stats.begin(), scid_stats.end());
            scid_stats.clear();

            buffer->timestamp = time(0);
            buffer->directory = directory;

            writeImages(*buffer);
        }
    }

    void SVISSRImageDecoderModule::drawUI(bool window)
    {
        if (textureID == 0)
        {
            textureID = makeImageTexture();
            textureBuffer = new uint32_t[2501 * 2291];
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
        {
            ImGui::Button("Full Disk Progress", {200 * ui_scale, 20 * ui_scale});
            ImGui::ProgressBar((float)approx_progess / 100.0f, ImVec2(200 * ui_scale, 20 * ui_scale));
            ImGui::Text("State : ");
            ImGui::SameLine();
            if (writingImage)
                ImGui::TextColored(IMCOLOR_SYNCED, "Writing images...");
            else if (backwardScan)
                ImGui::TextColored(IMCOLOR_NOSYNC, "Imager rollback...");
            else
                ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string SVISSRImageDecoderModule::getID()
    {
        return "fengyun_svissr_image_decoder";
    }

    std::vector<std::string> SVISSRImageDecoderModule::getParameters()
    {
        return {"satname"};
    }

    std::shared_ptr<ProcessingModule> SVISSRImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SVISSRImageDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace elektro