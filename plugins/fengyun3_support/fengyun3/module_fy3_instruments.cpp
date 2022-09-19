#include "module_fy3_instruments.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/image/bowtie.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "products/dataset.h"
#include "common/simple_deframer.h"
#include "fengyun3.h"
#include "resources.h"

namespace fengyun3
{
    namespace instruments
    {
        FY3InstrumentsDecoderModule::FY3InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : ProcessingModule(input_file, output_file_hint, parameters),
              d_mersi_bowtie(d_parameters.contains("mersi_bowtie") ? d_parameters["mersi_bowtie"].get<bool>() : false)
        {
            if (parameters["satellite"] == "fy3ab")
                d_satellite = FY_AB;
            else if (parameters["satellite"] == "fy3c")
                d_satellite = FY_3C;
            else if (parameters["satellite"] == "fy3d")
                d_satellite = FY_3D;
            else if (parameters["satellite"] == "fy3e")
                d_satellite = FY_3E;
            else
                throw std::runtime_error("FY3 Instruments Decoder : FY3 satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");

            if (parameters["downlink"] == "ahrpt")
                d_downlink = AHRPT;
            else if (parameters["downlink"] == "mpt")
                d_downlink = MPT;
            else if (parameters["downlink"] == "dpt")
                d_downlink = DPT;
            else
                d_downlink = AHRPT;

            if (parameters.contains("dump_mersi"))
                d_dump_mersi = parameters["dump_mersi"].get<bool>();
            else
                d_dump_mersi = false;
        }

        void FY3InstrumentsDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;
            uint8_t cadu[1024];

            // Init stuff per-sat
            if (d_satellite == FY_3E)
            {
                std::string xeuvi_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/XEUVI";
                if (!std::filesystem::exists(xeuvi_directory))
                    std::filesystem::create_directory(xeuvi_directory);
                xeuvi_reader = std::make_unique<xeuvi::XEUVIReader>(xeuvi_directory);

                std::string windrad_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/WindRAD";
                if (!std::filesystem::exists(windrad_directory))
                    std::filesystem::create_directory(windrad_directory);
                if (!std::filesystem::exists(windrad_directory + "/C-Band"))
                    std::filesystem::create_directory(windrad_directory + "/C-Band");
                if (!std::filesystem::exists(windrad_directory + "/Ku-Band"))
                    std::filesystem::create_directory(windrad_directory + "/Ku-Band");
                windrad_reader1 = std::make_unique<windrad::WindRADReader>(758, "", windrad_directory + "/C-Band");
                windrad_reader2 = std::make_unique<windrad::WindRADReader>(1065, "", windrad_directory + "/Ku-Band");
            }
            else if (d_satellite == FY_3D)
            {
                std::string wai_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/WAI";
                if (!std::filesystem::exists(wai_directory))
                    std::filesystem::create_directory(wai_directory);
                wai_reader = std::make_unique<wai::WAIReader>(wai_directory);
            }

            // Demuxers
            ccsds::ccsds_1_0_1024::Demuxer demuxer_vcid12(882, true);

            // Deframer
            def::SimpleDeframer virr_deframer(0b101000010001011011111101011100011001110110000011110010010101, 60, 208400, 0);
            def::SimpleDeframer mwri_deframer(0xeb901acffc1d, 48, 60368, 0);
            def::SimpleDeframer xeuvi_deframer(0x55aa55aa55aa, 48, 519328, 0);
            def::SimpleDeframer waai_deframer(0x55aa55aa55aa, 48, 524336, 0);
            def::SimpleDeframer windrad_deframer1(0xfaf355aa, 32, 13160, 0);
            def::SimpleDeframer windrad_deframer2(0xfaf3aabb, 32, 18072, 0);
            def::SimpleDeframer gas_deframer(0x1acffc7d, 32, 5363264, 0);

            std::vector<uint8_t> fy_scids;

            // std::ofstream gas_test("gas.frm");
            std::ofstream mersi_bin;

            if (d_dump_mersi)
            {
                std::string mersi_path = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/mersi.bin";
                logger->debug("Dumping mersi to " + mersi_path);
                mersi_bin.open(mersi_path, std::ios::binary);
            }

            // std::ofstream idk_out("idk_frm.bin");

