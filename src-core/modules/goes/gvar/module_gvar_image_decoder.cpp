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
#include "crc_table.h"

#define FRAME_SIZE 32786

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace gvar
    {

         // CRC Implementation from LRIT-Missin-Specific-Document.pdf
        uint16_t computeCRC(const uint8_t *data, int size)
        {
            uint16_t crc = 0xffff;
            for (int i = 0; i < size; i++)
                crc = (crc << 8) ^ crc_table[(crc >> 8) ^ (uint16_t)data[i]];
            return crc;
        }       
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

            //logger->trace("VIS 1 size before " + std::to_string(images.image5.width()) + "x" + std::to_string(images.image5.height()));
            //logger->trace("IR size before " + std::to_string(images.image1.width()) + "x" + std::to_string(images.image1.height()));

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

            logger->info("Channel 1... " + getGvarFilename(sat_number, timeReadable, "1") + ".png");
            images.image5.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "1") + ".png").c_str(), false);

            logger->info("Channel 2... " + getGvarFilename(sat_number, timeReadable, "2") + ".png");
            images.image1.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "2") + ".png").c_str());

            logger->info("Channel 3... " + getGvarFilename(sat_number, timeReadable, "3") + ".png");
            images.image2.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "3") + ".png").c_str());

            logger->info("Channel 4... " + getGvarFilename(sat_number, timeReadable, "4") + ".png");
            images.image3.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "4") + ".png").c_str());

            logger->info("Channel 5... " + getGvarFilename(sat_number, timeReadable, "5") + ".png");
            images.image4.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "5") + ".png").c_str());

            // Let plugins do something
            satdump::eventBus->fire_event<events::GVARSaveChannelImagesEvent>({images, timeReadable, timevalue, disk_folder});

            // We are done with all channels but 1 and 4. Clear others to free up memory!
            images.image1.clear();
            images.image2.clear();
            images.image4.clear();

            // If we can, generate false color
            if (resources::resourceExists("goes/gvar/lut.png"))
            {
                logger->trace("Scale Ch1 to 8-bits...");
                image::Image<uint8_t> channel1(images.image5.width(), images.image5.height(), 1);
                for (int i = 0; i < channel1.width() * channel1.height(); i++)
                    channel1[i] = images.image5[i] / 255;
                images.image5.clear(); // We're done with Ch1. Free up memory

                logger->trace("Scale Ch4 to 8-bits...");
                image::Image<uint8_t> channel4(images.image3.width(), images.image3.height(), 1);
                for (int i = 0; i < channel4.width() * channel4.height(); i++)
                    channel4[i] = images.image3[i] / 255;
                images.image3.clear(); // We're done with Ch4. Free up memory

                logger->trace("Resize images...");
                channel4.resize(channel1.width(), channel1.height());

                logger->trace("Loading LUT...");
                image::Image<uint8_t> lutImage;
                lutImage.load_png(resources::getResourcePath("goes/gvar/lut.png").c_str());
                lutImage.resize(256, 256);

                logger->trace("Loading correction curve...");
                image::Image<uint8_t> curveImage;
                curveImage.load_png(resources::getResourcePath("goes/gvar/curve_goesn.png").c_str());

                image::Image<uint8_t> compoImage = image::Image<uint8_t>(channel1.width(), channel1.height(), 3);

                logger->trace("Applying LUT...");
                for (int i = 0; i < channel1.width() * channel1.height(); i++)
                {
                    uint8_t x = 255 - curveImage[channel1[i]] / 1.5;
                    uint8_t y = channel4[i];

                    for (int c = 0; c < 3; c++)
                        compoImage[c * compoImage.width() * compoImage.height() + i] = lutImage[c * lutImage.width() * lutImage.height() + x * lutImage.width() + y];
                }

                logger->trace("Contrast correction...");
                image::brightness_contrast(compoImage, -10.0f / 127.0f, 24.0f / 127.0f);

                logger->trace("Hue shift...");
                image::HueSaturation hueTuning;
                hueTuning.hue[image::HUE_RANGE_MAGENTA] = 133.0 / 180.0;
                hueTuning.overlap = 100.0 / 100.0;
                image::hue_saturation(compoImage, hueTuning);

                logger->info("False color... " + getGvarFilename(sat_number, timeReadable, "FC") + ".png");
                compoImage.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, "FC") + ".png").c_str());

                // Let plugins do something
                satdump::eventBus->fire_event<events::GVARSaveFCImageEvent>({compoImage, images.sat_number, timeReadable, timevalue, disk_folder});
            }
            else
            {
                logger->warn("goes/gvar/lut.png LUT is missing! False Color will not be generated");
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

            nonEndCount = 0;
            endCount = 0;
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
                setThreadPriority(imageSavingThread, 1); // Low priority to avoid sampledrop
            }

            directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IMAGE";

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

                // Parse main header
                std::vector<uint16_t> block_ids;
                PrimaryBlockHeader block_header = *((PrimaryBlockHeader *)&frame[8]);
                block_header.header_crc=~block_header.header_crc;
                if(computeCRC((uint8_t*)&block_header,30)!=0)
                {//CRC failed for first header, try second header
                    block_ids.push_back(block_header.block_id);
                    block_header = *((PrimaryBlockHeader *)&frame[8+30]);
                    block_header.header_crc=~block_header.header_crc;
                    if(computeCRC((uint8_t*)&block_header,30)!=0)
                    {//CRC failed for second header, try third header
                        block_ids.push_back(block_header.block_id);
                        block_header = *((PrimaryBlockHeader *)&frame[8+60]);
                        block_header.header_crc=~block_header.header_crc;
                        if(computeCRC((uint8_t*)&block_header,30)!=0)
                        {//All headers failed CRC. Use best of 3.
                            block_ids.push_back(block_header.block_id);
                            block_header.block_id=most_common(block_ids.begin(),block_ids.end());
                        }
                    }
                }

                // Is this imagery? Blocks 1 to 10 are imagery
                if (block_header.block_id >= 1 && block_header.block_id <= 10)
                {
                    // This is imagery, so we can parse the line information header
                    LineDocumentationHeader line_header(&frame[8 + 30 * 3]);

                    // SCID Stats
                    scid_stats.push_back(line_header.sc_id);

                    // Internal line counter should NEVER be over 1974. Discard frame if it is
                    // Though we know the max in normal operations is 1354 for a full disk....
                    // So we use that
                    if (line_header.relative_scan_count > 1354)
                        continue;

                    // Is this VIS Channel 1?
                    if (block_header.block_id >= 3 && block_header.block_id <= 10)
                    {
                        // Push width stats
                        vis_width_stats.push_back(line_header.pixel_count);

                        // Push into decoder
                        visibleImageReader.pushFrame(frame, block_header.block_id, line_header.relative_scan_count);

                        // Detect full disk end
                        if (line_header.relative_scan_count < 2)
                        {
                            nonEndCount = 0;
                            endCount += 2;

                            if (endCount > 6)
                            {
                                logger->info("Image start detected!");

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
                                                                most_common(scid_stats.begin(), scid_stats.end()),
                                                                most_common(vis_width_stats.begin(), vis_width_stats.end())});
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
                                                             most_common(scid_stats.begin(), scid_stats.end()),
                                                             most_common(vis_width_stats.begin(), vis_width_stats.end())};
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

                                endCount = 0;
                            }
                        }
                        else
                        {
                            if (endCount > 0)
                                endCount--;

                            nonEndCount++;

                            if (nonEndCount > 100 && !isImageInProgress)
                                isImageInProgress = true;
                        }
                    }
                    // Is this IR?
                    else if (block_header.block_id == 1 || block_header.block_id == 2)
                    {
                        // Easy way of showing an approximate progress percentage
                        approx_progess = round(((float)line_header.relative_scan_count / 1353.0f) * 1000.0f) / 10.0f;

                        // Push frame size stats
                        ir_width_stats.push_back(line_header.word_count);

                        // Get current stats
                        int current_words = most_common(ir_width_stats.begin(), ir_width_stats.end());

                        // Safeguard
                        if (current_words > 6565)
                            current_words = 6530; // Default to fulldisk size

                        // Is this IR Channel 1-2?
                        if (block_header.block_id == 1)
                            infraredImageReader1.pushFrame(&frame[8 + 30 * 3], line_header.relative_scan_count, current_words);
                        else if (block_header.block_id == 2)
                            infraredImageReader2.pushFrame(&frame[8 + 30 * 3], line_header.relative_scan_count, current_words);
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
                                     most_common(scid_stats.begin(), scid_stats.end()),
                                     most_common(vis_width_stats.begin(), vis_width_stats.end())};
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
            }

            ImGui::Begin("GVAR Image Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

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
                    ImGui::TextColored(IMCOLOR_SYNCED, "Writing images...");
                else if (isImageInProgress)
                    ImGui::TextColored(IMCOLOR_SYNCING, "Receiving...");
                else
                    ImGui::TextColored(IMCOLOR_NOSYNC, "IDLE");
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
            return {};
        }

        std::shared_ptr<ProcessingModule> GVARImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GVARImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace elektro
}