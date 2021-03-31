#include "module_fengyun_mersi2.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "mersi_deframer.h"
#include "mersi_250m_reader.h"
#include "mersi_1000m_reader.h"
#include "mersi_correlator.h"
#include "imgui/imgui.h"
#include "modules/common/bowtie.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    namespace mersi2
    {
        FengyunMERSI2DecoderModule::FengyunMERSI2DecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                      bowtie(std::stoi(parameters["correct_bowtie"]))
        {
        }

        void FengyunMERSI2DecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-2";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            // Read buffer
            uint8_t buffer[1024];

            // Deframer
            MersiDeframer mersiDefra;

            // MERSI Correlator used to fix synchronise channels in composites
            // It will only preserve full MERSI scans
            MERSICorrelator *mersiCorrelator = new MERSICorrelator();

            // MERSI Readers
            MERSI250Reader reader1, reader2, reader3, reader4, reader5, reader6;
            MERSI1000Reader reader7, reader8, reader9, reader10, reader11, reader12, reader13, reader14,
                reader15, reader16, reader17, reader18, reader19, reader20, reader21, reader22, reader23, reader24, reader25;

            int vcidFrames = 0;
            int m1000Frames = 0;
            int m250Frames = 0;

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

                        //logger->info(marker );
                        if (marker >= 240)
                        {
                            m1000Frames++;

                            marker -= 240;

                            // Demultiplex them all!
                            if (marker < 10 * 1)
                                reader7.pushFrame(frameVec);
                            else if (marker >= 10 * 1 && marker < 10 * 2)
                                reader8.pushFrame(frameVec);
                            else if (marker >= 10 * 2 && marker < 10 * 3)
                                reader9.pushFrame(frameVec);
                            else if (marker >= 10 * 3 && marker < 10 * 4)
                                reader10.pushFrame(frameVec);
                            else if (marker >= 10 * 4 && marker < 10 * 5)
                                reader11.pushFrame(frameVec);
                            else if (marker >= 10 * 5 && marker < 10 * 6)
                                reader12.pushFrame(frameVec);
                            else if (marker >= 10 * 6 && marker < 10 * 7)
                                reader13.pushFrame(frameVec);
                            else if (marker >= 10 * 7 && marker < 10 * 8)
                                reader14.pushFrame(frameVec);
                            else if (marker >= 10 * 8 && marker < 10 * 9)
                                reader15.pushFrame(frameVec);
                            else if (marker >= 10 * 9 && marker < 10 * 10)
                                reader16.pushFrame(frameVec);
                            else if (marker >= 10 * 10 && marker < 10 * 11)
                                reader17.pushFrame(frameVec);
                            else if (marker >= 10 * 11 && marker < 10 * 12)
                                reader18.pushFrame(frameVec);
                            else if (marker >= 10 * 12 && marker < 10 * 13)
                                reader19.pushFrame(frameVec);
                            else if (marker >= 10 * 13 && marker < 10 * 14)
                                reader20.pushFrame(frameVec);
                            else if (marker >= 10 * 14 && marker < 10 * 15)
                                reader21.pushFrame(frameVec);
                            else if (marker >= 10 * 15 && marker < 10 * 16)
                                reader22.pushFrame(frameVec);
                            else if (marker >= 10 * 16 && marker < 10 * 17)
                                reader23.pushFrame(frameVec);
                            else if (marker >= 10 * 17 && marker < 10 * 18)
                                reader24.pushFrame(frameVec);
                            else if (marker >= 10 * 18 && marker < 10 * 19)
                                reader25.pushFrame(frameVec);
                        }
                        else if (marker < 240)
                        {
                            m250Frames++;

                            // Demux those lonely 250m ones
                            if (marker < 40 * 1)
                                reader1.pushFrame(frameVec);
                            else if (marker >= 40 * 1 && marker < 40 * 2)
                                reader2.pushFrame(frameVec);
                            else if (marker >= 40 * 2 && marker < 40 * 3)
                                reader3.pushFrame(frameVec);
                            else if (marker >= 40 * 3 && marker < 40 * 4)
                                reader4.pushFrame(frameVec);
                            else if (marker >= 40 * 4 && marker < 40 * 5)
                                reader5.pushFrame(frameVec);
                            else if (marker >= 40 * 5 && marker < 40 * 6)
                                reader6.pushFrame(frameVec);
                        }
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

            logger->info("VCID 3 Frames         : " + std::to_string(vcidFrames));
            logger->info("250m Channels frames  : " + std::to_string(m250Frames));
            logger->info("1000m Channels frames : " + std::to_string(m1000Frames));
            logger->info("Complete scans        : " + std::to_string(mersiCorrelator->complete));
            logger->info("Incomplete scans      : " + std::to_string(mersiCorrelator->incomplete));

            logger->info("Writing images.... (Can take a while)");

            // BowTie values
            const float alpha = 1.0 / 1.6;
            const float beta = 0.58333; //1.0 - alpha;
            const long scanHeight_250 = 40;
            const long scanHeight_1000 = 10;

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image1 = reader1.getImage();
            cimg_library::CImg<unsigned short> image2 = reader2.getImage();
            cimg_library::CImg<unsigned short> image3 = reader3.getImage();
            cimg_library::CImg<unsigned short> image4 = reader4.getImage();
            cimg_library::CImg<unsigned short> image5 = reader5.getImage();
            cimg_library::CImg<unsigned short> image6 = reader6.getImage();
            cimg_library::CImg<unsigned short> image7 = reader7.getImage();
            cimg_library::CImg<unsigned short> image8 = reader8.getImage();
            cimg_library::CImg<unsigned short> image9 = reader9.getImage();
            cimg_library::CImg<unsigned short> image10 = reader10.getImage();
            cimg_library::CImg<unsigned short> image11 = reader11.getImage();
            cimg_library::CImg<unsigned short> image12 = reader12.getImage();
            cimg_library::CImg<unsigned short> image13 = reader13.getImage();
            cimg_library::CImg<unsigned short> image14 = reader14.getImage();
            cimg_library::CImg<unsigned short> image15 = reader15.getImage();
            cimg_library::CImg<unsigned short> image16 = reader16.getImage();
            cimg_library::CImg<unsigned short> image17 = reader17.getImage();
            cimg_library::CImg<unsigned short> image18 = reader18.getImage();
            cimg_library::CImg<unsigned short> image19 = reader19.getImage();
            cimg_library::CImg<unsigned short> image20 = reader20.getImage();
            cimg_library::CImg<unsigned short> image21 = reader21.getImage();
            cimg_library::CImg<unsigned short> image22 = reader22.getImage();
            cimg_library::CImg<unsigned short> image23 = reader23.getImage();
            cimg_library::CImg<unsigned short> image24 = reader24.getImage();
            cimg_library::CImg<unsigned short> image25 = reader25.getImage();

            // Do it for our correlated ones
            mersiCorrelator->makeImages();

            // They all need to be flipped horizontally
            image1.mirror('y');
            image2.mirror('y');
            image3.mirror('y');
            image4.mirror('y');
            image5.mirror('y');
            image6.mirror('y');
            image7.mirror('y');
            image8.mirror('y');
            image9.mirror('y');
            image10.mirror('y');
            image11.mirror('y');
            image12.mirror('y');
            image13.mirror('y');
            image14.mirror('y');
            image15.mirror('y');
            image16.mirror('y');
            image17.mirror('y');
            image18.mirror('y');
            image19.mirror('y');
            image20.mirror('y');
            image21.mirror('y');
            image22.mirror('y');
            image23.mirror('y');
            image24.mirror('y');
            image25.mirror('y');

            // Takes a while so we say how we're doing
            logger->info("Channel 1...");
            WRITE_IMAGE(image1, directory + "/MERSI2-1.png");

            logger->info("Channel 2...");
            WRITE_IMAGE(image2, directory + "/MERSI2-2.png");

            logger->info("Channel 3...");
            WRITE_IMAGE(image3, directory + "/MERSI2-3.png");

            logger->info("Channel 4...");
            WRITE_IMAGE(image4, directory + "/MERSI2-4.png");

            logger->info("Channel 5...");
            WRITE_IMAGE(image5, directory + "/MERSI2-5.png");

            logger->info("Channel 6...");
            WRITE_IMAGE(image6, directory + "/MERSI2-6.png");

            logger->info("Channel 7...");
            WRITE_IMAGE(image7, directory + "/MERSI2-7.png");

            logger->info("Channel 8...");
            WRITE_IMAGE(image8, directory + "/MERSI2-8.png");

            logger->info("Channel 9...");
            WRITE_IMAGE(image9, directory + "/MERSI2-9.png");

            logger->info("Channel 10...");
            WRITE_IMAGE(image10, directory + "/MERSI2-10.png");

            logger->info("Channel 11...");
            WRITE_IMAGE(image11, directory + "/MERSI2-11.png");

            logger->info("Channel 12...");
            WRITE_IMAGE(image12, directory + "/MERSI2-12.png");

            logger->info("Channel 13...");
            WRITE_IMAGE(image13, directory + "/MERSI2-13.png");

            logger->info("Channel 14...");
            WRITE_IMAGE(image14, directory + "/MERSI2-14.png");

            logger->info("Channel 15...");
            WRITE_IMAGE(image15, directory + "/MERSI2-15.png");

            logger->info("Channel 16...");
            WRITE_IMAGE(image16, directory + "/MERSI2-16.png");

            logger->info("Channel 17...");
            WRITE_IMAGE(image17, directory + "/MERSI2-17.png");

            logger->info("Channel 18...");
            WRITE_IMAGE(image18, directory + "/MERSI2-18.png");

            logger->info("Channel 19...");
            WRITE_IMAGE(image19, directory + "/MERSI2-19.png");

            logger->info("Channel 20...");
            WRITE_IMAGE(image20, directory + "/MERSI2-20.png");

            logger->info("Channel 21...");
            WRITE_IMAGE(image21, directory + "/MERSI2-21.png");

            logger->info("Channel 22...");
            WRITE_IMAGE(image22, directory + "/MERSI2-22.png");

            logger->info("Channel 23...");
            WRITE_IMAGE(image23, directory + "/MERSI2-23.png");

            logger->info("Channel 24...");
            WRITE_IMAGE(image24, directory + "/MERSI2-24.png");

            logger->info("Channel 25...");
            WRITE_IMAGE(image25, directory + "/MERSI2-25.png");

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
                WRITE_IMAGE(image221, directory + "/MERSI2-RGB-221.png");
            }

            logger->info("341 Composite...");
            {
                cimg_library::CImg<unsigned short> image341(8192, mersiCorrelator->image1.height(), 1, 3);
                image341.draw_image(24, 0, 0, 0, mersiCorrelator->image3);
                image341.draw_image(0, 0, 0, 1, mersiCorrelator->image4);
                image341.draw_image(24, 0, 0, 2, mersiCorrelator->image1);

                if (bowtie)
                    image341 = bowtie::correctGenericBowTie(image341, 3, scanHeight_250, alpha, beta);

                WRITE_IMAGE(image341, directory + "/MERSI2-RGB-341.png");
            }

            logger->info("441 Composite...");
            {
                cimg_library::CImg<unsigned short> image441(8192, mersiCorrelator->image1.height(), 1, 3);
                image441.draw_image(0, 0, 0, 0, mersiCorrelator->image4);
                image441.draw_image(0, 0, 0, 1, mersiCorrelator->image4);
                image441.draw_image(21, 0, 0, 2, mersiCorrelator->image1);

                if (bowtie)
                    image441 = bowtie::correctGenericBowTie(image441, 3, scanHeight_250, alpha, beta);

                WRITE_IMAGE(image441, directory + "/MERSI2-RGB-441.png");
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
                WRITE_IMAGE(image321, directory + "/MERSI2-RGB-321.png");
            }

            /*logger->info("321 Composite..." );
             cimg_library::CImg<unsigned short> image321t(8192, std::max(image3.height(), std::max(image2.height(), image3.height())), 1, 3);
              {
                cimg_library::CImg<unsigned short> tempImage3 = image6, tempImage2 = image5, tempImage1 = image4;
                tempImage3.equalize(1000);
               tempImage2.equalize(1000);
               tempImage1.equalize(1000);
               image321t.draw_image(0, 0, 0, 0, tempImage3);
               image321t.draw_image(0, 0, 0, 1, tempImage2);
               image321t.draw_image(0, 0, 0, 2, tempImage1);
              }
               WRITE_IMAGE(image321t, directory + "/MERSI2-RGB-654.png");*/

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
                    image3241.draw_image(26, 0, 0, 2, tempImage1);

                    if (bowtie)
                        image3241 = bowtie::correctGenericBowTie(image3241, 3, scanHeight_250, alpha, beta);
                }
                WRITE_IMAGE(image3241, directory + "/MERSI2-RGB-3(24)1.png");
            }

            logger->info("17.19.18 Composite...");
            {
                cimg_library::CImg<unsigned short> image171918(2048, mersiCorrelator->image17.height(), 1, 3);
                image171918.draw_image(2, 0, 0, 0, mersiCorrelator->image17);
                image171918.draw_image(0, 0, 0, 1, mersiCorrelator->image19);
                image171918.draw_image(7, 0, 0, 2, mersiCorrelator->image18);
                image171918.normalize(0, std::numeric_limits<unsigned char>::max());

                if (bowtie)
                    image171918 = bowtie::correctGenericBowTie(image171918, 3, scanHeight_1000, alpha, beta);

                WRITE_IMAGE(image171918, directory + "/MERSI2-RGB-18.19.17.png");
            }

            logger->info("9.12.11 Composite...");
            {
                cimg_library::CImg<unsigned short> image91211(2048, mersiCorrelator->image11.height(), 1, 3);
                {
                    cimg_library::CImg<unsigned short> tempImage9 = mersiCorrelator->image9, tempImage11 = mersiCorrelator->image11, tempImage12 = mersiCorrelator->image12;
                    tempImage9.equalize(1000);
                    for (unsigned short &px : tempImage9)
                        px = std::numeric_limits<unsigned short>::max() - px;
                    tempImage12.equalize(1000);
                    for (unsigned short &px : tempImage12)
                        px = std::numeric_limits<unsigned short>::max() - px;
                    tempImage11.equalize(1000);
                    for (unsigned short &px : tempImage11)
                        px = std::numeric_limits<unsigned short>::max() - px;
                    image91211.draw_image(11, 0, 0, 0, tempImage9);
                    image91211.draw_image(0, 0, 0, 1, tempImage12);
                    image91211.draw_image(0, 0, 0, 2, tempImage11);

                    if (bowtie)
                        image91211 = bowtie::correctGenericBowTie(image91211, 3, scanHeight_1000, alpha, beta);
                }
                WRITE_IMAGE(image91211, directory + "/MERSI2-RGB-9.12.11.png");
            }

            // Write synced channels
            logger->info("Channel 1 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image1, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image1), directory + "/MERSI2-SYNCED-1.png");

            logger->info("Channel 2 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image2, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image2), directory + "/MERSI2-SYNCED-2.png");

            logger->info("Channel 3 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image3, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image3), directory + "/MERSI2-SYNCED-3.png");

            logger->info("Channel 4 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image4, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image4), directory + "/MERSI2-SYNCED-4.png");

            logger->info("Channel 5 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image5, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image5), directory + "/MERSI2-SYNCED-5.png");

            logger->info("Channel 6 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image6, 1, scanHeight_250, alpha, beta) : mersiCorrelator->image6), directory + "/MERSI2-SYNCED-6.png");

            logger->info("Channel 7 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image7, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image7), directory + "/MERSI2-SYNCED-7.png");

            logger->info("Channel 8 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image8, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image8), directory + "/MERSI2-SYNCED-8.png");

            logger->info("Channel 9 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image9, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image9), directory + "/MERSI2-SYNCED-9.png");

            logger->info("Channel 10 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image10, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image10), directory + "/MERSI2-SYNCED-10.png");

            logger->info("Channel 11 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image11, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image11), directory + "/MERSI2-SYNCED-11.png");

            logger->info("Channel 12 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image12, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image12), directory + "/MERSI2-SYNCED-12.png");

            logger->info("Channel 13 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image13, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image13), directory + "/MERSI2-SYNCED-13.png");

            logger->info("Channel 14 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image14, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image14), directory + "/MERSI2-SYNCED-14.png");

            logger->info("Channel 15 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image15, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image15), directory + "/MERSI2-SYNCED-15.png");

            logger->info("Channel 16 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image16, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image16), directory + "/MERSI2-SYNCED-16.png");

            logger->info("Channel 17 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image17, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image17), directory + "/MERSI2-SYNCED-17.png");

            logger->info("Channel 18 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image18, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image18), directory + "/MERSI2-SYNCED-18.png");

            logger->info("Channel 19 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image19, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image19), directory + "/MERSI2-SYNCED-19.png");

            logger->info("Channel 20 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image20, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image20), directory + "/MERSI2-SYNCED-20.png");

            logger->info("Channel 21 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image21, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image21), directory + "/MERSI2-SYNCED-21.png");

            logger->info("Channel 22 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image22, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image22), directory + "/MERSI2-SYNCED-22.png");

            logger->info("Channel 23 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image23, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image23), directory + "/MERSI2-SYNCED-23.png");

            logger->info("Channel 24 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image24, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image24), directory + "/MERSI2-SYNCED-24.png");

            logger->info("Channel 25 (synced for composites)...");
            WRITE_IMAGE((bowtie ? bowtie::correctGenericBowTie(mersiCorrelator->image25, 1, scanHeight_1000, alpha, beta) : mersiCorrelator->image25), directory + "/MERSI2-SYNCED-25.png");
        }

        void FengyunMERSI2DecoderModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun MERSI-2 Decoder", NULL, window ? NULL : NOWINDOW_FLAGS );

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

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

        std::shared_ptr<ProcessingModule> FengyunMERSI2DecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<FengyunMERSI2DecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mersi2
} // namespace fengyun