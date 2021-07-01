#include "module_metop_ascat.h"
#include <fstream>
#include "ascat_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace ascat
    {
        MetOpASCATDecoderModule::MetOpASCATDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpASCATDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ASCAT";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            ASCATReader ascatreader;
            uint64_t ascat_cadu = 0, ccsds = 0, ascat_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 12 is ASCAT)
                if (vcdu.vcid == 15)
                {
                    ascat_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        ascatreader.work(pkt);
                        ascat_ccsds++;
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

            logger->info("VCID 15 (ASCAT) Frames : " + std::to_string(ascat_cadu));
            logger->info("CCSDS Frames           : " + std::to_string(ccsds));
            logger->info("ASCAT Frames           : " + std::to_string(ascat_ccsds));
            logger->info("ASCAT Channel 1 lines  : " + std::to_string(ascatreader.lines[0]));
            logger->info("ASCAT Channel 2 lines  : " + std::to_string(ascatreader.lines[1]));
            logger->info("ASCAT Channel 3 lines  : " + std::to_string(ascatreader.lines[2]));
            logger->info("ASCAT Channel 4 lines  : " + std::to_string(ascatreader.lines[3]));
            logger->info("ASCAT Channel 5 lines  : " + std::to_string(ascatreader.lines[4]));
            logger->info("ASCAT Channel 6 lines  : " + std::to_string(ascatreader.lines[5]));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            for (int i = 0; i < 6; i++)
            {
                logger->info("Channel " + std::to_string(i + 1) + "...");
                cimg_library::CImg<unsigned short> img = ascatreader.getChannel(i);
                WRITE_IMAGE(img, directory + "/ASCAT-" + std::to_string(i + 1) + ".png");
                img.equalize(1000);
                WRITE_IMAGE(img, directory + "/ASCAT-" + std::to_string(i + 1) + "-EQU.png");
            }

            cimg_library::CImg<unsigned short> image1 = ascatreader.getChannel(0);
            cimg_library::CImg<unsigned short> image2 = ascatreader.getChannel(1);
            cimg_library::CImg<unsigned short> image3 = ascatreader.getChannel(2);
            cimg_library::CImg<unsigned short> image4 = ascatreader.getChannel(3);
            cimg_library::CImg<unsigned short> image5 = ascatreader.getChannel(4);
            cimg_library::CImg<unsigned short> image6 = ascatreader.getChannel(5);

            // Output a few nice composites as well
            logger->info("Global Composite...");
            cimg_library::CImg<unsigned short> imageAll(256 * 2, ascatreader.getChannel(0).height() * 3, 1, 1);
            {
                int height = ascatreader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(256 * 0, 0, 0, 0, image1);
                imageAll.draw_image(256 * 1, 0, 0, 0, image4);

                // Row 2
                imageAll.draw_image(256 * 0, height, 0, 0, image2);
                imageAll.draw_image(256 * 1, height, 0, 0, image5);

                // Row 3
                imageAll.draw_image(256 * 0, height * 2, 0, 0, image3);
                imageAll.draw_image(256 * 1, height * 2, 0, 0, image6);
            }
            WRITE_IMAGE(imageAll, directory + "/ASCAT-ALL.png");

            image1.equalize(1000);
            image2.equalize(1000);
            image3.equalize(1000);
            image4.equalize(1000);
            image5.equalize(1000);
            image6.equalize(1000);

            logger->info("Global Equalized Composite...");
            cimg_library::CImg<unsigned short> imageAllEqu(256 * 2, ascatreader.getChannel(0).height() * 3, 1, 1);
            {
                int height = ascatreader.getChannel(0).height();

                // Row 1
                imageAll.draw_image(256 * 0, 0, 0, 0, image1);
                imageAll.draw_image(256 * 1, 0, 0, 0, image4);

                // Row 2
                imageAll.draw_image(256 * 0, height, 0, 0, image2);
                imageAll.draw_image(256 * 1, height, 0, 0, image5);

                // Row 3
                imageAll.draw_image(256 * 0, height * 2, 0, 0, image3);
                imageAll.draw_image(256 * 1, height * 2, 0, 0, image6);
            }
            WRITE_IMAGE(imageAll, directory + "/ASCAT-EQU-ALL.png");
        }

        void MetOpASCATDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp ASCAT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpASCATDecoderModule::getID()
        {
            return "metop_ascat";
        }

        std::vector<std::string> MetOpASCATDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpASCATDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<MetOpASCATDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mhs
} // namespace metop