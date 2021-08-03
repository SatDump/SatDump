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

#define FRAME_SIZE 32786

// Return filesize
size_t getFilesize(std::string filepath);

namespace goes
{
    namespace gvar
    {
        std::string GVARImageDecoderModule::getGvarFilename(int sat_number, std::tm *timeReadable, int channel)
        {
            std::string utc_filename = "G" + std::to_string(sat_number) + "_" + std::to_string(channel) + "_" +                                                     // Satellite name and channel
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
            isSavingInProgress = true;
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));

            // Get stats. This is done over a lot of data to allow decoding at low SNR
            int sat_number = most_common(scid_stats.begin(), scid_stats.end());
            int vis_width = most_common(vis_width_stats.begin(), vis_width_stats.end());

            std::string dir_name = "GOES-" + std::to_string(sat_number) + "/" + timestamp;
            logger->info("Full disk finished, saving at " + dir_name + "...");

            std::filesystem::create_directories(directory + "/" + dir_name);

            std::string disk_folder = directory + "/" + dir_name;

            logger->info("Resizing...");
            image1.resize(image1.width(), image1.height() * 1.75);
            image2.resize(image2.width(), image2.height() * 1.75);
            image3.resize(image3.width(), image3.height() * 1.75);
            image4.resize(image4.width(), image4.height() * 1.75);
            image5.resize(image5.width(), image5.height() * 1.75);

            // VIS-1 height
            int vis_height = image5.height();

            if (vis_width == 13216) // Some partial scan
                vis_height = 9500;
            else if (vis_width == 11416) // Some partial scan
                vis_height = 6895;
            else if (vis_width == 20836) // Full disk
                vis_height = 20836;
            else if (vis_width == 20824) // Full disk
                vis_height = 20824;

            // IR height
            int ir1_width = vis_width / 4;
            int ir1_height = vis_height / 4;

            logger->info("Cropping to transmited size...");
            logger->debug("VIS 1 size " + std::to_string(vis_width) + "x" + std::to_string(vis_height));
            image5.crop(0, 0, vis_width - 1, vis_height - 1);
            logger->debug("IR size " + std::to_string(ir1_width) + "x" + std::to_string(ir1_height));
            image1.crop(0, 0, ir1_width - 1, ir1_height - 1);
            image2.crop(0, 0, ir1_width - 1, ir1_height - 1);
            image3.crop(0, 0, ir1_width - 1, ir1_height - 1);
            image4.crop(0, 0, ir1_width - 1, ir1_height - 1);

            logger->info("Channel 1... " + getGvarFilename(sat_number, timeReadable, 1) + ".png");
            image5.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, 1) + ".png").c_str());

            logger->info("Channel 2... " + getGvarFilename(sat_number, timeReadable, 2) + ".png");
            image1.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, 2) + ".png").c_str());

            logger->info("Channel 3... " + getGvarFilename(sat_number, timeReadable, 3) + ".png");
            image2.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, 3) + ".png").c_str());

            logger->info("Channel 4... " + getGvarFilename(sat_number, timeReadable, 4) + ".png");
            image3.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, 4) + ".png").c_str());

            logger->info("Channel 5... " + getGvarFilename(sat_number, timeReadable, 5) + ".png");
            image4.save_png(std::string(disk_folder + "/" + getGvarFilename(sat_number, timeReadable, 5) + ".png").c_str());

            isImageInProgress = false;
            scid_stats.clear();
            vis_width_stats.clear();
            ir_width_stats.clear();
            isSavingInProgress = false;
        }

        GVARImageDecoderModule::GVARImageDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            frame = new uint8_t[FRAME_SIZE];
            isImageInProgress = false;

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

            std::ofstream output_file("test.gvar");

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)frame, FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)frame, FRAME_SIZE);

                // Parse main header
                PrimaryBlockHeader block_header = *((PrimaryBlockHeader *)&frame[8]);

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
                                    // Backup images
                                    image1 = infraredImageReader1.getImage1();
                                    image2 = infraredImageReader1.getImage2();
                                    image3 = infraredImageReader2.getImage1();
                                    image4 = infraredImageReader2.getImage2();
                                    image5 = visibleImageReader.getImage();

                                    // Write those
                                    writeImages(directory);

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

            logger->info("Dump remaining data...");
            if (isImageInProgress)
            {

                // Backup images
                image1 = infraredImageReader1.getImage1();
                image2 = infraredImageReader1.getImage2();
                image3 = infraredImageReader2.getImage1();
                image4 = infraredImageReader2.getImage2();
                image5 = visibleImageReader.getImage();

                // Write those
                writeImages(directory);
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

        std::shared_ptr<ProcessingModule> GVARImageDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<GVARImageDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace elektro
}