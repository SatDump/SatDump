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
#include "common/image/composite.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace viirs
    {
        JPSSVIIRSDecoderModule::JPSSVIIRSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                          npp_mode(parameters["npp_mode"].get<bool>())
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

            // Readers for all APIDs, in order!
            VIIRSReader reader_m[] = {VIIRSChannels[804], VIIRSChannels[803], VIIRSChannels[802], VIIRSChannels[800],
                                      VIIRSChannels[801], VIIRSChannels[805], VIIRSChannels[806], VIIRSChannels[809],
                                      VIIRSChannels[807], VIIRSChannels[808], VIIRSChannels[810], VIIRSChannels[812],
                                      VIIRSChannels[811], VIIRSChannels[816], VIIRSChannels[815], VIIRSChannels[814]};

            VIIRSReader reader_i[] = {VIIRSChannels[818], VIIRSChannels[819], VIIRSChannels[820],
                                      VIIRSChannels[813], VIIRSChannels[817]};

            VIIRSReader reader_dnb[] = {VIIRSChannels[821], VIIRSChannels[822],
                                        VIIRSChannels[823]};

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

                    std::vector<ccsds::CCSDSPacket> ccsds2 = ccsdsDemux.work(cadu);

                    ccsds_frames += ccsds2.size();

                    for (ccsds::CCSDSPacket &pkt : ccsds2)
                    {
                        // Moderate resolution channels
                        for (int i = 0; i < 16; i++)
                            reader_m[i].feed(pkt);

                        // Imaging channels
                        for (int i = 0; i < 5; i++)
                            reader_i[i].feed(pkt);

                        // DNB channels
                        for (int i = 0; i < 3; i++)
                            reader_dnb[i].feed(pkt);
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
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[4 - 1], reader_m[5 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[5 - 1] = correlatedChannels.second;
            }
            logger->info("Diff M3...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[4 - 1], reader_m[3 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[3 - 1] = correlatedChannels.second;
            }
            logger->info("Diff M2...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[3 - 1], reader_m[2 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[2 - 1] = correlatedChannels.second;
            }
            logger->info("Diff M1...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[2 - 1], reader_m[1 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[1 - 1] = correlatedChannels.second;
            }

            // Differential decoding for M8, M11
            logger->info("Diff M8...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[10 - 1], reader_m[8 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[8 - 1] = correlatedChannels.second;
            }
            logger->info("Diff M11...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[10 - 1], reader_m[11 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[11 - 1] = correlatedChannels.second;
            }

            // Differential decoding for M14
            logger->info("Diff M14...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[15 - 1], reader_m[14 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_m[14 - 1] = correlatedChannels.second;
            }

            // Differential decoding for I2, I3
            logger->info("Diff I2...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_i[1 - 1], reader_i[2 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_i[2 - 1] = correlatedChannels.second;
            }
            logger->info("Diff I3...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_i[2 - 1], reader_i[3 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 1);
                reader_i[3 - 1] = correlatedChannels.second;
            }

            // Differential decoding for I4 and I5
            logger->info("Diff I4...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[12 - 1], reader_i[4 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 2);
                reader_i[4 - 1] = correlatedChannels.second;
            }
            logger->info("Diff I5...");
            {
                std::pair<VIIRSReader, VIIRSReader> correlatedChannels = correlateChannels(reader_m[15 - 1], reader_i[5 - 1]);
                correlatedChannels.second.differentialDecode(correlatedChannels.first, 2);
                reader_i[5 - 1] = correlatedChannels.second;
            }

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            image::Image<uint16_t> images_m[16];
            for (int i = 0; i < 16; i++)
                images_m[i] = reader_m[i].getImage();

            image::Image<uint16_t> images_i[5];
            for (int i = 0; i < 5; i++)
                images_i[i] = reader_i[i].getImage();

            image::Image<uint16_t> images_dnb[] = {reader_dnb[0].getImage(), reader_dnb[1].getImage(),
                                                   reader_dnb[2].getImage()};

            // BowTie values
            const float alpha = 1.0 / 1.9;
            const float beta = 0.52333; //1.0 - alpha;

            // Defrag, to correct those lines on the edges...
            logger->info("Defragmenting...");
            for (int i = 0; i < 16; i++)
                images_m[i] = image::bowtie::correctGenericBowTie(images_m[i], 1, reader_m[i].channelSettings.zoneHeight, alpha, beta);

            for (int i = 0; i < 5; i++)
                images_i[i] = image::bowtie::correctGenericBowTie(images_i[i], 1, reader_i[i].channelSettings.zoneHeight, alpha, beta);

            // Correct mirrored channels
            for (int i = 0; i < 16; i++)
                images_m[i].mirror(true, false);

            for (int i = 0; i < 5; i++)
                images_i[i].mirror(true, false);

            for (int i = 0; i < 3; i++)
                images_dnb[i].mirror(true, false);

            logger->info("Making DNB night version...");
            image::Image<uint16_t> image_dnb_night = images_dnb[0];
            for (int i = 0; i < image_dnb_night.height() * image_dnb_night.width(); i++)
                image_dnb_night[i] = std::min(65535, 20 * image_dnb_night[i]);

            for (int i = 0; i < 16; i++)
            {
                if (images_m[i].height() > 0)
                {
                    logger->info("Channel M" + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(images_m[i], directory + "/VIIRS-M" + std::to_string(i + 1) + ".png");
                }
            }

            for (int i = 0; i < 5; i++)
            {
                if (images_i[i].height() > 0)
                {
                    logger->info("Channel I" + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(images_i[i], directory + "/VIIRS-I" + std::to_string(i + 1) + ".png");
                }
            }

            if (images_dnb[0].height() > 0)
            {
                logger->info("Channel DNB...");
                WRITE_IMAGE(images_dnb[0], directory + "/VIIRS-DNB.png");
                images_dnb[0].equalize();
                images_dnb[0].normalize();
                WRITE_IMAGE(images_dnb[0], directory + "/VIIRS-DNB-EQU.png");
            }

            if (image_dnb_night.height() > 0)
            {
                logger->info("Channel DNB - Night improved...");
                WRITE_IMAGE(image_dnb_night, directory + "/VIIRS-DNB-NIGHT.png");
            }

            if (images_dnb[1].height() > 0)
            {
                logger->info("Channel DNB-MGS...");
                WRITE_IMAGE(images_dnb[1], directory + "/VIIRS-DNB-MGS.png");
            }

            if (images_dnb[2].height() > 0)
            {
                logger->info("Channel DNB-LGS...");
                WRITE_IMAGE(images_dnb[2], directory + "/VIIRS-DNB-LGS.png");
            }

            // Generate composites
            for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &compokey : d_parameters["composites"].items())
            {
                nlohmann::json compositeDef = compokey.value();

                std::string expression = compositeDef["expression"].get<std::string>();
                //bool corrected = compositeDef.count("corrected") > 0 ? compositeDef["corrected"].get<bool>() : false;
                //bool projected = compositeDef.count("projected") > 0 ? compositeDef["projected"].get<bool>() : false;

                std::string name = "VIIRS-" + compokey.key();

                // Get required channels
                std::vector<int> requiredChannels = compositeDef["channels"].get<std::vector<int>>();

                // Check they are all present
                bool foundAll = true;
                std::vector<VIIRSReader> channels_reader;
                for (int channel : requiredChannels)
                {
                    VIIRSReader *reader = nullptr;
                    if (channel > 0 && channel <= 16)
                        reader = &reader_m[channel - 1];
                    else if (channel > 100 && channel <= 105)
                        reader = &reader_i[channel - 101];
                    else if (channel > 200 && channel <= 203)
                        reader = &reader_dnb[channel - 201];

                    if (reader->segments.size() > 0)
                    {
                        logger->debug("Channel " + std::to_string(channel) + " is present!");
                        channels_reader.push_back(*reader);
                    }
                    else
                    {
                        foundAll = false;
                        break;
                    }
                }

                if (!foundAll)
                {
                    logger->error("Some channels are missing, skipping composite.");
                    continue;
                }

                // Correlate
                std::vector<VIIRSReader> channels_reader_correlated = correlateChannels(channels_reader);
                std::vector<image::Image<uint16_t>> channels;
                std::vector<int> channel_numbers;
                for (int i = 0; i < (int)requiredChannels.size(); i++)
                {
                    channels.push_back(channels_reader_correlated[i].getImage());
                    channel_numbers.push_back(requiredChannels[i]);

                    channels[i] = image::bowtie::correctGenericBowTie(channels[i], 1, channels_reader_correlated[i].channelSettings.zoneHeight, alpha, beta);
                    channels[i].mirror(true, false);
                }

                logger->info(name + "...");
                image::Image<uint16_t>
                    compositeImage = image::generate_composite_from_equ<unsigned short>(channels,
                                                                                        channel_numbers,
                                                                                        expression,
                                                                                        compositeDef);

                WRITE_IMAGE(compositeImage, directory + "/" + name + ".png");
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

        std::shared_ptr<ProcessingModule> JPSSVIIRSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<JPSSVIIRSDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace viirs
} // namespace jpss