            is_init = true;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.spacecraft_id == FY3_A_SCID ||
                    vcdu.spacecraft_id == FY3_B_SCID ||
                    vcdu.spacecraft_id == FY3_C_SCID ||
                    vcdu.spacecraft_id == FY3_D_SCID ||
                    vcdu.spacecraft_id == FY3_E_SCID)
                    fy_scids.push_back(vcdu.spacecraft_id);

                if (d_satellite == FY_AB)
                {
                    if (d_downlink == DPT || d_downlink == AHRPT)
                    {
                        if (vcdu.vcid == 5) // VIRR
                        {
                            std::vector<std::vector<uint8_t>> out = virr_deframer.work(&cadu[14], 882);
                            for (std::vector<uint8_t> frameVec : out)
                                virr_reader.work(frameVec);
                        }
                        else if (vcdu.vcid == 12) // CCSDS-Compliant VCID
                        {
                            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                                if (pkt.header.apid == 16) // MWHS-1
                                    mwhs1_reader.work(pkt);
                                else if (pkt.header.apid == 2) // MWTS-1
                                    mwts1_reader.work(pkt);
                                else if (pkt.header.apid == 5) // ERM
                                    erm_reader.work(pkt);
                        }
                    }
                    else if (d_downlink == DPT || d_downlink == MPT)
                    {
                        if (vcdu.vcid == 3) // MERSI-1
                        {
                            mersi1_reader.work(&cadu[14], 882);
                            if (d_dump_mersi)
                                mersi_bin.write((char *)&cadu[14], 882);
                        }
                    }
                }
                else if (d_satellite == FY_3C)
                {
                    if (d_downlink == DPT || d_downlink == AHRPT)
                    {
                        if (vcdu.vcid == 5) // VIRR
                        {
                            std::vector<std::vector<uint8_t>> out = virr_deframer.work(&cadu[14], 882);
                            for (std::vector<uint8_t> frameVec : out)
                                virr_reader.work(frameVec);
                        }
                        else if (vcdu.vcid == 12) // CCSDS-Compliant VCID
                        {
                            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                                if (pkt.header.apid == 16) // MWHS-2
                                    mwhs2_reader.work(pkt, false);
                                else if (pkt.header.apid == 5) // ERM
                                    erm_reader.work(pkt);
                        }
                    }
                }
                else if (d_satellite == FY_3D)
                {
                    if (vcdu.vcid == 3) // MERSI-2
                    {
                        mersi2_reader.work(&cadu[14], 882);
                        if (d_dump_mersi)
                            mersi_bin.write((char *)&cadu[14], 882);
                    }
                    // else if (vcdu.vcid == 6) // HIRAS-1
                    //{                        // 0x87762226 0x316e4f02
                    //      hiras_test.write((char *)&cadu[14], 882);
                    //}
                    else if (vcdu.vcid == 9) // GAS
                    {
                        std::vector<std::vector<uint8_t>> out = gas_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            gas_reader.work(frameVec);
                    }
                    else if (vcdu.vcid == 5) // WAAI
                    {
                        std::vector<std::vector<uint8_t>> out = waai_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            wai_reader->work(frameVec);
                    }
                    else if (vcdu.vcid == 10) // MWRI
                    {
                        std::vector<std::vector<uint8_t>> out = mwri_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            mwri_reader.work(frameVec);
                    }
                    else if (vcdu.vcid == 12) // CCSDS-Compliant VCID
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 16) // MWHS-2
                                mwhs2_reader.work(pkt, false);
                            else if (pkt.header.apid == 7) // MWTS-2
                                mwts2_reader.work(pkt);
                        // else if (pkt.header.apid == 17)
                        //     continue; // idk_out.write((char *)pkt.payload.data(), 250);
                        // else if (pkt.header.apid == 3)
                        //     continue; // idk_out.write((char *)pkt.payload.data(), 506);
                        // else if (pkt.header.apid == 15)
                        //     continue; // idk_out.write((char *)pkt.payload.data(), 506);
                        // else if (pkt.header.apid == 1)
                        //     continue; // idk_out.write((char *)pkt.payload.data(), 282);
                        //               // else logger->critical("APID {:d}           {:d}", pkt.header.apid, pkt.payload.size());
                    }
                    // else
                    // {
                    //    // 11 : 0b10100000011000000110000000110000100010100011
                    //    if (vcdu.vcid == 11)
                    //        idk_out.write((char *)&cadu[14], 882);
                    //    else
                    //        logger->critical("VCID {:d}", vcdu.vcid);
                    //}
                }
                else if (d_satellite == FY_3E)
                {
                    if (vcdu.vcid == 3) // MERSI-LL
                    {
                        mersill_reader.work(&cadu[14], 882);
                        if (d_dump_mersi)
                            mersi_bin.write((char *)&cadu[14], 882);
                    }
                    else if (vcdu.vcid == 5) // XEUVI
                    {
                        std::vector<std::vector<uint8_t>> out = xeuvi_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            xeuvi_reader->work(frameVec);
                    }
                    else if (vcdu.vcid == 10) // WindRad 1
                    {
                        std::vector<std::vector<uint8_t>> out = windrad_deframer1.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            windrad_reader1->work(frameVec);
                    }
                    else if (vcdu.vcid == 18) // WindRad 2
                    {
                        std::vector<std::vector<uint8_t>> out = windrad_deframer2.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            windrad_reader2->work(frameVec);
                    }
                    else if (vcdu.vcid == 12) // CCSDS-Compliant VCID
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 16) // MWHS-2
                                mwhs2_reader.work(pkt, true);
                            else if (pkt.header.apid == 7) // MWTS-3
                                mwts3_reader.work(pkt);
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

            if (d_dump_mersi)
                mersi_bin.close();

            int scid = most_common(fy_scids.begin(), fy_scids.end());
            fy_scids.clear();

            std::string sat_name = "Unknown FengYun-3";
            if (scid == FY3_A_SCID)
                sat_name = "FengYun-3A";
            else if (scid == FY3_B_SCID)
                sat_name = "FengYun-3B";
            else if (scid == FY3_C_SCID)
                sat_name = "FengYun-3C";
            else if (scid == FY3_D_SCID)
                sat_name = "FengYun-3D";
            else if (scid == FY3_E_SCID)
                sat_name = "FengYun-3E";

            int norad = 0;
            if (scid == FY3_A_SCID)
                norad = FY3_A_NORAD;
            else if (scid == FY3_B_SCID)
                norad = FY3_B_NORAD;
            else if (scid == FY3_C_SCID)
                norad = FY3_C_NORAD;
            else if (scid == FY3_D_SCID)
                norad = FY3_D_NORAD;
            else if (scid == FY3_E_SCID)
                norad = FY3_E_NORAD;

            std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry.get_from_norad(norad);

            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = sat_name;
            // dataset.timestamp = avg_overflowless(avhrr_reader.timestamps);

            // Satellite ID
            {
                logger->info("----------- Satellite");
                logger->info("NORAD : " + std::to_string(norad));
                logger->info("Name  : " + sat_name);
            }

            if (d_satellite == FY_3E) // XEUVI
            {
                xeuvi_status = SAVING;

                logger->info("----------- XEUVI");
                logger->info("Images : " + std::to_string(xeuvi_reader->images_count));

                xeuvi_reader->writeCurrent();

                xeuvi_status = DONE;
            }

            if (d_satellite == FY_3E) // WindRad
            {
                windrad_C_status = windrad_Ku_status = SAVING;

                logger->info("----------- WindRAD");
                logger->info("C-Band Frames : " + std::to_string(windrad_reader1->imgCount));
                logger->info("Ku-Band Frames : " + std::to_string(windrad_reader2->imgCount));

                windrad_C_status = windrad_Ku_status = DONE;
            }

            if (d_satellite == FY_AB && (d_downlink == DPT || d_downlink == AHRPT)) // MWHS-1
            {
                mwhs1_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWHS-1";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWHS-1");
                logger->info("Lines : " + std::to_string(mwhs1_reader.lines));

                satdump::ImageProducts mwhs1_products;
                mwhs1_products.instrument_name = "mwhs1";
                mwhs1_products.has_timestamps = true;
                mwhs1_products.set_tle(satellite_tle);
                mwhs1_products.bit_depth = 16;
                mwhs1_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mwhs1_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_ab_mwhs1.json")));

                for (int i = 0; i < 5; i++)
                    mwhs1_products.images.push_back({"MWHS1-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mwhs1_reader.getChannel(i)});

                mwhs1_products.set_timestamps(mwhs1_reader.timestamps);

                mwhs1_products.save(directory);
                dataset.products_list.push_back("MWHS-1");

                mwhs1_status = DONE;
            }

            if (d_satellite == FY_3E || d_satellite == FY_3D || d_satellite == FY_3C) // MWHS-2
            {
                mwhs2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWHS-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWHS-2");
                logger->info("Lines : " + std::to_string(mwhs2_reader.lines));

                satdump::ImageProducts mwhs2_products;
                mwhs2_products.instrument_name = "mwhs2";
                mwhs2_products.has_timestamps = true;
                mwhs2_products.set_tle(satellite_tle);
                mwhs2_products.bit_depth = 16;
                mwhs2_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mwhs2_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_cde_mwhs2.json")));

                for (int i = 0; i < 15; i++)
                    mwhs2_products.images.push_back({"MWHS2-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mwhs2_reader.getChannel(i)});

                mwhs2_products.set_timestamps(mwhs2_reader.timestamps);

                mwhs2_products.save(directory);
                dataset.products_list.push_back("MWHS-2");

                mwhs2_status = DONE;
            }

            if ((d_satellite == FY_AB || d_satellite == FY_3C) && (d_downlink == DPT || d_downlink == AHRPT)) // VIRR
            {
                virr_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/VIRR";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- VIRR");
                logger->info("Lines : " + std::to_string(virr_reader.lines));

                satdump::ImageProducts virr_products;
                virr_products.instrument_name = "virr";
                virr_products.has_timestamps = true;
                virr_products.set_tle(satellite_tle);
                virr_products.bit_depth = 10;
                virr_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                virr_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_abc_virr.json")));

                for (int i = 0; i < 10; i++)
                    virr_products.images.push_back({"VIRR-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), virr_reader.getChannel(i)});

                virr_products.set_timestamps(virr_reader.timestamps);

                virr_products.save(directory);
                dataset.products_list.push_back("VIRR");

                virr_status = DONE;
            }

            if (d_satellite == FY_AB) // MERSI-1
            {
                mersi1_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-1";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                // BowTie values
                const float alpha = 1.0 / 1.6;
                const float beta = 0.58333; // 1.0 - alpha;
                const long scanHeight_250 = 40;
                const long scanHeight_1000 = 10;

                logger->info("----------- MERSI-1");
                logger->info("Segments : " + std::to_string(mersi1_reader.segments));

                satdump::ImageProducts mersi1_products;
                mersi1_products.instrument_name = "mersi1";
                mersi1_products.has_timestamps = true;
                mersi1_products.set_tle(satellite_tle);
                mersi1_products.bit_depth = 12;
                mersi1_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;

                // Channel offsets relative to Ch1
                int offset[20] = {
                    0,
                    8,
                    0,
                    24,
                    0,

                    16,
                    0,
                    -16,
                    -16,
                    -8,
                    16,
                    24,
                    8,
                    -8,
                    16,
                    -16,
                    40,
                    -32,
                    32,
                    -24,
                };

                for (int i = 0; i < 20; i++)
                {
                    image::Image<uint16_t> image = mersi1_reader.getChannel(i);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 5 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    mersi1_products.images.push_back({"MERSI1-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), image, {}, -1, -1, offset[i]});
                }

                // virr_products.set_timestamps(mwts2_reader.timestamps);

                // mersi2_reader.getChannel(-1).save_png(directory + "/calib.png");

                mersi1_products.save(directory);
                dataset.products_list.push_back("MERSI-1");

                mersi1_status = DONE;
            }

            if (d_satellite == FY_3D) // MERSI-2
            {
                mersi2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                // BowTie values
                const float alpha = 1.0 / 1.6;
                const float beta = 0.58333; // 1.0 - alpha;
                const long scanHeight_250 = 40;
                const long scanHeight_1000 = 10;

                logger->info("----------- MERSI-2");
                logger->info("Segments : " + std::to_string(mersi2_reader.segments));

                satdump::ImageProducts mersi2_products;
                mersi2_products.instrument_name = "mersi2";
                mersi2_products.has_timestamps = true;
                mersi2_products.set_tle(satellite_tle);
                mersi2_products.bit_depth = 12;
                mersi2_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
                mersi2_products.set_timestamps(mersi2_reader.timestamps);
                mersi2_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_d_mersi2.json")));

                // Channel offsets relative to Ch1
                int offset[25] = {
                    0,
                    8,
                    0,
                    24,
                    -8,
                    16,

                    16,
                    32,
                    -32,
                    -24,
                    32,
                    0,
                    8,
                    -16,
                    -8,
                    16,
                    24,
                    8,
                    -8,
                    16,
                    -16,
                    32,
                    32,
                    -32,
                    -24,
                };

                for (int i = 0; i < 25; i++)
                {
                    image::Image<uint16_t> image = mersi2_reader.getChannel(i);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 6 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    mersi2_products.images.push_back({"MERSI2-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), image, {}, -1, -1, offset[i]});
                }

                // mersi2_reader.getChannel(-1).save_png(directory + "/calib.png");

                mersi2_products.save(directory);
                dataset.products_list.push_back("MERSI-2");

                mersi2_status = DONE;
            }

            if (d_satellite == FY_3E) // MERSI-LL
            {
                mersill_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-LL";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                // BowTie values
                const float alpha = 1.0 / 1.6;
                const float beta = 0.58333; // 1.0 - alpha;
                const long scanHeight_250 = 40;
                const long scanHeight_1000 = 10;

                logger->info("----------- MERSI-LL");
                logger->info("Segments : " + std::to_string(mersill_reader.segments));

                satdump::ImageProducts mersill_products;
                mersill_products.instrument_name = "mersill";
                mersill_products.has_timestamps = true;
                mersill_products.set_tle(satellite_tle);
                mersill_products.bit_depth = 12;
                mersill_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_MULTIPLE_LINES;
                mersill_products.set_timestamps(mersill_reader.timestamps);
                mersill_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_e_mersill.json")));

                // Channel offsets relative to Ch1
                int offset[18] = {
                    0,
                    16,

                    0,
                    -16,
                    32,
                    8,
                    16,
                    -24,
                    16,
                    16,
                    16,
                    16,
                    16,
                    16,
                    16,
                    16,
                    16,
                    16,
                };

                for (int i = 0; i < 18; i++)
                {
                    image::Image<uint16_t> image = mersill_reader.getChannel(i);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 2 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    mersill_products.images.push_back({"MERSILL-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), image, {}, -1, -1, offset[i]});
                }

                // mersill_reader.getChannel(-1).save_png(directory + "/calib.png");

                mersill_products.save(directory);
                dataset.products_list.push_back("MERSI-LL");

                mersill_status = DONE;
            }

            if (d_satellite == FY_3D) // MWRI
            {
                mwri_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWRI";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWRI");
                logger->info("Lines : " + std::to_string(mwri_reader.lines));

                satdump::ImageProducts mwri_products;
                mwri_products.instrument_name = "mwri";
                mwri_products.has_timestamps = true;
                mwri_products.set_tle(satellite_tle);
                mwri_products.bit_depth = 12;
                mwri_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mwri_products.set_timestamps(mwri_reader.timestamps);
                mwri_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_abcd_mwri1.json")));

                for (int i = 0; i < 10; i++)
                    mwri_products.images.push_back({"MWRI-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mwri_reader.getChannel(i)});

                mwri_products.save(directory);
                dataset.products_list.push_back("MWRI");

                mwri_status = DONE;
            }

            if (d_satellite == FY_3D) // WAI
            {
                wai_status = SAVING;

                logger->info("----------- WAI");
                logger->info("Frames : " + std::to_string(wai_reader->images_count));

                wai_status = DONE;
            }

            if (d_satellite == FY_3D) // GAS
            {
                gas_status = SAVING;

                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/GAS";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- GAS");
                logger->info("Lines : " + std::to_string(gas_reader.lines));

                gas_reader.getChannel().save_png(directory + "/GAS.png");

                gas_status = DONE;
            }

            if ((d_satellite == FY_AB || d_satellite == FY_3C) && (d_downlink == DPT || d_downlink == AHRPT)) // ERM
            {
                erm_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/ERM";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- ERM");
                logger->info("Lines : " + std::to_string(erm_reader.lines));

                satdump::ImageProducts erm_products;
                erm_products.instrument_name = "erm";
                erm_products.has_timestamps = true;
                erm_products.set_tle(satellite_tle);
                erm_products.bit_depth = 16;
                erm_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                erm_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_abc_erm.json")));

                erm_products.images.push_back({"ERM-1.png", "1", erm_reader.getChannel()});

                erm_products.set_timestamps(erm_reader.timestamps);

                erm_products.save(directory);
                dataset.products_list.push_back("ERM");

                erm_status = DONE;
            }

            if (d_satellite == FY_AB && (d_downlink == DPT || d_downlink == AHRPT)) // MWTS-1
            {
                mwts1_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWTS-1";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWTS-1");
                logger->info("Lines : " + std::to_string(mwts1_reader.lines));

                satdump::ImageProducts mwts1_products;
                mwts1_products.instrument_name = "mwts1";
                mwts1_products.has_timestamps = true;
                mwts1_products.set_tle(satellite_tle);
                mwts1_products.bit_depth = 16;
                mwts1_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mwts1_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_ab_mwts1.json")));

                for (int i = 0; i < 27; i++)
                    mwts1_products.images.push_back({"MWTS1-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mwts1_reader.getChannel(i)});

                mwts1_products.set_timestamps(mwts2_reader.timestamps);

                mwts1_products.save(directory);
                dataset.products_list.push_back("MWTS-1");

                mwts1_status = DONE;
            }

            if (d_satellite == FY_3D) // MWTS-2
            {
                mwts2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWTS-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWTS-2");
                logger->info("Lines : " + std::to_string(mwts2_reader.lines));

                satdump::ImageProducts mwts2_products;
                mwts2_products.instrument_name = "mwts2";
                mwts2_products.has_timestamps = true;
                mwts2_products.set_tle(satellite_tle);
                mwts2_products.bit_depth = 16;
                mwts2_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mwts2_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_d_mwts2.json")));

                for (int i = 0; i < 18; i++)
                    mwts2_products.images.push_back({"MWTS2-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mwts2_reader.getChannel(i)});

                mwts2_products.set_timestamps(mwts2_reader.timestamps);

                mwts2_products.save(directory);
                dataset.products_list.push_back("MWTS-2");

                mwts2_status = DONE;
            }

            if (d_satellite == FY_3E) // MWTS-3
            {
                mwts3_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWTS-3";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWTS-3");
                logger->info("Lines : " + std::to_string(mwhs2_reader.lines));

                satdump::ImageProducts mwts3_products;
                mwts3_products.instrument_name = "mwts3";
                mwts3_products.has_timestamps = true;
                mwts3_products.set_tle(satellite_tle);
                mwts3_products.bit_depth = 16;
                mwts3_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
                mwts3_products.set_proj_cfg(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_e_mwts3.json")));

                for (int i = 0; i < 18; i++)
                    mwts3_products.images.push_back({"MWTS3-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), mwts3_reader.getChannel(i)});

                mwts3_products.set_timestamps(mwts3_reader.timestamps);

                mwts3_products.save(directory);
                dataset.products_list.push_back("MWTS-3");

                mwts3_status = DONE;
            }

            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }

        void FY3InstrumentsDecoderModule::drawUI(bool window)
        {
            if (!is_init)
                return;

            ImGui::Begin("FengYun-3 Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (ImGui::BeginTable("##fy3instrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Instrument");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Lines / Frames");
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("Status");

                if (d_satellite == FY_AB && (d_downlink == MPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mersi1_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersi1_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mersi2_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersi2_status);
                }

                if (d_satellite == FY_3E)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-LL");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mersill_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersill_status);
                }

                if ((d_satellite == FY_AB || d_satellite == FY_3C) && (d_downlink == AHRPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("VIRR");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", virr_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(virr_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("ERM");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", erm_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(erm_status);
                }

                if (d_satellite == FY_AB && (d_downlink == AHRPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWTS-1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mwts1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwts1_status);
                }

                if (d_satellite == FY_AB && (d_downlink == AHRPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWHS-1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mwhs1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwhs1_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWTS-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mwts2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwts2_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWRI");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mwri_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwri_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("WAI");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", wai_reader->images_count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(wai_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("GAS");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", gas_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(gas_status);
                }

                if (d_satellite == FY_3E)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWTS-3");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mwts3_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwts3_status);
                }

                if (d_satellite == FY_3E || d_satellite == FY_3D || d_satellite == FY_3C)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWHS-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", mwhs2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwhs2_status);
                }

                if (d_satellite == FY_3E)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("XEUVI");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", xeuvi_reader->images_count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(xeuvi_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("WindRAD C-Band");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", windrad_reader1->imgCount);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(windrad_C_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("WindRAD Ku-Band");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImColor(0, 255, 0), "%d", windrad_reader2->imgCount);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(windrad_Ku_status);
                }

                ImGui::EndTable();
            }

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FY3InstrumentsDecoderModule::getID()
        {
            return "fy3_instruments";
        }

        std::vector<std::string> FY3InstrumentsDecoderModule::getParameters()
        {
            return {"satellite", "mersi_bowtie"};
        }

        std::shared_ptr<ProcessingModule> FY3InstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FY3InstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace modis
} // namespace eos