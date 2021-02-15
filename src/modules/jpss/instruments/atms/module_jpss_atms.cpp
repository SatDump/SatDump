#include "module_jpss_atms.h"
#include <fstream>
#include "atms_reader.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace atms
    {
        JPSSATMSDecoderModule::JPSSATMSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            npp_mode(std::stoi(parameters["npp_mode"]))
        {
            if (npp_mode)
            {
                cadu_size = 1024;
                mpdu_size = 884;
            }
            else
            {
                cadu_size = 1279;
                mpdu_size = 0;
            }
        }

        void JPSSATMSDecoderModule::process()
        {
            size_t filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ATMS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            logger->info("Demultiplexing and deframing...");

            // Read buffer
            libccsds::CADU cadu;

            // Counters
            uint64_t atms_cadu = 0, ccsds = 0, atms_ccsds = 0;

            libccsds::Demuxer ccsdsDemuxer = libccsds::Demuxer(mpdu_size, false);

            ATMSReader reader;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, cadu_size);

                // Parse this transport frame
                libccsds::VCDU vcdu = libccsds::parseVCDU(cadu);

                // Right channel? (VCID 1 is ATMS)
                if (vcdu.vcid == 1)
                {
                    atms_cadu++;

                    // Demux
                    std::vector<libccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 528)
                    for (libccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 528)
                        {
                            atms_ccsds++;
                            reader.work(pkt);
                        }
                    }
                }

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("VCID 1 (ATMS) Frames   : " + std::to_string(atms_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ATMS CCSDS Frames      : " + std::to_string(atms_ccsds));
            logger->info("ATMS Lines             : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image1 = reader.getImage(0);
            cimg_library::CImg<unsigned short> image2 = reader.getImage(1);
            cimg_library::CImg<unsigned short> image3 = reader.getImage(2);
            cimg_library::CImg<unsigned short> image4 = reader.getImage(3);
            cimg_library::CImg<unsigned short> image5 = reader.getImage(4);
            cimg_library::CImg<unsigned short> image6 = reader.getImage(5);
            cimg_library::CImg<unsigned short> image7 = reader.getImage(6);
            cimg_library::CImg<unsigned short> image8 = reader.getImage(7);
            cimg_library::CImg<unsigned short> image9 = reader.getImage(8);
            cimg_library::CImg<unsigned short> image10 = reader.getImage(9);
            cimg_library::CImg<unsigned short> image11 = reader.getImage(10);
            cimg_library::CImg<unsigned short> image12 = reader.getImage(11);
            cimg_library::CImg<unsigned short> image13 = reader.getImage(12);
            cimg_library::CImg<unsigned short> image14 = reader.getImage(13);
            cimg_library::CImg<unsigned short> image15 = reader.getImage(14);
            cimg_library::CImg<unsigned short> image16 = reader.getImage(15);
            cimg_library::CImg<unsigned short> image17 = reader.getImage(16);
            cimg_library::CImg<unsigned short> image18 = reader.getImage(17);
            cimg_library::CImg<unsigned short> image19 = reader.getImage(18);
            cimg_library::CImg<unsigned short> image20 = reader.getImage(19);
            cimg_library::CImg<unsigned short> image21 = reader.getImage(20);
            cimg_library::CImg<unsigned short> image22 = reader.getImage(21);

            image1.equalize(1000);
            image2.equalize(1000);
            image3.equalize(1000);
            image4.equalize(1000);
            image5.equalize(1000);
            image6.equalize(1000);
            image7.equalize(1000);
            image8.equalize(1000);
            image9.equalize(1000);
            image10.equalize(1000);
            image11.equalize(1000);
            image12.equalize(1000);
            image13.equalize(1000);
            image14.equalize(1000);
            image15.equalize(1000);
            image16.equalize(1000);
            image17.equalize(1000);
            image18.equalize(1000);
            image19.equalize(1000);
            image20.equalize(1000);
            image21.equalize(1000);
            image22.equalize(1000);

            logger->info("Channel 1...");
            image1.save_png(std::string(directory + "/ATMS-1.png").c_str());
            d_output_files.push_back(directory + "/ATMS-1.png");

            logger->info("Channel 2...");
            image2.save_png(std::string(directory + "/ATMS-2.png").c_str());
            d_output_files.push_back(directory + "/ATMS-2.png");

            logger->info("Channel 3...");
            image3.save_png(std::string(directory + "/ATMS-3.png").c_str());
            d_output_files.push_back(directory + "/ATMS-3.png");

            logger->info("Channel 4...");
            image4.save_png(std::string(directory + "/ATMS-4.png").c_str());
            d_output_files.push_back(directory + "/ATMS-4.png");

            logger->info("Channel 5...");
            image5.save_png(std::string(directory + "/ATMS-5.png").c_str());
            d_output_files.push_back(directory + "/ATMS-5.png");

            logger->info("Channel 6...");
            image6.save_png(std::string(directory + "/ATMS-6.png").c_str());
            d_output_files.push_back(directory + "/ATMS-6.png");

            logger->info("Channel 7...");
            image7.save_png(std::string(directory + "/ATMS-7.png").c_str());
            d_output_files.push_back(directory + "/ATMS-7.png");

            logger->info("Channel 8...");
            image8.save_png(std::string(directory + "/ATMS-8.png").c_str());
            d_output_files.push_back(directory + "/ATMS-8.png");

            logger->info("Channel 9...");
            image9.save_png(std::string(directory + "/ATMS-9.png").c_str());
            d_output_files.push_back(directory + "/ATMS-9.png");

            logger->info("Channel 10...");
            image10.save_png(std::string(directory + "/ATMS-10.png").c_str());
            d_output_files.push_back(directory + "/ATMS-10.png");

            logger->info("Channel 11...");
            image11.save_png(std::string(directory + "/ATMS-11.png").c_str());
            d_output_files.push_back(directory + "/ATMS-11.png");

            logger->info("Channel 12...");
            image12.save_png(std::string(directory + "/ATMS-12.png").c_str());
            d_output_files.push_back(directory + "/ATMS-12.png");

            logger->info("Channel 13...");
            image13.save_png(std::string(directory + "/ATMS-13.png").c_str());
            d_output_files.push_back(directory + "/ATMS-13.png");

            logger->info("Channel 14...");
            image14.save_png(std::string(directory + "/ATMS-14.png").c_str());
            d_output_files.push_back(directory + "/ATMS-14.png");

            logger->info("Channel 15...");
            image15.save_png(std::string(directory + "/ATMS-15.png").c_str());
            d_output_files.push_back(directory + "/ATMS-15.png");

            logger->info("Channel 16...");
            image16.save_png(std::string(directory + "/ATMS-16.png").c_str());
            d_output_files.push_back(directory + "/ATMS-16.png");

            logger->info("Channel 17...");
            image17.save_png(std::string(directory + "/ATMS-17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-17.png");

            logger->info("Channel 18...");
            image18.save_png(std::string(directory + "/ATMS-18.png").c_str());
            d_output_files.push_back(directory + "/ATMS-18.png");

            logger->info("Channel 19...");
            image19.save_png(std::string(directory + "/ATMS-19.png").c_str());
            d_output_files.push_back(directory + "/ATMS-19.png");

            logger->info("Channel 20...");
            image20.save_png(std::string(directory + "/ATMS-20.png").c_str());
            d_output_files.push_back(directory + "/ATMS-20.png");

            logger->info("Channel 21...");
            image21.save_png(std::string(directory + "/ATMS-21.png").c_str());
            d_output_files.push_back(directory + "/ATMS-21.png");

            logger->info("Channel 22...");
            image22.save_png(std::string(directory + "/ATMS-22.png").c_str());
            d_output_files.push_back(directory + "/ATMS-22.png");

            // Output a few nice composites as well
            logger->info("Global Composite...");
            cimg_library::CImg<unsigned short> imageAll(96 * 11, image1.height() * 2, 1, 1);
            {
                // Row 1
                imageAll.draw_image(96 * 0, 0, 0, 0, image1);
                imageAll.draw_image(96 * 1, 0, 0, 0, image2);
                imageAll.draw_image(96 * 2, 0, 0, 0, image3);
                imageAll.draw_image(96 * 3, 0, 0, 0, image4);
                imageAll.draw_image(96 * 4, 0, 0, 0, image5);
                imageAll.draw_image(96 * 5, 0, 0, 0, image6);
                imageAll.draw_image(96 * 6, 0, 0, 0, image7);
                imageAll.draw_image(96 * 7, 0, 0, 0, image8);
                imageAll.draw_image(96 * 8, 0, 0, 0, image9);
                imageAll.draw_image(96 * 9, 0, 0, 0, image10);
                imageAll.draw_image(96 * 10, 0, 0, 0, image11);

                // Row 2
                imageAll.draw_image(96 * 0, image1.height(), 0, 0, image12);
                imageAll.draw_image(96 * 1, image1.height(), 0, 0, image13);
                imageAll.draw_image(96 * 2, image1.height(), 0, 0, image14);
                imageAll.draw_image(96 * 3, image1.height(), 0, 0, image15);
                imageAll.draw_image(96 * 4, image1.height(), 0, 0, image16);
                imageAll.draw_image(96 * 5, image1.height(), 0, 0, image17);
                imageAll.draw_image(96 * 6, image1.height(), 0, 0, image18);
                imageAll.draw_image(96 * 7, image1.height(), 0, 0, image19);
                imageAll.draw_image(96 * 8, image1.height(), 0, 0, image20);
                imageAll.draw_image(96 * 9, image1.height(), 0, 0, image21);
                imageAll.draw_image(96 * 10, image1.height(), 0, 0, image22);
            }
            imageAll.save_png(std::string(directory + "/ATMS-ALL.png").c_str());
            d_output_files.push_back(directory + "/ATMS-ALL.png");

            logger->info("346 Composite...");
            cimg_library::CImg<unsigned short> image346(96, image1.height(), 1, 3);
            {
                image346.draw_image(0, 0, 0, 0, image3);
                image346.draw_image(0, 0, 0, 1, image4);
                image346.draw_image(0, 0, 0, 2, image6);
            }
            image346.equalize(1000);
            image346.save_png(std::string(directory + "/ATMS-RGB-346.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-346.png");

            logger->info("335 Composite...");
            cimg_library::CImg<unsigned short> image335(96, image1.height(), 1, 3);
            {
                image335.draw_image(0, 0, 0, 0, image3);
                image335.draw_image(0, 0, 0, 1, image3);
                image335.draw_image(0, 0, 0, 2, image5);
            }
            image335.equalize(1000);
            image335.save_png(std::string(directory + "/ATMS-RGB-335.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-335.png");

            logger->info("4.3.17 Composite...");
            cimg_library::CImg<unsigned short> image4317(96, image1.height(), 1, 3);
            {
                image4317.draw_image(0, 0, 0, 0, image4);
                image4317.draw_image(0, 0, 0, 1, image3);
                image4317.draw_image(0, 0, 0, 2, image17);
            }
            image4317.equalize(1000);
            image4317.save_png(std::string(directory + "/ATMS-RGB-4.3.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-4.3.17.png");

            logger->info("445 Composite...");
            cimg_library::CImg<unsigned short> image445(96, image1.height(), 1, 3);
            {
                image445.draw_image(0, 0, 0, 0, image4);
                image445.draw_image(0, 0, 0, 1, image4);
                image445.draw_image(0, 0, 0, 2, image5);
            }
            image445.equalize(1000);
            image445.save_png(std::string(directory + "/ATMS-RGB-445.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-445.png");

            logger->info("4.4.17 Composite...");
            cimg_library::CImg<unsigned short> image4417(96, image1.height(), 1, 3);
            {
                image4417.draw_image(0, 0, 0, 0, image4);
                image4417.draw_image(0, 0, 0, 1, image4);
                image4417.draw_image(0, 0, 0, 2, image17);
            }
            image4417.equalize(1000);
            image4417.save_png(std::string(directory + "/ATMS-RGB-4.4.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-4.4.17.png");

            logger->info("4.16.17 Composite...");
            cimg_library::CImg<unsigned short> image41617(96, image1.height(), 1, 3);
            {
                image41617.draw_image(0, 0, 0, 0, image4);
                image41617.draw_image(0, 0, 0, 1, image16);
                image41617.draw_image(0, 0, 0, 2, image17);
            }
            image41617.equalize(1000);
            image41617.save_png(std::string(directory + "/ATMS-RGB-4.16.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-4.16.17.png");

            logger->info("3.4.17 Composite...");
            cimg_library::CImg<unsigned short> image3417(96, image1.height(), 1, 3);
            {
                image3417.draw_image(0, 0, 0, 0, image3);
                image3417.draw_image(0, 0, 0, 1, image4);
                image3417.draw_image(0, 0, 0, 2, image17);
            }
            image3417.equalize(1000);
            image3417.save_png(std::string(directory + "/ATMS-RGB-3.4.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-3.4.17.png");

            logger->info("5.5.17 Composite...");
            cimg_library::CImg<unsigned short> image5517(96, image1.height(), 1, 3);
            {
                image5517.draw_image(0, 0, 0, 0, image5);
                image5517.draw_image(0, 0, 0, 1, image5);
                image5517.draw_image(0, 0, 0, 2, image17);
            }
            image5517.equalize(1000);
            image5517.save_png(std::string(directory + "/ATMS-RGB-5.5.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-5.5.17.png");

            logger->info("6.4.17 Composite...");
            cimg_library::CImg<unsigned short> image6417(96, image1.height(), 1, 3);
            {
                image6417.draw_image(0, 0, 0, 0, image6);
                image6417.draw_image(0, 0, 0, 1, image4);
                image6417.draw_image(0, 0, 0, 2, image17);
            }
            image6417.equalize(1000);
            image6417.save_png(std::string(directory + "/ATMS-RGB-6.4.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-6.4.17.png");

            logger->info("16.4.17 Composite...");
            cimg_library::CImg<unsigned short> image16417(96, image1.height(), 1, 3);
            {
                image16417.draw_image(0, 0, 0, 0, image16);
                image16417.draw_image(0, 0, 0, 1, image4);
                image16417.draw_image(0, 0, 0, 2, image17);
            }
            image16417.equalize(1000);
            image16417.save_png(std::string(directory + "/ATMS-RGB-16.4.17.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-16.4.17.png");

            logger->info("17.16.6 Composite...");
            cimg_library::CImg<unsigned short> image17166(96, image1.height(), 1, 3);
            {
                image17166.draw_image(0, 0, 0, 0, image17);
                image17166.draw_image(0, 0, 0, 1, image16);
                image17166.draw_image(0, 0, 0, 2, image6);
            }
            image17166.equalize(1000);
            image17166.save_png(std::string(directory + "/ATMS-RGB-17.16.6.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-17.16.6.png");

            // Output a few nice composites as well
            logger->info("Global Composite...");
            cimg_library::CImg<unsigned short> imageRgbAll(96 * 6, image1.height() * 2, 1, 3);
            {
                // Row 1
                imageRgbAll.draw_image(96 * 0, 0, 0, 0, image346);
                imageRgbAll.draw_image(96 * 1, 0, 0, 0, image335);
                imageRgbAll.draw_image(96 * 2, 0, 0, 0, image4317);
                imageRgbAll.draw_image(96 * 3, 0, 0, 0, image445);
                imageRgbAll.draw_image(96 * 4, 0, 0, 0, image4417);
                imageRgbAll.draw_image(96 * 5, 0, 0, 0, image41617);

                // Row 2
                imageRgbAll.draw_image(96 * 0, image1.height(), 0, 0, image3417);
                imageRgbAll.draw_image(96 * 1, image1.height(), 0, 0, image5517);
                imageRgbAll.draw_image(96 * 2, image1.height(), 0, 0, image6417);
                imageRgbAll.draw_image(96 * 3, image1.height(), 0, 0, image16417);
                imageRgbAll.draw_image(96 * 4, image1.height(), 0, 0, image17166);
            }
            imageRgbAll.save_png(std::string(directory + "/ATMS-RGB-ALL.png").c_str());
            d_output_files.push_back(directory + "/ATMS-RGB-ALL.png");
        }

        std::string JPSSATMSDecoderModule::getID()
        {
            return "jpss_atms";
        }

        std::vector<std::string> JPSSATMSDecoderModule::getParameters()
        {
            return {"npp_mode"};
        }

        std::shared_ptr<ProcessingModule> JPSSATMSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<JPSSATMSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace atms
} // namespace jpss