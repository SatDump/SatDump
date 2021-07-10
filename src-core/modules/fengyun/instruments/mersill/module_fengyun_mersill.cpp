#include "module_fengyun_mersill.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "mersi_deframer.h"
#include "mersi_correlator.h"
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/image/image.h"
#include "common/image/earth_curvature.h"
#include "modules/fengyun/fengyun3.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace mersill
    {
        FengyunMERSILLDecoderModule::FengyunMERSILLDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                        bowtie(std::stoi(parameters["correct_bowtie"]))
        {
        }

        void FengyunMERSILLDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-LL";

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
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image1, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image1), directory + "/MERSILL-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image2, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image2), directory + "/MERSILL-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image3, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image3), directory + "/MERSILL-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image4, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image4), directory + "/MERSILL-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image5, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image5), directory + "/MERSILL-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image6, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image6), directory + "/MERSILL-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image7, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image7), directory + "/MERSILL-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image8, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image8), directory + "/MERSILL-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image9, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image9), directory + "/MERSILL-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image10, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image10), directory + "/MERSILL-10.png");

            logger->info("Channel 11...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image11, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image11), directory + "/MERSILL-11.png");

            logger->info("Channel 12...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image12, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image12), directory + "/MERSILL-12.png");

            logger->info("Channel 13...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image13, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image13), directory + "/MERSILL-13.png");

            logger->info("Channel 14...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image14, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image14), directory + "/MERSILL-14.png");

            logger->info("Channel 15...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image15, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image15), directory + "/MERSILL-15.png");

            logger->info("Channel 16...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image16, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image16), directory + "/MERSILL-16.png");

            logger->info("Channel 17...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image17, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image17), directory + "/MERSILL-17.png");

            logger->info("Channel 18...");
            WRITE_IMAGE((bowtie ? image::bowtie::correctGenericBowTie(mersiCorrelator->image18, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image18), directory + "/MERSILL-18.png");

            logger->info("388 Composite...");
            {
                cimg_library::CImg<unsigned short> image388(1536, mersiCorrelator->image17.height(), 1, 3);
                image388.draw_image(0, 0, 0, 0, mersiCorrelator->image3);
                image388.draw_image(6, 0, 0, 1, mersiCorrelator->image8);
                image388.draw_image(6, 0, 0, 2, mersiCorrelator->image8);
                image388.normalize(0, std::numeric_limits<unsigned char>::max());
                image388.equalize(1000);

                if (bowtie)
                    image388 = image::bowtie::correctGenericBowTie(image388, 3, scanHeight_1000, alpha, beta);

                WRITE_IMAGE(image388, directory + "/MERSILL-RGB-388.png");
                cimg_library::CImg<unsigned short> corrected388 = image::earth_curvature::correct_earth_curvature(image388,
                                                                                                                  FY3_ORBIT_HEIGHT,
                                                                                                                  FY3_MERSILL_SWATH,
                                                                                                                  FY3_MERSI_RES1000);
                WRITE_IMAGE(corrected388, directory + "/MERSILL-RGB-388-CORRECTED.png");
            }

            logger->info("838 Composite...");
            {
                cimg_library::CImg<unsigned short> image8838(1536, mersiCorrelator->image17.height(), 1, 3);
                image8838.draw_image(6, 0, 0, 0, mersiCorrelator->image8);
                image8838.draw_image(0, 0, 0, 1, mersiCorrelator->image3);
                image8838.draw_image(6, 0, 0, 2, mersiCorrelator->image8);
                image8838.normalize(0, std::numeric_limits<unsigned char>::max());
                image8838.equalize(1000);

                if (bowtie)
                    image8838 = image::bowtie::correctGenericBowTie(image8838, 3, scanHeight_1000, alpha, beta);

                WRITE_IMAGE(image8838, directory + "/MERSILL-RGB-838.png");
                cimg_library::CImg<unsigned short> corrected838 = image::earth_curvature::correct_earth_curvature(image8838,
                                                                                                                  FY3_ORBIT_HEIGHT,
                                                                                                                  FY3_MERSILL_SWATH,
                                                                                                                  FY3_MERSI_RES1000);
                WRITE_IMAGE(corrected838, directory + "/MERSILL-RGB-838-CORRECTED.png");
            }
        }

        void FengyunMERSILLDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MERSI-LL Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMERSILLDecoderModule::getID()
        {
            return "fengyun_mersill";
        }

        std::vector<std::string> FengyunMERSILLDecoderModule::getParameters()
        {
            return {"correct_bowtie"};
        }

        std::shared_ptr<ProcessingModule> FengyunMERSILLDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunMERSILLDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mersi2
} // namespace fengyun