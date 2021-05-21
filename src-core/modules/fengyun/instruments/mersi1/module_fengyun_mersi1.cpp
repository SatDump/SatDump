#include "module_fengyun_mersi1.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "mersi_deframer.h"
#include "mersi_250m_reader.h"
#include "mersi_1000m_reader.h"
#include "mersi_correlator.h"
#include "imgui/imgui.h"
#include "common/bowtie.h"
#include "common/image.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace mersi1
    {
        FengyunMERSI1DecoderModule::FengyunMERSI1DecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                      bowtie(std::stoi(parameters["correct_bowtie"]))
        {
        }

        void FengyunMERSI1DecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-1";

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
                    std::vector<uint8_t> defraVec;
                    defraVec.insert(defraVec.end(), &buffer[14], &buffer[14 + 882]);
                    std::vector<std::vector<uint8_t>> out = mersiDefra.work(defraVec);

                    for (std::vector<uint8_t> frameVec : out)
                    {
                        int marker = (frameVec[3] % (int)pow(2, 3)) << 7 | frameVec[4] >> 1;

                        mersiCorrelator->feedFrames(marker, frameVec);

                        // Write it out
                        data_out.write((char *)frameVec.data(), frameVec.size());
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
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image1, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image1), directory + "/MERSI1-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image2, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image2), directory + "/MERSI1-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image3, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image3), directory + "/MERSI1-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image4, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image4), directory + "/MERSI1-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image5, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image5), directory + "/MERSI1-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image6, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image6), directory + "/MERSI1-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image7, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image7), directory + "/MERSI1-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image8, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image8), directory + "/MERSI1-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image9, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image9), directory + "/MERSI1-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image10, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image10), directory + "/MERSI1-10.png");

            logger->info("Channel 11...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image11, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image11), directory + "/MERSI1-11.png");

            logger->info("Channel 12...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image12, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image12), directory + "/MERSI1-12.png");

            logger->info("Channel 13...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image13, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image13), directory + "/MERSI1-13.png");

            logger->info("Channel 14...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image14, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image14), directory + "/MERSI1-14.png");

            logger->info("Channel 15...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image15, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image15), directory + "/MERSI1-15.png");

            logger->info("Channel 16...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image16, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image16), directory + "/MERSI1-16.png");

            logger->info("Channel 17...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image17, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image17), directory + "/MERSI1-17.png");

            logger->info("Channel 18...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image18, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image18), directory + "/MERSI1-18.png");

            logger->info("Channel 19...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image19, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image19), directory + "/MERSI1-19.png");

            logger->info("Channel 20...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image20, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image20), directory + "/MERSI1-20.png");

            // Output a few nice composites as well
            logger->info("221 Composite...");
            {
                cimg_library::CImg<unsigned short> image221(8192, mersiCorrelator->image1.height(), 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage2 = mersiCorrelator->image2, tempImage1 = mersiCorrelator->image1;
                    tempImage2.equalize(1000);
                    tempImage1.equalize(1000);
                    image221.draw_image(0, 0, 0, 0, tempImage2);
                    image221.draw_image(0, 0, 0, 1, tempImage2);
                    image221.draw_image(7, 0, 0, 2, tempImage1);

                    if (bowtie)
                        image221 = bowtie::correctGenericBowTie(image221, 3, scanHeight_250, alpha, beta);
                }
                WRITE_IMAGE(image221, directory + "/MERSI1-RGB-221.png");
            }

            logger->info("341 Composite...");
            {
                cimg_library::CImg<unsigned short> image341(8192, mersiCorrelator->image1.height(), 1, 3);
                image341.draw_image(24, 0, 0, 0, mersiCorrelator->image3);
                image341.draw_image(0, 0, 0, 1, mersiCorrelator->image4);
                image341.draw_image(24, 0, 0, 2, mersiCorrelator->image1);

                if (bowtie)
                    image341 = bowtie::correctGenericBowTie(image341, 3, scanHeight_250, alpha, beta);

                WRITE_IMAGE(image341, directory + "/MERSI1-RGB-341.png");
            }

            logger->info("441 Composite...");
            {
                cimg_library::CImg<unsigned short> image441(8192, mersiCorrelator->image1.height(), 1, 3);
                image441.draw_image(0, 0, 0, 0, mersiCorrelator->image4);
                image441.draw_image(0, 0, 0, 1, mersiCorrelator->image4);
                image441.draw_image(21, 0, 0, 2, mersiCorrelator->image1);

                if (bowtie)
                    image441 = bowtie::correctGenericBowTie(image441, 3, scanHeight_250, alpha, beta);

                WRITE_IMAGE(image441, directory + "/MERSI1-RGB-441.png");
            }

            logger->info("321 Composite...");
            {
                cimg_library::CImg<unsigned short> image321(8192, mersiCorrelator->image1.height(), 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage3 = mersiCorrelator->image3, tempImage2 = mersiCorrelator->image2, tempImage1 = mersiCorrelator->image1;
                    tempImage3.equalize(1000);
                    tempImage2.equalize(1000);
                    tempImage1.equalize(1000);
                    image321.draw_image(8, 0, 0, 0, tempImage3);
                    image321.draw_image(0, 0, 0, 1, tempImage2);
                    image321.draw_image(8, 0, 0, 2, tempImage1);

                    if (bowtie)
                        image321 = bowtie::correctGenericBowTie(image321, 3, scanHeight_250, alpha, beta);
                }
                WRITE_IMAGE(image321, directory + "/MERSI1-RGB-321.png");
            }

            logger->info("3(24)1 Composite...");
            {
                cimg_library::CImg<unsigned short> image3241(8192, mersiCorrelator->image1.height(), 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage4 = mersiCorrelator->image4, tempImage3 = mersiCorrelator->image3, tempImage2 = mersiCorrelator->image2, tempImage1 = mersiCorrelator->image1;
                    tempImage4.equalize(1000);
                    tempImage3.equalize(1000);
                    tempImage2.equalize(1000);
                    tempImage1.equalize(1000);
                    // Correct for offset... Kinda bad
                    image3241.draw_image(25, 0, 0, 0, tempImage3);
                    image3241.draw_image(17, 0, 0, 1, tempImage2, 0.93f + 0.5f);
                    image3241.draw_image(0, 0, 0, 1, tempImage4, 0.57f);
                    image3241.draw_image(22, 0, 0, 2, tempImage1);

                    if (bowtie)
                        image3241 = bowtie::correctGenericBowTie(image3241, 3, scanHeight_250, alpha, beta);
                }
                WRITE_IMAGE(image3241, directory + "/MERSI1-RGB-3(24)1.png");
            }

            logger->info("13.15.14 Composite...");
            {
                cimg_library::CImg<unsigned short> image131514(2048, mersiCorrelator->image15.height(), 1, 3);
                image131514.draw_image(14, 0, 0, 0, mersiCorrelator->image15);
                image131514.draw_image(0, 0, 0, 1, mersiCorrelator->image14);
                image131514.draw_image(14, 0, 0, 2, mersiCorrelator->image13);
                image131514.equalize(1000);

                if (bowtie)
                    image131514 = bowtie::correctGenericBowTie(image131514, 3, scanHeight_1000, alpha, beta);

                WRITE_IMAGE(image131514, directory + "/MERSI1-RGB-13.15.14.png");
            }
        }

        void FengyunMERSI1DecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MERSI-1 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengyunMERSI1DecoderModule::getID()
        {
            return "fengyun_mersi1";
        }

        std::vector<std::string> FengyunMERSI1DecoderModule::getParameters()
        {
            return {"correct_bowtie"};
        }

        std::shared_ptr<ProcessingModule> FengyunMERSI1DecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunMERSI1DecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mersi1
} // namespace fengyun