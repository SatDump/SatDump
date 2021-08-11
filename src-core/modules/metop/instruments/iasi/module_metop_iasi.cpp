#include "module_metop_iasi.h"
#include <fstream>
#include "iasi_imaging_reader.h"
#include "iasi_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/image.h"
#include "common/image/fft.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace iasi
    {
        MetOpIASIDecoderModule::MetOpIASIDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                              write_all(std::stoi(parameters["write_all"]))
        {
            windowTitle = "MetOp IASI Decoder";
        }

        void MetOpIASIDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IASI";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);

            IASIReader iasireader;
            IASIIMGReader iasireader_img;

            uint64_t iasi_cadu = 0, ccsds = 0, iasi_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                // Right channel? (VCID 10 is IASI)
                if (vcdu.vcid == 10)
                {
                    iasi_cadu++;

                    // Demux
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

                    // Count frames
                    ccsds += ccsdsFrames.size();

                    // Push into processor (filtering APID 64)
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 150)
                        {
                            iasireader_img.work(pkt);
                            iasi_ccsds++;
                        }
                        else if (pkt.header.apid == 130 || pkt.header.apid == 135 || pkt.header.apid == 140 || pkt.header.apid == 145)
                        {
                            iasireader.work(pkt);
                            iasi_ccsds++;
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

            logger->info("VCID 10 (IASI) Frames : " + std::to_string(iasi_cadu));
            logger->info("CCSDS Frames          :  " + std::to_string(ccsds));
            logger->info("IASI Frames           : " + std::to_string(iasi_ccsds));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            if (write_all)
            {
                if (!std::filesystem::exists(directory + "/IASI_all"))
                    std::filesystem::create_directory(directory + "/IASI_ALL");

                for (int i = 0; i < 8461; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(iasireader.getChannel(i), directory + "/IASI_ALL/IASI-" + std::to_string(i + 1) + ".png");
                }
            }

            logger->info("Channel IR imaging...");
            cimg_library::CImg<unsigned short> iasi_imaging = iasireader_img.getIRChannel();
            cimg_library::CImg<unsigned short> iasi_imaging_equ = iasi_imaging;
            iasi_imaging_equ.equalize(1000);
            iasi_imaging_equ.normalize(0, 65535);
            WRITE_IMAGE(iasi_imaging_equ, directory + "/IASI-IMG.png");

            if (iasi_imaging.height() > 0)
            {
                image::simple_despeckle(iasi_imaging, 10);
                image::fft_forward(iasi_imaging);
                image::extract_percentile(iasi_imaging, 4.0, 94.0, 1);
                image::fft_inverse(iasi_imaging);

                cimg_library::CImg<unsigned short> iasi_imaging_equ_denoised = iasi_imaging;
                iasi_imaging_equ_denoised.equalize(1000);
                iasi_imaging_equ_denoised.normalize(0, 65535);

                WRITE_IMAGE(iasi_imaging_equ_denoised, directory + "/IASI-IMG-DENOISED-EQU.png");

                image::linear_invert(iasi_imaging);
                iasi_imaging.equalize(1000);
                iasi_imaging.normalize(0, 65535);

                WRITE_IMAGE(iasi_imaging, directory + "/IASI-IMG-DENOISED-EQU-INV.png");
            }

            // Output a few nice composites as well
            logger->info("Global Composite...");
            int all_width_count = 150;
            int all_height_count = 60;
            cimg_library::CImg<unsigned short> imageAll(60 * all_width_count, iasireader.getChannel(0).height() * all_height_count, 1, 1);
            {
                int height = iasireader.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 8461)
                            break;

                        imageAll.draw_image(60 * column, height * row, 0, 0, iasireader.getChannel(row * all_width_count + column));
                    }
                }
            }
            WRITE_IMAGE(imageAll, directory + "/IASI-ALL.png");
        }

        void MetOpIASIDecoderModule::drawUI(bool window)
        {
            ImGui::Begin(windowTitle.c_str(), NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpIASIDecoderModule::getID()
        {
            return "metop_iasi";
        }

        std::vector<std::string> MetOpIASIDecoderModule::getParameters()
        {
            return {"write_all"};
        }

        std::shared_ptr<ProcessingModule> MetOpIASIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<MetOpIASIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace iasi
} // namespace metop