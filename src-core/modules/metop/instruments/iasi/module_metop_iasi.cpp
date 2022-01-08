#include "module_metop_iasi.h"
#include <fstream>
#include "iasi_imaging_reader.h"
#include "iasi_reader.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/geodetic/projection/satellite_reprojector.h"
#include "nlohmann/json_utils.h"
#include "common/geodetic/projection/proj_file.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace iasi
    {
        MetOpIASIDecoderModule::MetOpIASIDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                          write_all(parameters["write_all"].get<bool>())
        {
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

            const float alpha = 1.0 / 1.8;
            const float beta = 0.58343;
            const long scanHeight = 64;

            if (iasireader.lines > 0)
            {
                logger->info("Channel IR imaging...");
                image::Image<uint16_t> iasi_imaging = iasireader_img.getIRChannel();
                iasi_imaging = image::bowtie::correctGenericBowTie(iasi_imaging, 1, scanHeight, alpha, beta); // Bowtie.... As IASI scans per IFOV
                iasi_imaging.simple_despeckle(10);                                                            // And, it has some dead pixels sometimes so well, we need to remove them I guess?

                image::Image<uint16_t> iasi_imaging_equ = iasi_imaging;
                image::Image<uint16_t> iasi_imaging_equ_inv = iasi_imaging;
                WRITE_IMAGE(iasi_imaging, directory + "/IASI-IMG.png");
                iasi_imaging_equ.equalize();
                iasi_imaging_equ.normalize();
                WRITE_IMAGE(iasi_imaging_equ, directory + "/IASI-IMG-EQU.png");
                iasi_imaging_equ_inv.linear_invert();
                iasi_imaging_equ_inv.equalize();
                iasi_imaging_equ_inv.normalize();
                WRITE_IMAGE(iasi_imaging_equ_inv, directory + "/IASI-IMG-EQU-INV.png");

                /*cimg_library::CImg<unsigned char> thermalTest(iasi_imaging.width(), iasi_imaging.height(), 1, 3, 0);
            cimg_library::CImg<unsigned char> LUT;
            LUT.load_png("/home/alan/Downloads/crappyLut.png");
            for (int i = 0; i < iasi_imaging.width() * iasi_imaging.height(); i++)
            {
                thermalTest[iasi_imaging.width() * iasi_imaging.height() * 0 + i] = LUT[LUT.width() * 0 + 65535 - iasi_imaging_equ[i]];
                thermalTest[iasi_imaging.width() * iasi_imaging.height() * 1 + i] = LUT[LUT.width() * 1 + 65535 - iasi_imaging_equ[i]];
                thermalTest[iasi_imaging.width() * iasi_imaging.height() * 2 + i] = LUT[LUT.width() * 2 + 65535 - iasi_imaging_equ[i]];
            }
            WRITE_IMAGE(thermalTest, directory + "/IASI-IMG-LUT.png");
            */

                // Reproject to an equirectangular proj
                if (iasi_imaging.height() > 0)
                {
                    nlohmann::json satData = loadJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json");
                    int norad = satData.contains("norad") > 0 ? satData["norad"].get<int>() : 0;
                    //image4.equalize(1000);

                    // Setup Projecition
                    std::shared_ptr<geodetic::projection::LEOScanProjectorSettings_IFOV> proj_settings = std::make_shared<geodetic::projection::LEOScanProjectorSettings_IFOV>(
                        100.0,                         // Scan angle
                        3.4,                           // IFOV X scan angle
                        3.6,                           // IFOV Y scan angle
                        -1.7,                          // Roll offset
                        0,                             // Pitch offset
                        0.2,                           // Yaw offset
                        0,                             // Time offset
                        30,                            // Number of IFOVs
                        64,                            // IFOV Width
                        64,                            // IFOV Height
                        iasi_imaging.width(),          // Image width
                        true,                          // Invert scan
                        tle::getTLEfromNORAD(norad),   // TLEs
                        iasireader_img.timestamps_ifov // Timestamps
                    );
                    geodetic::projection::LEOScanProjector projector(proj_settings);

                    {
                        geodetic::projection::proj_file::LEO_GeodeticReferenceFile geofile = geodetic::projection::proj_file::leoRefFileFromProjector(norad, proj_settings);
                        geodetic::projection::proj_file::writeReferenceFile(geofile, directory + "/IASI.georef");
                    }

                    logger->info("Projected imaging channel...");
                    image::Image<uint8_t> projected_image = geodetic::projection::projectLEOToEquirectangularMapped(iasi_imaging_equ, projector, 2048 * 4, 1024 * 4, 1);
                    WRITE_IMAGE(projected_image, directory + "/IASI-EQU-PROJ.png");

                    logger->info("Projected imaging channel inverted...");
                    projected_image = geodetic::projection::projectLEOToEquirectangularMapped(iasi_imaging_equ_inv, projector, 2048 * 4, 1024 * 4, 1);
                    WRITE_IMAGE(projected_image, directory + "/IASI-EQU-INV-PROJ.png");

                    /*
                logger->info("Projected channel IMG LUT...");
                cimg_library::CImg<unsigned short> lut_16 = thermalTest;
                lut_16 <<= 8;
                projected_image = geodetic::projection::projectLEOToEquirectangularMapped(lut_16, projector, 2048 * 4 * 5, 1024 * 4 * 5, 3);
                logger->info("Write");
                projected_image.crop(18788, 952, 18788 + 9521, 952 + 6352);
                WRITE_IMAGE(projected_image, directory + "/IASI-LUT-PROJ.png");
                */
                }

                // Output a few nice composites as well
                logger->info("Global Composite...");
                int all_width_count = 150;
                int all_height_count = 60;
                image::Image<uint16_t> imageAll(60 * all_width_count, iasireader.getChannel(0).height() * all_height_count, 1);
                {
                    int height = iasireader.getChannel(0).height();

                    for (int row = 0; row < all_height_count; row++)
                    {
                        for (int column = 0; column < all_width_count; column++)
                        {
                            if (row * all_width_count + column >= 8461)
                                break;

                            imageAll.draw_image(0, iasireader.getChannel(row * all_width_count + column), 60 * column, height * row);
                        }
                    }
                }
                WRITE_IMAGE(imageAll, directory + "/IASI-ALL.png");
            }
        }

        void MetOpIASIDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp IASI Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

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

        std::shared_ptr<ProcessingModule> MetOpIASIDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpIASIDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace iasi
} // namespace metop