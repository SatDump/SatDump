#include "module_metop_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "metop.h"
#include "common/image/bowtie.h"
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "instruments/avhrr/avhrr_reader.h"
#include "instruments/mhs/mhs_reader.h"
#include "instruments/ascat/ascat_reader.h"
#include "instruments/iasi/iasi_reader.h"
#include "instruments/iasi/iasi_imaging_reader.h"
#include "instruments/amsu/amsu_a1_reader.h"
#include "instruments/amsu/amsu_a2_reader.h"
#include "instruments/gome/gome_reader.h"

namespace metop
{
    namespace instruments
    {
        MetOpInstrumentsDModule::MetOpInstrumentsDModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpInstrumentsDModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            // Demuxers
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid3(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid9(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid10(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid12(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid15(882, true);
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid24(882, true);

            // Readers
            avhrr::AVHRRReader avhrr_reader;
            mhs::MHSReader mhs_reader;
            ascat::ASCATReader ascat_reader;
            iasi::IASIReader iasi_reader;
            iasi::IASIIMGReader iasi_reader_img;
            amsu::AMSUA1Reader amsu_a1_reader;
            amsu::AMSUA2Reader amsu_a2_reader;
            gome::GOMEReader gome_reader;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.vcid == 9) // AVHRR/3
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid9.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 103 || pkt.header.apid == 104)
                            avhrr_reader.work(pkt);
                }
                else if (vcdu.vcid == 12) // MHS
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 34)
                            mhs_reader.work(pkt);
                }
                else if (vcdu.vcid == 15) // ASCAT
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid15.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        ascat_reader.work(pkt);
                }
                else if (vcdu.vcid == 10) // IASI
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid10.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 150)
                            iasi_reader_img.work(pkt);
                        else if (pkt.header.apid == 130 || pkt.header.apid == 135 || pkt.header.apid == 140 || pkt.header.apid == 145)
                            iasi_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 3) // AMSU
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    {
                        if (pkt.header.apid == 39)
                            amsu_a1_reader.work(pkt);
                        else if (pkt.header.apid == 40)
                            amsu_a2_reader.work(pkt);
                    }
                }
                else if (vcdu.vcid == 24) // GOME
                {
                    std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid24.work(cadu);
                    for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        if (pkt.header.apid == 384)
                            gome_reader.work(pkt);
                }

                progress = data_in.tellg();
                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            // AVHRR
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AVHRR/3");

                logger->info("Channel 1...");
                WRITE_IMAGE(avhrr_reader.getChannel(0), directory + "/AVHRR-1.png");

                logger->info("Channel 2...");
                WRITE_IMAGE(avhrr_reader.getChannel(1), directory + "/AVHRR-2.png");

                logger->info("Channel 3...");
                WRITE_IMAGE(avhrr_reader.getChannel(2), directory + "/AVHRR-3.png");

                logger->info("Channel 4...");
                WRITE_IMAGE(avhrr_reader.getChannel(3), directory + "/AVHRR-4.png");

                logger->info("Channel 5...");
                WRITE_IMAGE(avhrr_reader.getChannel(4), directory + "/AVHRR-5.png");
            }

            // MHS
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MHS");

                for (int i = 0; i < 5; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(mhs_reader.getChannel(i), directory + "/MHS-" + std::to_string(i + 1) + ".png");
                }
            }

            // ASCAT
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ASCAT";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- ASCAT");

                for (int i = 0; i < 6; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    image::Image<uint16_t> img = ascat_reader.getChannel(i);
                    WRITE_IMAGE(img, directory + "/ASCAT-" + std::to_string(i + 1) + ".png");
                }
            }

            // IASI
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/IASI";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- IASI");

                const float alpha = 1.0 / 1.8;
                const float beta = 0.58343;
                const long scanHeight = 64;

                if (iasi_reader.lines > 0)
                {
                    logger->info("Channel IR imaging...");
                    image::Image<uint16_t> iasi_imaging = iasi_reader_img.getIRChannel();
                    iasi_imaging = image::bowtie::correctGenericBowTie(iasi_imaging, 1, scanHeight, alpha, beta); // Bowtie.... As IASI scans per IFOV
                    iasi_imaging.simple_despeckle(10);                                                            // And, it has some dead pixels sometimes so well, we need to remove them I guess?

                    image::Image<uint16_t> iasi_imaging_equ = iasi_imaging;
                    image::Image<uint16_t> iasi_imaging_equ_inv = iasi_imaging;
                    WRITE_IMAGE(iasi_imaging, directory + "/IASI-IMG.png");
                }

                // Output a few nice composites as well
                logger->info("Global Composite...");
                int all_width_count = 150;
                int all_height_count = 60;
                image::Image<uint16_t> imageAll(60 * all_width_count, iasi_reader.getChannel(0).height() * all_height_count, 1);
                {
                    int height = iasi_reader.getChannel(0).height();

                    for (int row = 0; row < all_height_count; row++)
                    {
                        for (int column = 0; column < all_width_count; column++)
                        {
                            if (row * all_width_count + column >= 8461)
                                break;

                            imageAll.draw_image(0, iasi_reader.getChannel(row * all_width_count + column), 60 * column, height * row);
                        }
                    }
                }
                WRITE_IMAGE(imageAll, directory + "/IASI-ALL.png");
            }

            // AMSU
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AMSU";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- AMSU");

                for (int i = 0; i < 2; i++)
                {
                    logger->info("Channel " + std::to_string(i + 1) + "...");
                    WRITE_IMAGE(amsu_a2_reader.getChannel(i), directory + "/AMSU-A2-" + std::to_string(i + 1) + ".png");
                    // WRITE_IMAGE(amsu_a2_reader.getChannel(i).equalize().normalize(), directory + "/AMSU-A2-" + std::to_string(i + 1) + "-EQU.png");
                }

                for (int i = 0; i < 13; i++)
                {
                    logger->info("Channel " + std::to_string(i + 3) + "...");
                    WRITE_IMAGE(amsu_a1_reader.getChannel(i), directory + "/AMSU-A1-" + std::to_string(i + 3) + ".png");
                    // WRITE_IMAGE(amsu_a1_reader.getChannel(i).equalize().normalize(), directory + "/AMSU-A1-" + std::to_string(i + 3) + "-EQU.png");
                }
            }

            // GOME
            {
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/GOME";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- GOME");

                // Output a few nice composites as well
                logger->info("Global Composite...");
                int all_width_count = 130;
                int all_height_count = 48;
                image::Image<uint16_t> imageAll(15 * all_width_count, gome_reader.lines * all_height_count, 1);
                {
                    int height = gome_reader.lines;

                    for (int row = 0; row < all_height_count; row++)
                    {
                        for (int column = 0; column < all_width_count; column++)
                        {
                            if (row * all_width_count + column >= 6144)
                                break;

                            imageAll.draw_image(0, gome_reader.getChannel(row * all_width_count + column), 15 * column, height * row);
                        }
                    }
                }
                WRITE_IMAGE(imageAll, directory + "/GOME-ALL.png");
                imageAll.clear();
            }

            data_in.close();
        }

        void MetOpInstrumentsDModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp Instruments Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpInstrumentsDModule::getID()
        {
            return "metop_instruments";
        }

        std::vector<std::string> MetOpInstrumentsDModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpInstrumentsDModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<MetOpInstrumentsDModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop