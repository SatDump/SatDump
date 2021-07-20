#include "module_gvar_image_decoder.h"
#include "logger.h"
#include "common/codings/differential/nrzs.h"
#include "imgui/imgui.h"
#include "gvar_deframer.h"
#include "gvar_derand.h"
#include "imgui/imgui_image.h"
#include <filesystem>

#define FRAME_SIZE 32786

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace gvar
    {
        std::string GVARImageDecoderModule::getGvarFilename(std::tm *timeReadable, int channel)
        {
            std::string utc_filename = sat_name + "_" + std::to_string(channel) + "_" +                                                                             // Satellite name and channel
                                       std::to_string(timeReadable->tm_year + 1900) +                                                                               // Year yyyy
                                       (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + // Month MM
                                       (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "T" +    // Day dd
                                       (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +          // Hour HH
                                       (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) +             // Minutes mm
                                       (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec)) + "Z";        // Seconds ss
            return utc_filename;
        }

        void GVARImageDecoderModule::writeImages(std::string directory)
        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));

            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait a bit
            logger->info("Full disk finished, saving at GVAR_" + timestamp + "...");

            std::filesystem::create_directory(directory + "/GVAR_" + timestamp);

            std::string disk_folder = directory + "/GVAR_" + timestamp;

            logger->info("Resizing...");
            image1.resize(image1.width(), image1.height() * 1.75);
            image2.resize(image2.width(), image2.height() * 1.75);
            image3.resize(image3.width(), image3.height() * 1.75);
            image4.resize(image4.width(), image4.height() * 1.75);
            image5.resize(image5.width(), image5.height() * 1.75);

            logger->info("Channel 1... " + getGvarFilename(timeReadable, 1) + ".png");
            image5.save_png(std::string(disk_folder + "/" + getGvarFilename(timeReadable, 1) + ".png").c_str());

            logger->info("Channel 2... " + getGvarFilename(timeReadable, 2) + ".png");
            image1.save_png(std::string(disk_folder + "/" + getGvarFilename(timeReadable, 2) + ".png").c_str());

            logger->info("Channel 3... " + getGvarFilename(timeReadable, 3) + ".png");
            image2.save_png(std::string(disk_folder + "/" + getGvarFilename(timeReadable, 3) + ".png").c_str());

            logger->info("Channel 4... " + getGvarFilename(timeReadable, 4) + ".png");
            image3.save_png(std::string(disk_folder + "/" + getGvarFilename(timeReadable, 4) + ".png").c_str());

            logger->info("Channel 5... " + getGvarFilename(timeReadable, 5) + ".png");
            image4.save_png(std::string(disk_folder + "/" + getGvarFilename(timeReadable, 5) + ".png").c_str());

            writingImage = false;
        }

        GVARImageDecoderModule::GVARImageDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                              sat_name(parameters["satname"])
        {
            frame = new uint8_t[FRAME_SIZE];
            writingImage = false;

            nonEndCount = 0;

            // Init thread pool
            imageSavingThreadPool = std::make_shared<ctpl::thread_pool>(1);
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
                deleteImageTexture(textureID);
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

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGE";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)frame, FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)frame, FRAME_SIZE);

                // Do the actual work
                {
                    // Get block number
                    int block_number = frame[8];

                    //if (block_number == 11)
                    //{
                    //   uint16_t product = frame[12] << 8 | frame[13];
                    //   logger->info(product );
                    //   if (product == 15)
                    //       testFile.write((char *)&frame.data()[8], 32786 - 8);
                    // }

                    // IR Channels 1-2 (Reader 1)
                    if (block_number == 1)
                    {
                        // Read counter
                        uint16_t counter = frame[105] << 6 | frame[106] >> 2;

                        // Safeguard
                        if (counter > 1353)
                            continue;

                        // Easy way of showing an approximate progress percentage
                        approx_progess = round(((float)counter / 1353.0f) * 1000.0f) / 10.0f;

                        // Process it!
                        infraredImageReader1.pushFrame(frame, counter);
                    }
                    // IR Channels 3-4 (Reader 2)
                    else if (block_number == 2)
                    {
                        // Read counter
                        uint16_t counter = frame[105] << 6 | frame[106] >> 2;

                        // Safeguard
                        if (counter > 1353)
                            continue;

                        // Process it!
                        infraredImageReader2.pushFrame(frame, counter);
                    }
                    // VIS 1
                    else if (block_number >= 3 && block_number <= 10)
                    {
                        //testFile.write((char *)&frame.data()[8], 32786 - 8);
                        // Read counter
                        uint16_t counter = frame[105] << 6 | frame[106] >> 2;

                        // Safeguard
                        if (counter > 1353)
                            continue;

                        visibleImageReader.pushFrame(frame, block_number, counter);

                        if (counter == 1353)
                        {
                            logger->info("Full disk end detected!");

                            if (!writingImage && (nonEndCount > 20 || input_data_type == DATA_STREAM))
                            {
                                writingImage = true;

                                // Backup images
                                image1 = infraredImageReader1.getImage1();
                                image2 = infraredImageReader1.getImage2();
                                image3 = infraredImageReader2.getImage1();
                                image4 = infraredImageReader2.getImage2();
                                image5 = visibleImageReader.getImage();

                                // Write those
                                imageSavingThreadPool->push([directory, this](int)
                                                            { writeImages(directory); });

                                // Reset readers
                                infraredImageReader1.startNewFullDisk();
                                infraredImageReader2.startNewFullDisk();
                                visibleImageReader.startNewFullDisk();
                            }
                            else if (input_data_type == DATA_FILE)
                            {
                                while (writingImage)
                                    std::this_thread::sleep_for(std::chrono::seconds(1));
                            }

                            nonEndCount = 0;
                        }
                        else
                        {
                            nonEndCount++;
                        }
                    }
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

            logger->info("Wait for in-progress images...");
            while (writingImage)
                std::this_thread::sleep_for(std::chrono::seconds(1));

            logger->info("Dump remaining data...");
            if (input_data_type == DATA_FILE)
            {
                writingImage = true;

                // Backup images
                image1 = infraredImageReader1.getImage1();
                image2 = infraredImageReader1.getImage2();
                image3 = infraredImageReader2.getImage1();
                image4 = infraredImageReader2.getImage2();
                image5 = visibleImageReader.getImage();

                // Write those
                writeImages(directory);
            }

            imageSavingThreadPool->stop();
        }

        void GVARImageDecoderModule::drawUI(bool window)
        {
            if (textureID == 0)
            {
                textureID = makeImageTexture();
                textureBuffer = new uint32_t[1354 * 2 * 5206];
            }

            ImGui::Begin("GVAR Image Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            // This is outer crap...
            ImGui::BeginGroup();
            {
                ushort_to_rgba(infraredImageReader1.imageBuffer1, textureBuffer, 5206 * 1354 * 2);
                updateImageTexture(textureID, textureBuffer, 5206, 1354 * 2);
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
                else
                    ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
            }
            ImGui::EndGroup();

            if (!streamingInput)
                ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GVARImageDecoderModule::getID()
        {
            return "goes_gvar_image_decoder";
        }

        std::vector<std::string> GVARImageDecoderModule::getParameters()
        {
            return {"satname"};
        }

        std::shared_ptr<ProcessingModule> GVARImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<GVARImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace elektro
}