#include "module_metop_gome.h"
#include <fstream>
#include "gome_reader.h"
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
    namespace gome
    {
        MetOpGOMEDecoderModule::MetOpGOMEDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                          write_all(parameters["write_all"].get<bool>())
        {
        }

        void MetOpGOMEDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/GOME";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            GOMEReader gome_reader;
            uint64_t gome_cadu = 0, ccsds = 0, gome_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 12 is MHS)
                if (vcdu.vcid == 24)
                {
                    gome_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 384)
                        {
                            gome_reader.work(pkt);
                            gome_ccsds++;
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

            logger->info("VCID 24 (GOME) Frames :" + std::to_string(gome_cadu));
            logger->info("CCSDS Frames          : " + std::to_string(ccsds));
            logger->info("GOME Frames           : " + std::to_string(gome_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            if (write_all)
            {
                if (!std::filesystem::exists(directory + "/GOME_ALL"))
                    std::filesystem::create_directory(directory + "/GOME_ALL");
                for (int i = 0; i < 6144; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(gome_reader.getChannel(i), directory + "/GOME_ALL/GOME-" + std::to_string(i + 1) + ".png");
                }
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            int all_width_count = 130;
            int all_height_count = 48;
            image::Image<uint16_t> imageAll(30 * all_width_count, gome_reader.getChannel(0).height() * all_height_count, 1);
            {
                int height = gome_reader.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 6144)
                            break;

                        imageAll.draw_image(0, gome_reader.getChannel(row * all_width_count + column), 30 * column, height * row);
                    }
                }
            }
            WRITE_IMAGE(imageAll, directory + "/GOME-ALL.png");
        }

        void MetOpGOMEDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp GOME Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpGOMEDecoderModule::getID()
        {
            return "metop_gome";
        }

        std::vector<std::string> MetOpGOMEDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpGOMEDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpGOMEDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace gome
} // namespace metop