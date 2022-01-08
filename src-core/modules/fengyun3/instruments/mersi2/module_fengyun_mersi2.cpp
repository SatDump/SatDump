#include "module_fengyun_mersi2.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "mersi_deframer.h"
#include "mersi_correlator.h"
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/image/earth_curvature.h"
#include "modules/fengyun3/fengyun3.h"
#include "common/image/composite.h"
#include "modules/fengyun3/instruments/mersi_banding.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace mersi2
    {
        FengyunMERSI2DecoderModule::FengyunMERSI2DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                  bowtie(parameters["correct_bowtie"].get<bool>())
        {
        }

        void FengyunMERSI2DecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-2";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            logger->info("Writing MERSI frames to " + directory + "/mersi.bin");
            std::ofstream data_out(directory + "/mersi.bin", std::ios::binary);
            d_output_files.push_back(directory + "/mersi.bin");

            time_t lastTime = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            MersiDeframer mersiDefra;

            // MERSI Correlator used to fix synchronise channels in composites
            // It will recover full MERSI scans
            MERSICorrelator *mersiCorrelator = new MERSICorrelator();

            int vcidFrames = 0;

            logger->info("Demultiplexing and deframing...");

            uint8_t frame_write_buffer[12357];

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract VCID
                int vcid = buffer[5] % ((int)pow(2, 6));

                if (vcid == 3)
                {
                    vcidFrames++;

                    // Deframe MERSI
                    std::vector<std::vector<uint8_t>> out = mersiDefra.work(&buffer[14], 882);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        int marker = (frameVec[3] % (int)pow(2, 3)) << 7 | frameVec[4] >> 1;

                        mersiCorrelator->feedFrames(marker, frameVec);

                        // Write it out
                        std::fill(&frame_write_buffer[0], &frame_write_buffer[12357], 0);
                        std::memcpy(&frame_write_buffer[0], frameVec.data(), frameVec.size());
                        data_out.write((char *)&frame_write_buffer[0], 12357);
                    }
                }

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();
            data_out.close();

            logger->info("VCID 3 Frames         : " + std::to_string(vcidFrames));
            logger->info("250m Channels frames  : " + std::to_string(mersiCorrelator->m250Frames));
            logger->info("1000m Channels frames : " + std::to_string(mersiCorrelator->m1000Frames));
            logger->info("Scans                 : " + std::to_string(mersiCorrelator->complete));

            logger->info("Writing images.... (Can take a while)");

            // BowTie values
            const float alpha = 1.0 / 1.6;
            const float beta = 0.58333; //1.0 - alpha;
            const long scanHeight_250 = 40;
            const long scanHeight_1000 = 10;

            // Do it for our correlated ones
            mersiCorrelator->makeImages();

            // Write synced channels
            logger->info("Channel 1...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image1, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image1), directory + "/MERSI2-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image2, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image2), directory + "/MERSI2-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image3, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image3), directory + "/MERSI2-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image4, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image4), directory + "/MERSI2-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image5, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image5), directory + "/MERSI2-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image6, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image6), directory + "/MERSI2-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image7, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image7), directory + "/MERSI2-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image8, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image8), directory + "/MERSI2-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image9, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image9), directory + "/MERSI2-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image10, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image10), directory + "/MERSI2-10.png");

            logger->info("Channel 11...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image11, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image11), directory + "/MERSI2-11.png");

            logger->info("Channel 12...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image12, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image12), directory + "/MERSI2-12.png");

            logger->info("Channel 13...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image13, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image13), directory + "/MERSI2-13.png");

            logger->info("Channel 14...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image14, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image14), directory + "/MERSI2-14.png");

            logger->info("Channel 15...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image15, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image15), directory + "/MERSI2-15.png");

            logger->info("Channel 16...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image16, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image16), directory + "/MERSI2-16.png");

            logger->info("Channel 17...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image17, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image17), directory + "/MERSI2-17.png");

            logger->info("Channel 18...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image18, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image18), directory + "/MERSI2-18.png");

            logger->info("Channel 19...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image19, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image19), directory + "/MERSI2-19.png");

            logger->info("Channel 20...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image20, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image20), directory + "/MERSI2-20.png");

            logger->info("Channel 21...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image21, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image21), directory + "/MERSI2-21.png");

            logger->info("Channel 22...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image22, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image22), directory + "/MERSI2-22.png");

            logger->info("Channel 23...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image23, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image23), directory + "/MERSI2-23.png");

            logger->info("Channel 24...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image24, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image24), directory + "/MERSI2-24.png");

            logger->info("Channel 25...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image25, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image25), directory + "/MERSI2-25.png");

            if (mersiCorrelator->complete > 0)
            {
                // Generate composites
                for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &compokey : d_parameters["composites"].items())
                {
                    nlohmann::json compositeDef = compokey.value();

                    std::string expression = compositeDef["expression"].get<std::string>();
                    bool corrected = compositeDef.count("corrected") > 0 ? compositeDef["corrected"].get<bool>() : false;
                    bool do_banding_correct = compositeDef.count("banding_correct") > 0 ? compositeDef["banding_correct"].get<bool>() : false;
                    //bool projected = compositeDef.count("projected") > 0 ? compositeDef["projected"].get<bool>() : false;

                    std::string name = "MERSI2-" + compokey.key();

                    // Prepare what we'll need
                    std::vector<image::Image<uint16_t>> all_channels = {mersiCorrelator->image1, mersiCorrelator->image2, mersiCorrelator->image3, mersiCorrelator->image4,
                                                                        mersiCorrelator->image5, mersiCorrelator->image6, mersiCorrelator->image7, mersiCorrelator->image8,
                                                                        mersiCorrelator->image9, mersiCorrelator->image10, mersiCorrelator->image11, mersiCorrelator->image12,
                                                                        mersiCorrelator->image13, mersiCorrelator->image14, mersiCorrelator->image15, mersiCorrelator->image16,
                                                                        mersiCorrelator->image17, mersiCorrelator->image18, mersiCorrelator->image19, mersiCorrelator->image20,
                                                                        mersiCorrelator->image21, mersiCorrelator->image22, mersiCorrelator->image23, mersiCorrelator->image24,
                                                                        mersiCorrelator->image25};
                    std::vector<int> all_channel_numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};

                    // Get required channels
                    std::vector<int> requiredChannels = compositeDef["channels"].get<std::vector<int>>();

                    // Prepare them
                    std::vector<image::Image<uint16_t>> channels;
                    std::vector<int> channel_numbers;
                    for (int required_ch : requiredChannels)
                    {
                        channels.push_back(all_channels[required_ch - 1]);
                        channel_numbers.push_back(all_channel_numbers[required_ch - 1]);
                    }

                    logger->info(name + "...");
                    image::Image<uint16_t> compositeImage = image::generate_composite_from_equ<unsigned short>(channels,
                                                                                                               channel_numbers,
                                                                                                               expression,
                                                                                                               compositeDef);

                    if (do_banding_correct)
                    {
                        if (compositeImage.width() == 2048)
                            mersi::banding_correct(compositeImage, scanHeight_1000);
                        if (compositeImage.width() == 8192)
                            mersi::banding_correct(compositeImage, scanHeight_250);
                    }

                    if (bowtie)
                    {
                        if (compositeImage.width() == 2048)
                            compositeImage = image::bowtie::correctGenericBowTie(compositeImage, compositeImage.channels(), scanHeight_1000, alpha, beta);
                        if (compositeImage.width() == 8192)
                            compositeImage = image::bowtie::correctGenericBowTie(compositeImage, compositeImage.channels(), scanHeight_250, alpha, beta);
                    }

                    WRITE_IMAGE(compositeImage, directory + "/" + name + ".png");

                    if (corrected)
                    {
                        logger->info(name + "-CORRECTED...");
                        if (compositeImage.width() == 2048)
                            compositeImage = image::earth_curvature::correct_earth_curvature(compositeImage,
                                                                                             FY3_ORBIT_HEIGHT,
                                                                                             FY3_MERSI_SWATH,
                                                                                             FY3_MERSI_RES1000);
                        else if (compositeImage.width() == 8192)
                            compositeImage = image::earth_curvature::correct_earth_curvature(compositeImage,
                                                                                             FY3_ORBIT_HEIGHT,
                                                                                             FY3_MERSI_SWATH,
                                                                                             FY3_MERSI_RES250);
                        WRITE_IMAGE(compositeImage, directory + "/" + name + "-CORRECTED.png");
                    }
                }
            }
        }

        void FengyunMERSI2DecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MERSI-2 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMERSI2DecoderModule::getID()
        {
            return "fengyun_mersi2";
        }

        std::vector<std::string> FengyunMERSI2DecoderModule::getParameters()
        {
            return {"correct_bowtie"};
        }

        std::shared_ptr<ProcessingModule> FengyunMERSI2DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengyunMERSI2DecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mersi2
} // namespace fengyun