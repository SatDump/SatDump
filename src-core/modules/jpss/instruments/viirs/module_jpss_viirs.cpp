#include "module_jpss_viirs.h"
#include <fstream>
#include "channel_reader.h"
#include "channel_correlator.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/image/image.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace viirs
    {
        JPSSVIIRSDecoderModule::JPSSVIIRSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
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

        void JPSSVIIRSDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIIRS";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            logger->info("Demultiplexing and deframing...");

            // Read buffer
            uint8_t cadu[1024];

            // Counters
            uint64_t vcidFrames = 0, virr_cadu = 0, ccsds_frames = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemux = ccsds::ccsds_1_0_1024::Demuxer(mpdu_size, false);

            // Readers for all APIDs
            VIIRSReader reader_m4(VIIRSChannels[800]),
                reader_m5(VIIRSChannels[801]),
                reader_m3(VIIRSChannels[802]),
                reader_m2(VIIRSChannels[803]),
                reader_m1(VIIRSChannels[804]),
                reader_m6(VIIRSChannels[805]),
                reader_m7(VIIRSChannels[806]),
                reader_m9(VIIRSChannels[807]),
                reader_m10(VIIRSChannels[808]),
                reader_m8(VIIRSChannels[809]),
                reader_m11(VIIRSChannels[810]),
                reader_m13(VIIRSChannels[811]),
                reader_m12(VIIRSChannels[812]),
                reader_m16(VIIRSChannels[814]),
                reader_m15(VIIRSChannels[815]),
                reader_m14(VIIRSChannels[816]);

            VIIRSReader reader_i1(VIIRSChannels[818]),
                reader_i2(VIIRSChannels[819]),
                reader_i3(VIIRSChannels[820]),
                reader_i4(VIIRSChannels[813]),
                reader_i5(VIIRSChannels[817]);

            VIIRSReader reader_dnb(VIIRSChannels[821]),
                reader_dnb_mgs(VIIRSChannels[822]),
                reader_dnb_lgs(VIIRSChannels[823]);

            while (!data_in.eof())
            {
                data_in.read((char *)&cadu, 1024);
                vcidFrames++;

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 30 is MODIS)
                if (vcdu.vcid == 16)
                {
                    virr_cadu++;

                    std::vector<ccsds::ccsds_1_0_1024::CCSDSPacket> ccsds2 = ccsdsDemux.work(cadu);

                    ccsds_frames += ccsds2.size();

                    for (ccsds::ccsds_1_0_1024::CCSDSPacket &pkt : ccsds2)
                    {
                        // Moderate resolution channels
                        reader_m4.feed(pkt);
                        reader_m5.feed(pkt);
                        reader_m3.feed(pkt);
                        reader_m2.feed(pkt);
                        reader_m1.feed(pkt);
                        reader_m6.feed(pkt);
                        reader_m7.feed(pkt);
                        reader_m9.feed(pkt);
                        reader_m10.feed(pkt);
                        reader_m8.feed(pkt);
                        reader_m11.feed(pkt);
                        reader_m13.feed(pkt);
                        reader_m12.feed(pkt);
                        reader_m16.feed(pkt);
                        reader_m15.feed(pkt);
                        reader_m14.feed(pkt);

                        // Imaging channels
                        reader_i1.feed(pkt);
                        reader_i2.feed(pkt);
                        reader_i3.feed(pkt);
                        reader_i4.feed(pkt);
                        reader_i5.feed(pkt);

                        // DNB channels
                        reader_dnb.feed(pkt);
                        reader_dnb_mgs.feed(pkt);
                        reader_dnb_lgs.feed(pkt);
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

            logger->info("VCID 16 Frames         : " + std::to_string(vcidFrames));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds_frames));

            // Differential decoding for M5, M3, M2, M1
            logger->info("Diff M5...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m4, reader_m5);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m5 = correlatedChannels.second;
            }
            logger->info("Diff M3...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m4, reader_m3);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m3 = correlatedChannels.second;
            }
            logger->info("Diff M2...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m3, reader_m2);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m2 = correlatedChannels.second;
            }
            logger->info("Diff M1...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m2, reader_m1);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m1 = correlatedChannels.second;
            }

            // Differential decoding for M8, M11
            logger->info("Diff M8...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m10, reader_m8);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m8 = correlatedChannels.second;
            }
            logger->info("Diff M11...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m10, reader_m11);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m11 = correlatedChannels.second;
            }

            // Differential decoding for M14
            logger->info("Diff M14...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m15, reader_m14);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m14 = correlatedChannels.second;
            }

            // Differential decoding for I2, I3
            logger->info("Diff I2...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_i1, reader_i2);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_i2 = correlatedChannels.second;
            }
            logger->info("Diff I3...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_i2, reader_i3);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_i3 = correlatedChannels.second;
            }

            // Differential decoding for I4 and I5
            logger->info("Diff I4...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m12, reader_i4);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 2);
                reader_i4 = correlatedChannels.second;
            }
            logger->info("Diff I5...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m15, reader_i5);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 2);
                reader_i5 = correlatedChannels.second;
            }

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            cimg_library::CImg<unsigned short> image_m1 = reader_m1.getImage();
            cimg_library::CImg<unsigned short> image_m2 = reader_m2.getImage();
            cimg_library::CImg<unsigned short> image_m3 = reader_m3.getImage();
            cimg_library::CImg<unsigned short> image_m4 = reader_m4.getImage();
            cimg_library::CImg<unsigned short> image_m5 = reader_m5.getImage();
            cimg_library::CImg<unsigned short> image_m6 = reader_m6.getImage();
            cimg_library::CImg<unsigned short> image_m7 = reader_m7.getImage();
            cimg_library::CImg<unsigned short> image_m8 = reader_m8.getImage();
            cimg_library::CImg<unsigned short> image_m9 = reader_m9.getImage();
            cimg_library::CImg<unsigned short> image_m10 = reader_m10.getImage();
            cimg_library::CImg<unsigned short> image_m11 = reader_m11.getImage();
            cimg_library::CImg<unsigned short> image_m12 = reader_m12.getImage();
            cimg_library::CImg<unsigned short> image_m13 = reader_m13.getImage();
            cimg_library::CImg<unsigned short> image_m14 = reader_m14.getImage();
            cimg_library::CImg<unsigned short> image_m15 = reader_m15.getImage();
            cimg_library::CImg<unsigned short> image_m16 = reader_m16.getImage();

            cimg_library::CImg<unsigned short> image_i1 = reader_i1.getImage();
            cimg_library::CImg<unsigned short> image_i2 = reader_i2.getImage();
            cimg_library::CImg<unsigned short> image_i3 = reader_i3.getImage();
            cimg_library::CImg<unsigned short> image_i4 = reader_i4.getImage();
            cimg_library::CImg<unsigned short> image_i5 = reader_i5.getImage();

            cimg_library::CImg<unsigned short> image_dnb = reader_dnb.getImage();
            cimg_library::CImg<unsigned short> image_dnb_mgs = reader_dnb_mgs.getImage();
            cimg_library::CImg<unsigned short> image_dnb_lgs = reader_dnb_lgs.getImage();

            // BowTie values
            const float alpha = 1.0 / 1.9;
            const float beta = 0.52333; //1.0 - alpha;

            // Defrag, to correct those lines on the edges...
            logger->info("Defragmenting...");
            image_m1 = image::bowtie::correctGenericBowTie(image_m1, 1, reader_m1.channelSettings.zoneHeight, alpha, beta);
            image_m2 = image::bowtie::correctGenericBowTie(image_m2, 1, reader_m2.channelSettings.zoneHeight, alpha, beta);
            image_m3 = image::bowtie::correctGenericBowTie(image_m3, 1, reader_m3.channelSettings.zoneHeight, alpha, beta);
            image_m4 = image::bowtie::correctGenericBowTie(image_m4, 1, reader_m4.channelSettings.zoneHeight, alpha, beta);
            image_m5 = image::bowtie::correctGenericBowTie(image_m5, 1, reader_m5.channelSettings.zoneHeight, alpha, beta);
            image_m6 = image::bowtie::correctGenericBowTie(image_m6, 1, reader_m6.channelSettings.zoneHeight, alpha, beta);
            image_m7 = image::bowtie::correctGenericBowTie(image_m7, 1, reader_m7.channelSettings.zoneHeight, alpha, beta);
            image_m8 = image::bowtie::correctGenericBowTie(image_m8, 1, reader_m8.channelSettings.zoneHeight, alpha, beta);
            image_m9 = image::bowtie::correctGenericBowTie(image_m9, 1, reader_m9.channelSettings.zoneHeight, alpha, beta);
            image_m10 = image::bowtie::correctGenericBowTie(image_m10, 1, reader_m10.channelSettings.zoneHeight, alpha, beta);
            image_m11 = image::bowtie::correctGenericBowTie(image_m11, 1, reader_m11.channelSettings.zoneHeight, alpha, beta);
            image_m12 = image::bowtie::correctGenericBowTie(image_m12, 1, reader_m12.channelSettings.zoneHeight, alpha, beta);
            image_m13 = image::bowtie::correctGenericBowTie(image_m13, 1, reader_m13.channelSettings.zoneHeight, alpha, beta);
            image_m14 = image::bowtie::correctGenericBowTie(image_m14, 1, reader_m14.channelSettings.zoneHeight, alpha, beta);
            image_m15 = image::bowtie::correctGenericBowTie(image_m15, 1, reader_m15.channelSettings.zoneHeight, alpha, beta);
            image_m16 = image::bowtie::correctGenericBowTie(image_m16, 1, reader_m16.channelSettings.zoneHeight, alpha, beta);

            image_i1 = image::bowtie::correctGenericBowTie(image_i1, 1, reader_i1.channelSettings.zoneHeight, alpha, beta);
            image_i2 = image::bowtie::correctGenericBowTie(image_i2, 1, reader_i2.channelSettings.zoneHeight, alpha, beta);
            image_i3 = image::bowtie::correctGenericBowTie(image_i3, 1, reader_i3.channelSettings.zoneHeight, alpha, beta);
            image_i4 = image::bowtie::correctGenericBowTie(image_i4, 1, reader_i4.channelSettings.zoneHeight, alpha, beta);
            image_i5 = image::bowtie::correctGenericBowTie(image_i5, 1, reader_i5.channelSettings.zoneHeight, alpha, beta);

            // DNB has no Bowtie
            //image_dnb = image::bowtie::correctGenericBowTie(image_dnb, 1, reader_dnb.channelSettings.zoneHeight, alpha, beta);
            //image_dnb_lgs = image::bowtie::correctGenericBowTie(image_dnb_lgs, 1, reader_dnb_lgs.channelSettings.zoneHeight, alpha, beta);
            //image_dnb_mgs = image::bowtie::correctGenericBowTie(image_dnb_mgs, 1, reader_dnb_mgs.channelSettings.zoneHeight, alpha, beta);

            // Correct mirrored channels
            image_i1.mirror('x');
            image_i2.mirror('x');
            image_i3.mirror('x');
            image_i4.mirror('x');
            image_i5.mirror('x');
            image_m1.mirror('x');
            image_m2.mirror('x');
            image_m3.mirror('x');
            image_m4.mirror('x');
            image_m5.mirror('x');
            image_m6.mirror('x');
            image_m7.mirror('x');
            image_m8.mirror('x');
            image_m9.mirror('x');
            image_m10.mirror('x');
            image_m11.mirror('x');
            image_m12.mirror('x');
            image_m13.mirror('x');
            image_m14.mirror('x');
            image_m15.mirror('x');
            image_m16.mirror('x');
            image_dnb.mirror('x');
            image_dnb_lgs.mirror('x');
            image_dnb_mgs.mirror('x');

            logger->info("Making DNB night version...");
            cimg_library::CImg<unsigned short> image_dnb_night = image_dnb;
            for (int i = 0; i < image_dnb_night.height() * image_dnb_night.width(); i++)
                image_dnb_night.data()[i] *= 15;

            // Takes a while so we say how we're doing
            if (image_m1.height() > 0)
            {
                logger->info("Channel M1...");
                WRITE_IMAGE(image_m1, directory + "/VIIRS-M1.png");
            }

            if (image_m2.height() > 0)
            {
                logger->info("Channel M2...");
                WRITE_IMAGE(image_m2, directory + "/VIIRS-M2.png");
            }

            if (image_m3.height() > 0)
            {
                logger->info("Channel M3...");
                WRITE_IMAGE(image_m3, directory + "/VIIRS-M3.png");
            }

            if (image_m4.height() > 0)
            {
                logger->info("Channel M4...");
                WRITE_IMAGE(image_m4, directory + "/VIIRS-M4.png");
            }

            if (image_m5.height() > 0)
            {
                logger->info("Channel M5...");
                WRITE_IMAGE(image_m5, directory + "/VIIRS-M5.png");
            }

            if (image_m6.height() > 0)
            {
                logger->info("Channel M6...");
                WRITE_IMAGE(image_m6, directory + "/VIIRS-M6.png");
            }

            if (image_m7.height() > 0)
            {
                logger->info("Channel M7...");
                WRITE_IMAGE(image_m7, directory + "/VIIRS-M7.png");
            }

            if (image_m8.height() > 0)
            {
                logger->info("Channel M8...");
                WRITE_IMAGE(image_m8, directory + "/VIIRS-M8.png");
            }

            if (image_m9.height() > 0)
            {
                logger->info("Channel M9...");
                WRITE_IMAGE(image_m9, directory + "/VIIRS-M9.png");
            }

            if (image_m10.height() > 0)
            {
                logger->info("Channel M10...");
                WRITE_IMAGE(image_m10, directory + "/VIIRS-M10.png");
            }

            if (image_m11.height() > 0)
            {
                logger->info("Channel M11...");
                WRITE_IMAGE(image_m11, directory + "/VIIRS-M11.png");
            }

            if (image_m12.height() > 0)
            {
                logger->info("Channel M12...");
                WRITE_IMAGE(image_m12, directory + "/VIIRS-M12.png");
            }

            if (image_m13.height() > 0)
            {
                logger->info("Channel M13...");
                WRITE_IMAGE(image_m13, directory + "/VIIRS-M13.png");
            }

            if (image_m14.height() > 0)
            {
                logger->info("Channel M14...");
                WRITE_IMAGE(image_m14, directory + "/VIIRS-M14.png");
            }

            if (image_m15.height() > 0)
            {
                logger->info("Channel M15...");
                WRITE_IMAGE(image_m15, directory + "/VIIRS-M15.png");
            }

            if (image_m16.height() > 0)
            {
                logger->info("Channel M16...");
                WRITE_IMAGE(image_m16, directory + "/VIIRS-M16.png");
            }

            if (image_i1.height() > 0)
            {
                logger->info("Channel I1...");
                WRITE_IMAGE(image_i1, directory + "/VIIRS-I1.png");
            }

            if (image_i2.height() > 0)
            {
                logger->info("Channel I2...");
                WRITE_IMAGE(image_i2, directory + "/VIIRS-I2.png");
            }

            if (image_i3.height() > 0)
            {
                logger->info("Channel I3...");
                WRITE_IMAGE(image_i3, directory + "/VIIRS-I3.png");
            }

            if (image_i4.height() > 0)
            {
                logger->info("Channel I4...");
                WRITE_IMAGE(image_i4, directory + "/VIIRS-I4.png");
            }

            if (image_i5.height() > 0)
            {
                logger->info("Channel I5...");
                WRITE_IMAGE(image_i5, directory + "/VIIRS-I5.png");
            }

            if (image_dnb.height() > 0)
            {
                logger->info("Channel DNB...");
                WRITE_IMAGE(image_dnb, directory + "/VIIRS-DNB.png");
                image_dnb.equalize(1000);
                image_dnb.normalize(0, 65535);
                WRITE_IMAGE(image_dnb, directory + "/VIIRS-DNB-EQU.png");
            }

            if (image_dnb_night.height() > 0)
            {
                logger->info("Channel DNB - Night improved...");
                WRITE_IMAGE(image_dnb_night, directory + "/VIIRS-DNB-NIGHT.png");
            }

            if (image_dnb_mgs.height() > 0)
            {
                logger->info("Channel DNB-MGS...");
                WRITE_IMAGE(image_dnb_mgs, directory + "/VIIRS-DNB-MGS.png");
            }

            if (image_dnb_lgs.height() > 0)
            {
                logger->info("Channel DNB-LGS...");
                WRITE_IMAGE(image_dnb_lgs, directory + "/VIIRS-DNB-LGS.png");
            }

            // Output a few nice composites as well
            if (image_i1.height() > 0 && image_i2.height() > 0)
            {
                logger->info("I221 Composite...");
                cimg_library::CImg<unsigned short> image221;
                {
                    std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_i1, reader_i2);
                    cimg_library::CImg<unsigned short> tempImage2 = correlatedChannels.second.getImage(), tempImage1 = correlatedChannels.first.getImage();
                    image221 = cimg_library::CImg<unsigned short>(6400, tempImage1.height(), 1, 3);
                    tempImage2 = image::bowtie::correctGenericBowTie(tempImage2, 1, std::get<0>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage1 = image::bowtie::correctGenericBowTie(tempImage1, 1, std::get<1>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    image221.draw_image(0, 0, 0, 0, tempImage2);
                    image221.draw_image(0, 0, 0, 1, tempImage2);
                    image221.draw_image(0, 0, 0, 2, tempImage1);
                    image221.mirror('x');
                    image::white_balance(image221);
                }
                WRITE_IMAGE(image221, directory + "/VIIRS-RGB-I221.png");
                image221.equalize(1000);
                WRITE_IMAGE(image221, directory + "/VIIRS-RGB-I221-EQU.png");
            }

            if (image_i1.height() > 0 && image_i2.height() > 0 && image_i3.height() > 0)
            {
                logger->info("I312 Composite...");
                cimg_library::CImg<unsigned short> image321;
                {
                    std::tuple<VIIRSReader, VIIRSReader, VIIRSReader> correlatedChannels = correlateThreeChannels(reader_i1, reader_i2, reader_i3);
                    cimg_library::CImg<unsigned short> tempImage2 = std::get<0>(correlatedChannels).getImage(), tempImage1 = std::get<1>(correlatedChannels).getImage(), tempImage3 = std::get<2>(correlatedChannels).getImage();
                    image321 = cimg_library::CImg<unsigned short>(6400, tempImage1.height(), 1, 3);
                    tempImage2 = image::bowtie::correctGenericBowTie(tempImage2, 1, std::get<0>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage1 = image::bowtie::correctGenericBowTie(tempImage1, 1, std::get<1>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage3 = image::bowtie::correctGenericBowTie(tempImage3, 1, std::get<2>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    image321.draw_image(0, 0, 0, 0, tempImage3);
                    image321.draw_image(0, 0, 0, 1, tempImage1);
                    image321.draw_image(0, 0, 0, 2, tempImage2);
                    image321.mirror('x');
                    image::white_balance(image321);
                }
                WRITE_IMAGE(image321, directory + "/VIIRS-RGB-I312.png");
                image321.equalize(1000);
                WRITE_IMAGE(image321, directory + "/VIIRS-RGB-I321-EQU.png");
            }

            if (image_m4.height() > 0 && image_m5.height() > 0 && image_m3.height() > 0)
            {
                logger->info("M453 Composite...");
                cimg_library::CImg<unsigned short> image453;
                {
                    std::tuple<VIIRSReader, VIIRSReader, VIIRSReader> correlatedChannels = correlateThreeChannels(reader_m4, reader_m5, reader_m3);
                    cimg_library::CImg<unsigned short> tempImage4 = std::get<0>(correlatedChannels).getImage(), tempImage5 = std::get<1>(correlatedChannels).getImage(), tempImage3 = std::get<2>(correlatedChannels).getImage();
                    image453 = cimg_library::CImg<unsigned short>(3200, tempImage5.height(), 1, 3);
                    tempImage5 = image::bowtie::correctGenericBowTie(tempImage5, 1, std::get<1>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage4 = image::bowtie::correctGenericBowTie(tempImage4, 1, std::get<0>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage3 = image::bowtie::correctGenericBowTie(tempImage3, 1, std::get<2>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    image453.draw_image(0, 0, 0, 0, tempImage4);
                    image453.draw_image(0, 0, 0, 1, tempImage5);
                    image453.draw_image(0, 0, 0, 2, tempImage3);
                    image453.mirror('x');
                    image::white_balance(image453);
                }
                WRITE_IMAGE(image453, directory + "/VIIRS-RGB-M453.png");
                image453.equalize(1000);
                WRITE_IMAGE(image453, directory + "/VIIRS-RGB-M453-EQU.png");
            }

            if (image_m4.height() > 0 && image_m5.height() > 0 && image_m3.height() > 0)
            {
                logger->info("M543 Composite...");
                cimg_library::CImg<unsigned short> image543;
                {
                    std::tuple<VIIRSReader, VIIRSReader, VIIRSReader> correlatedChannels = correlateThreeChannels(reader_m4, reader_m5, reader_m3);
                    cimg_library::CImg<unsigned short> tempImage4 = std::get<0>(correlatedChannels).getImage(), tempImage5 = std::get<1>(correlatedChannels).getImage(), tempImage3 = std::get<2>(correlatedChannels).getImage();
                    image543 = cimg_library::CImg<unsigned short>(3200, tempImage5.height(), 1, 3);
                    tempImage5 = image::bowtie::correctGenericBowTie(tempImage5, 1, std::get<1>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage4 = image::bowtie::correctGenericBowTie(tempImage4, 1, std::get<0>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImage3 = image::bowtie::correctGenericBowTie(tempImage3, 1, std::get<2>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    image543.draw_image(0, 0, 0, 0, tempImage5);
                    image543.draw_image(0, 0, 0, 1, tempImage4);
                    image543.draw_image(0, 0, 0, 2, tempImage3);
                    image543.mirror('x');
                    image::white_balance(image543);
                }
                WRITE_IMAGE(image543, directory + "/VIIRS-RGB-M543.png");
                image543.equalize(1000);
                WRITE_IMAGE(image543, directory + "/VIIRS-RGB-M543-EQU.png");
            }

            if (image_dnb.height() > 0 && image_m16.height() > 0)
            {
                logger->info("M16-DNB Composite...");
                cimg_library::CImg<unsigned short> imagem16dnb;
                {
                    std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m16, reader_dnb);
                    cimg_library::CImg<unsigned short> tempImage5 = std::get<0>(correlatedChannels).getImage(), tempImageDNB = std::get<1>(correlatedChannels).getImage();

                    tempImage5 = image::bowtie::correctGenericBowTie(tempImage5, 1, std::get<0>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);
                    tempImageDNB = image::bowtie::correctGenericBowTie(tempImageDNB, 1, std::get<1>(correlatedChannels).channelSettings.zoneHeight, alpha, beta);

                    tempImageDNB.resize(tempImage5.width(), tempImage5.height());

                    for (int i = 0; i < tempImageDNB.height() * tempImageDNB.width(); i++)
                        tempImageDNB.data()[i] *= 15;

                    imagem16dnb = cimg_library::CImg<unsigned short>(3200 - 50, tempImage5.height(), 1, 1);
                    tempImage5.equalize(1000);

                    imagem16dnb.draw_image(0, 0, 0, 0, tempImage5, 1);
                    imagem16dnb.draw_image(-50, 0, 0, 0, tempImageDNB, 0.8);

                    imagem16dnb.mirror('x');
                }
                WRITE_IMAGE(imagem16dnb, directory + "/VIIRS-M16-DNB.png");
            }
        }

        void JPSSVIIRSDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("JPSS VIIRS Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string JPSSVIIRSDecoderModule::getID()
        {
            return "jpss_viirs";
        }

        std::vector<std::string> JPSSVIIRSDecoderModule::getParameters()
        {
            return {"npp_mode"};
        }

        std::shared_ptr<ProcessingModule> JPSSVIIRSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<JPSSVIIRSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace viirs
} // namespace jpss