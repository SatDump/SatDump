#include "module_fy3_instruments.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/simple_deframer.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/exception.h"
#include "core/resources.h"
#include "fengyun3.h"
#include "image/bowtie.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "instruments/mersi_histmatch.h"
#include "instruments/mersi_offset_interleaved.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fengyun3
{
    namespace instruments
    {
        FY3InstrumentsDecoderModule::FY3InstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters),
              d_mersi_bowtie(d_parameters.contains("mersi_bowtie") ? d_parameters["mersi_bowtie"].get<bool>() : false),
              d_mersi_histmatch(d_parameters.contains("mersi_histmatch") ? d_parameters["mersi_histmatch"].get<bool>() : false)
        {
            if (parameters["satellite"] == "fy3ab")
                d_satellite = FY_AB;
            else if (parameters["satellite"] == "fy3c")
                d_satellite = FY_3C;
            else if (parameters["satellite"] == "fy3d")
                d_satellite = FY_3D;
            else if (parameters["satellite"] == "fy3e")
                d_satellite = FY_3E;
            else if (parameters["satellite"] == "fy3f")
                d_satellite = FY_3F;
            else if (parameters["satellite"] == "fy3g")
                d_satellite = FY_3G;
            else
                throw satdump_exception("FY3 Instruments Decoder : FY3 satellite \"" + parameters["satellite"].get<std::string>() + "\" is not valid!");

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

            if (parameters.contains("write_c10"))
                d_write_c10 = parameters["write_c10"].get<bool>();
            else
                d_write_c10 = false;

            fsfsm_enable_output = false;
        }

        void FY3InstrumentsDecoderModule::process()
        {
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
            else if (d_satellite == FY_3G)
            {
                std::string pmr1_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/PMR-1";
                if (!std::filesystem::exists(pmr1_directory))
                    std::filesystem::create_directory(pmr1_directory);
                pmr1_reader = std::make_unique<pmr::PMRReader>(pmr1_directory);
                pmr1_reader->offset = 1;

                std::string pmr2_directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/PMR-2";
                if (!std::filesystem::exists(pmr2_directory))
                    std::filesystem::create_directory(pmr2_directory);
                pmr2_reader = std::make_unique<pmr::PMRReader>(pmr2_directory);
                pmr2_reader->offset = 1;
            }

            // Demuxers
            ccsds::ccsds_aos::Demuxer demuxer_vcid12(882, true);

            // Deframer
            def::SimpleDeframer virr_deframer(0b101000010001011011111101011100011001110110000011110010010101, 60, 208400, 0);
            def::SimpleDeframer mwri_deframer(0xeb901acffc1d, 48, 60368, 0);
            def::SimpleDeframer mwrirm_deframer(0xa7f8aaaaeb90352e, 64, 344000, 0);
            def::SimpleDeframer mwri2_deframer(0xa7f8aaaaeb90352e, 64, 344000, 0);
            def::SimpleDeframer xeuvi_deframer(0x55aa55aa55aa, 48, 519328, 0);
            def::SimpleDeframer waai_deframer(0x55aa55aa55aa, 48, 524336, 0);
            def::SimpleDeframer windrad_deframer1(0xfaf355aa, 32, 13160, 0);
            def::SimpleDeframer windrad_deframer2(0xfaf3aabb, 32, 18072, 0);
            def::SimpleDeframer gas_deframer(0x1acffc7d, 32, 5363264, 0);
            def::SimpleDeframer pmr1_deframer(0xcc5afe5a, 32, 29536, 0);
            def::SimpleDeframer pmr2_deframer(0xcc5afe5a, 32, 29536, 0);

            std::vector<uint8_t> fy_scids;

            // std::ofstream gas_test("gas.frm");
            std::ofstream mersi_bin;

            if (d_dump_mersi)
            {
                std::string mersi_path = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/mersi.bin";
                logger->debug("Dumping mersi to " + mersi_path);
                mersi_bin.open(mersi_path, std::ios::binary);
            }

            if (d_write_c10)
            {
                virr_to_c10 = new virr::VIRRToC10();
                virr_to_c10->open(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/tmp.c10");
            }

            // std::ofstream idk_out("idk_frm.bin");

            is_init = true;

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

                if (vcdu.spacecraft_id == FY3_A_SCID || vcdu.spacecraft_id == FY3_B_SCID || vcdu.spacecraft_id == FY3_C_SCID || vcdu.spacecraft_id == FY3_D_SCID || vcdu.spacecraft_id == FY3_E_SCID ||
                    vcdu.spacecraft_id == FY3_F_SCID || vcdu.spacecraft_id == FY3_G_SCID)
                    fy_scids.push_back(vcdu.spacecraft_id);

                if (d_satellite == FY_AB)
                {
                    if (d_downlink == DPT || d_downlink == AHRPT)
                    {
                        if (vcdu.vcid == 5) // VIRR
                        {
                            std::vector<std::vector<uint8_t>> out = virr_deframer.work(&cadu[14], 882);
                            for (std::vector<uint8_t> frameVec : out)
                            {
                                virr_reader.work(frameVec);
                                if (d_write_c10)
                                    virr_to_c10->work(frameVec);
                            }
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
                            {
                                virr_reader.work(frameVec);
                                if (d_write_c10)
                                    virr_to_c10->work(frameVec);
                            }
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
                        //               // else logger->critical("APID %d           %d", pkt.header.apid, pkt.payload.size());
                    }
                    // else
                    // {
                    //    // 11 : 0b10100000011000000110000000110000100010100011
                    //    if (vcdu.vcid == 11)
                    //        idk_out.write((char *)&cadu[14], 882);
                    //    else
                    //        logger->critical("VCID %d", vcdu.vcid);
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
                else if (d_satellite == FY_3F)
                {
                    // printf("VCID %d\n", vcdu.vcid);
                    // if (vcdu.vcid == 3) //
                    // {
                    //     idk_out.write((char *)&cadu[14], 882);
                    // }
                    if (vcdu.vcid == 3) // MERSI-LL
                    {
                        mersi3_reader.work(&cadu[14], 882);
                        if (d_dump_mersi)
                            mersi_bin.write((char *)&cadu[14], 882);
                    }
                    else if (vcdu.vcid == 42) // MWRI-2
                    {
                        std::vector<std::vector<uint8_t>> out = mwri2_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            mwri2_reader.work(frameVec);
                    }
                    else if (vcdu.vcid == 12) // CCSDS-Compliant VCID
                    {
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                            if (pkt.header.apid == 16) // MWHS-2
                                mwhs2_reader.work(pkt, true);
                            else if (pkt.header.apid == 7) // MWTS-3
                                mwts3_reader.work(pkt);
                        // else
                        //     printf("APID %d\n", pkt.header.apid);
                    }
                }
                else if (d_satellite == FY_3G)
                {
                    // 12 = 0x4fa3aa06 6944
                    // 27 0x5f5f5f
                    // printf("VCID %d\n", vcdu.vcid);
                    /*if (vcdu.vcid == 27) // 3) // MERSI-LL
                    {
                        //  mersirm_reader.work(&cadu[14], 882);
                        //  if (d_dump_mersi)
                        //      mersi_bin.write((char *)&cadu[14], 882);

                        idk_out.write((char *)&cadu[14], 882);
                        // logger->info("MERSI!!");
                    }
                    else */
                    if (vcdu.vcid == 38) // PMR-1
                    {
                        std::vector<std::vector<uint8_t>> out = pmr1_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            pmr1_reader->work(frameVec);
                    }
                    else if (vcdu.vcid == 47) // PMR-2
                    {
                        std::vector<std::vector<uint8_t>> out = pmr2_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            pmr2_reader->work(frameVec);
                    }
                    else if (vcdu.vcid == 42) // MWRI-RM
                    {
                        std::vector<std::vector<uint8_t>> out = mwrirm_deframer.work(&cadu[14], 882);
                        for (std::vector<uint8_t> frameVec : out)
                            mwrirm_reader.work(frameVec);
                    }
                    else if (vcdu.vcid == 35) // MERSI-RM
                    {
                        mersirm_reader.work(&cadu[14], 882);
                        if (d_dump_mersi)
                            mersi_bin.write((char *)&cadu[14], 882);
                    }
                    else if (vcdu.vcid == 12) // CCSDS-Compliant VCID
                    {
#if 0
                        std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid12.work(cadu);
                        for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                        {
                            printf("APID %d\n", pkt.header.apid);
                            if (pkt.header.apid == 1)
                            {

                                uint8_t pkt_cnt = pkt.payload[35] >> 4;

                                printf("LEN %d - %d\n", pkt.payload.size(), pkt_cnt);

                                if (pkt_cnt == 4)
                                {
                                    idk_out.write((char *)pkt.header.raw, 6);
                                    idk_out.write((char *)pkt.payload.data(), 862);
                                }
                            }
                        }
#endif
                        //    if (pkt.header.apid == 16) // MWHS-2
                        //        mwhs2_reader.work(pkt, true);
                        //    else if (pkt.header.apid == 7) // MWTS-3
                        //        mwts3_reader.work(pkt);
                    }
                }
            }

            cleanup();

            if (d_dump_mersi)
                mersi_bin.close();

            int scid = satdump::most_common(fy_scids.begin(), fy_scids.end(), 0);
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
            else if (scid == FY3_F_SCID)
                sat_name = "FengYun-3F";
            else if (scid == FY3_G_SCID)
                sat_name = "FengYun-3G";

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
            else if (scid == FY3_F_SCID)
                norad = FY3_F_NORAD;
            else if (scid == FY3_G_SCID)
                norad = FY3_G_NORAD;

            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = sat_name;
            if ((d_satellite == FY_AB || d_satellite == FY_3C) && d_downlink == AHRPT)
                dataset.timestamp = satdump::get_median(virr_reader.timestamps);
            else if (d_satellite == FY_AB && d_downlink == MPT)
                dataset.timestamp = satdump::get_median(mersi1_reader.timestamps);
            else if (d_satellite == FY_3D)
                dataset.timestamp = satdump::get_median(mersi2_reader.timestamps);
            else if (d_satellite == FY_3E)
                dataset.timestamp = satdump::get_median(mersill_reader.timestamps);
            else if (d_satellite == FY_3F)
                dataset.timestamp = satdump::get_median(mersi3_reader.timestamps);
            else if (d_satellite == FY_3G)
                dataset.timestamp = satdump::get_median(mersirm_reader.timestamps);

            std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

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

                satdump::products::ImageProduct mwhs1_products;
                mwhs1_products.instrument_name = "mwhs1";

                for (int i = 0; i < 5; i++)
                    mwhs1_products.images.push_back({i, "MWHS1-" + std::to_string(i + 1), std::to_string(i + 1), mwhs1_reader.getChannel(i), 16});

                mwhs1_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_ab_mwhs1.json")), satellite_tle, mwhs1_reader.timestamps);

                mwhs1_products.save(directory);
                dataset.products_list.push_back("MWHS-1");

                mwhs1_status = DONE;
            }

            if (d_satellite == FY_3F || d_satellite == FY_3E || d_satellite == FY_3D || d_satellite == FY_3C) // MWHS-2
            {
                mwhs2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWHS-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWHS-2");
                logger->info("Lines : " + std::to_string(mwhs2_reader.lines));

                satdump::products::ImageProduct mwhs2_products;
                mwhs2_products.instrument_name = "mwhs2";

                for (int i = 0; i < 15; i++)
                    mwhs2_products.images.push_back({i, "MWHS2-" + std::to_string(i + 1), std::to_string(i + 1), mwhs2_reader.getChannel(i), 16});

                mwhs2_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_cde_mwhs2.json")), satellite_tle, mwhs2_reader.timestamps);

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

                int timestamp_offset = 0;
                if (scid == FY3_A_SCID)
                    timestamp_offset = 14923;
                else if (scid == FY3_B_SCID)
                    timestamp_offset = 14923;
                else if (scid == FY3_C_SCID)
                    timestamp_offset = 17620;

                for (int i = 0; i < (int)virr_reader.timestamps.size(); i++)
                    virr_reader.timestamps[i] += timestamp_offset * 86400.0;

                satdump::products::ImageProduct virr_products;
                virr_products.instrument_name = "virr";
                virr_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_abc_virr.json")), satellite_tle, virr_reader.timestamps);

                for (int i = 0; i < 10; i++)
                    virr_products.images.push_back({i, "VIRR-" + std::to_string(i + 1), std::to_string(i + 1), virr_reader.getChannel(i), 16});

                if (d_write_c10)
                {
                    virr_to_c10->close(satdump::get_median(virr_reader.timestamps), scid);
                    delete virr_to_c10;
                }

                if (d_downlink == AHRPT)
                    dataset.timestamp = satdump::get_median(virr_reader.timestamps); // Re-set dataset timestamp since we just adjusted it

                virr_products.save(directory);
                dataset.products_list.push_back("VIRR");

                virr_status = DONE;
            }

            if (d_satellite == FY_AB) // MERSI-1
            {
                mersi1_status = PROCESSING;
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

                satdump::products::ImageProduct mersi1_products;
                mersi1_products.instrument_name = "mersi1";

                // Channel offsets relative to Ch1
                int offset[20] = {
                    0,  8, 0,   24,  0,

                    16, 0, -16, -16, -8, 16, 24, 8, -8, 16, -16, 40, -32, 32, -24,
                };

                for (int i = 0; i < 20; i++)
                {
                    image::Image image = mersi1_reader.getChannel(i);
                    logger->debug("Processing channel %d", i + 1);
                    if (d_mersi_histmatch)
                        mersi::mersi_match_detector_histograms(image, i < 5 ? 40 : 10);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 5 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    double scale = i < 5 ? 1 : 4;
                    mersi1_products.images.push_back({i, "MERSI1-" + std::to_string(i + 1), std::to_string(i + 1), image, 12, satdump::ChannelTransform().init_affine(scale, scale, offset[i], 0)});
                }

                // virr_products.set_timestamps(mwts2_reader.timestamps);

                // mersi2_reader.getChannel(-1).save_img(directory + "/calib");

                mersi1_status = SAVING;

                mersi1_products.save(directory);
                dataset.products_list.push_back("MERSI-1");

                mersi1_status = DONE;
            }

            if (d_satellite == FY_3D) // MERSI-2
            {
                mersi2_status = PROCESSING;
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

                satdump::products::ImageProduct mersi2_products;
                mersi2_products.instrument_name = "mersi2";
                mersi2_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_d_mersi2.json")), satellite_tle, mersi2_reader.timestamps);

                // Channel offsets relative to Ch1
                int offset[25] = {
                    0,  8,  0,   24,  -8, 16,

                    16, 32, -32, -24, 32, 0,  8, -16, -8, 16, 24, 8, -8, 16, -16, 32, 32, -32, -24,
                };

                for (int i = 0; i < 25; i++)
                {
                    image::Image image = mersi2_reader.getChannel(i);
                    logger->debug("Processing channel %d", i + 1);
                    if (d_mersi_histmatch)
                        mersi::mersi_match_detector_histograms(image, (i == 4 || i == 5) ? 80 : (i < 6 ? 40 : 10));
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 6 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    double scale = i < 6 ? 1 : 4;
                    mersi2_products.images.push_back({i, "MERSI2-" + std::to_string(i + 1), std::to_string(i + 1), image, 12, satdump::ChannelTransform().init_affine(scale, scale, offset[i], 0)});
                }

                // mersi2_reader.getChannel(-1).save_img(directory + "/calib");

                mersi2_status = SAVING;

                mersi2_products.save(directory);
                dataset.products_list.push_back("MERSI-2");

                mersi2_status = DONE;
            }

            if (d_satellite == FY_3F) // MERSI-3
            {
                mersi3_status = PROCESSING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-3";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                // BowTie values
                const float alpha = 1.0 / 1.6;
                const float beta = 0.58333; // 1.0 - alpha;
                const long scanHeight_250 = 40;
                const long scanHeight_1000 = 10;

                logger->info("----------- MERSI-3");
                logger->info("Segments : " + std::to_string(mersi3_reader.segments));

                satdump::products::ImageProduct mersi3_products;
                mersi3_products.instrument_name = "mersi3";
                mersi3_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_f_mersi3.json")), satellite_tle, mersi3_reader.timestamps);

                // Channel offsets relative to Ch1
                int offset[25] = {
                    0,  8,  0,   24,  -8, 16,

                    16, 32, -32, -24, 32, 0,  8, -16, -8, 16, 24, 8, -8, 16, -16, 32, 32, -32, -24,
                };

                for (int i = 0; i < 25; i++)
                {
                    image::Image image = mersi3_reader.getChannel(i);
                    logger->debug("Processing channel %d", i + 1);
                    if (d_mersi_histmatch)
                        mersi::mersi_match_detector_histograms(image, (i == 4 || i == 5) ? 80 : (i < 6 ? 40 : 10));
                    if (i == 4 || i == 5)
                        mersi::mersi_offset_interleaved(image, 40, -2);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 6 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    double scale = i < 6 ? 1 : 4;
                    mersi3_products.images.push_back({i, "MERSI3-" + std::to_string(i + 1), std::to_string(i + 1), image, 12, satdump::ChannelTransform().init_affine(scale, scale, offset[i], 0)});
                }

                // mersi2_reader.getChannel(-1).save_img(directory + "/calib");

                mersi3_status = SAVING;

                mersi3_products.save(directory);
                dataset.products_list.push_back("MERSI-3");

                mersi3_status = DONE;
            }

            if (d_satellite == FY_3E) // MERSI-LL
            {
                mersill_status = PROCESSING;
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

                satdump::products::ImageProduct mersill_products;
                mersill_products.instrument_name = "mersill";
                mersill_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_e_mersill.json")), satellite_tle, mersill_reader.timestamps);

                // Channel offsets relative to Ch1
                int offset[18] = {
                    0, 16,

                    0, -16, 32, 8, 16, -24, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                };

                for (int i = 0; i < 18; i++)
                {
                    image::Image image = mersill_reader.getChannel(i);
                    logger->debug("Processing channel %d", i + 1);
                    if (d_mersi_histmatch)
                        mersi::mersi_match_detector_histograms(image, i < 2 ? 80 : 10);
                    if (i < 2)
                        mersi::mersi_offset_interleaved(image, 40, -3);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, i < 2 ? scanHeight_250 : scanHeight_1000, alpha, beta);
                    double scale = i < 2 ? 1 : 4;
                    mersill_products.images.push_back({i, "MERSILL-" + std::to_string(i + 1), std::to_string(i + 1), image, 12, satdump::ChannelTransform().init_affine(scale, scale, offset[i], 0)});
                }

                // mersill_reader.getChannel(-1).save_img(directory + "/calib");

                mersill_status = SAVING;

                mersill_products.save(directory);
                dataset.products_list.push_back("MERSI-LL");

                mersill_status = DONE;
            }

            if (d_satellite == FY_3G) // MERSI-RM
            {
                mersirm_status = PROCESSING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MERSI-RM";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                // BowTie values
                const float alpha = 1.0 / 1.6;
                const float beta = 0.58333; // 1.0 - alpha;
                // const long scanHeight_250 = 40;
                const long scanHeight_1000 = 10;

                logger->info("----------- MERSI-RM");
                logger->info("Segments : " + std::to_string(mersirm_reader.segments));

                satdump::products::ImageProduct mersirm_products;
                mersirm_products.instrument_name = "mersirm";
                mersirm_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_g_mersirm.json")), satellite_tle, mersirm_reader.timestamps);

                bool rotate = d_parameters.contains("satellite_rotated") ? d_parameters["satellite_rotated"].get<bool>() : false;

                // Channel offsets relative to Ch1
                int offset[8] = {
                    0, rotate ? -2 : 2, rotate ? -4 : 4, rotate ? -2 : 2, 0, rotate ? -7 : 7, rotate ? -2 : 2, rotate ? 2 : -1,
                };

                for (int i = 0; i < 8; i++)
                {
                    image::Image image = mersirm_reader.getChannel(i);
                    logger->debug("Processing channel %d", i + 1);
                    if (d_mersi_histmatch)
                        mersi::mersi_match_detector_histograms(image, 10);
                    if (d_mersi_bowtie)
                        image = image::bowtie::correctGenericBowTie(image, 1, scanHeight_1000, alpha, beta);

                    // What did you do NSMC.... Why did you turn FY-3G 180 degs!?
                    if (rotate)
                    {
                        auto img2 = image;
                        for (int l = 0; l < (int)img2.height() / 10; l++)
                        {
                            for (int x = 0; x < (int)img2.width(); x++)
                            {
                                for (int c = 0; c < 10; c++)
                                {
                                    image.set((l * 10 + c) * img2.width() + x, img2.get((l * 10 + (9 - c)) * img2.width() + x));
                                }
                            }
                        }

                        image.mirror(true, false);
                    }

                    mersirm_products.images.push_back({i, "MERSIRM-" + std::to_string(i + 1), std::to_string(i + 1), image, 12, satdump::ChannelTransform().init_affine(1, 1, offset[i], 0)});
                }

                // mersirm_reader.getChannel(-1).save_img(directory + "/calib");

                mersirm_status = SAVING;

                mersirm_products.save(directory);
                dataset.products_list.push_back("MERSI-RM");

                mersirm_status = DONE;
            }

            if (d_satellite == FY_3D) // MWRI
            {
                mwri_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWRI";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWRI");
                logger->info("Lines : " + std::to_string(mwri_reader.lines));

                satdump::products::ImageProduct mwri_products;
                mwri_products.instrument_name = "mwri";
                mwri_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_abcd_mwri1.json")), satellite_tle, mwri_reader.timestamps);

                for (int i = 0; i < 10; i++)
                    mwri_products.images.push_back({i, "MWRI-" + std::to_string(i + 1), std::to_string(i + 1), mwri_reader.getChannel(i), 16});

                mwri_products.save(directory);
                dataset.products_list.push_back("MWRI");

                mwri_status = DONE;
            }

            if (d_satellite == FY_3F) // MWRI-2
            {
                mwri2_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWRI-2";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWRI-2");
                logger->info("Lines : " + std::to_string(mwri2_reader.lines));

                satdump::products::ImageProduct mwri2_produducts;
                mwri2_produducts.instrument_name = "mwri2";
                mwri2_produducts.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_f_mwri2.json")), satellite_tle, mwri2_reader.timestamps);

                for (int i = 0; i < 26; i++)
                    mwri2_produducts.images.push_back({i, "MWRI2-" + std::to_string(i + 1), std::to_string(i + 1), mwri2_reader.getChannel(i), 16});

                mwri2_produducts.save(directory);
                dataset.products_list.push_back("MWRI-2");

                mwri2_status = DONE;
            }

            if (d_satellite == FY_3G) // MWRI-RM
            {
                mwrirm_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWRI-RM";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWRI-RM");
                logger->info("Lines : " + std::to_string(mwrirm_reader.lines));

                satdump::products::ImageProduct mwrirm_produducts;
                mwrirm_produducts.instrument_name = "mwri_rm";
                auto proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/fengyun_g_mwrirm.json"));
                if (d_parameters.contains("satellite_rotated") ? d_parameters["satellite_rotated"].get<bool>() : false)
                    proj_cfg["yaw_offset"] = proj_cfg["yaw_offset"].get<double>() - 180; // Rotate MWRI around
                mwrirm_produducts.set_proj_cfg_tle_timestamps(proj_cfg, satellite_tle, mwrirm_reader.timestamps);

                for (int i = 0; i < 26; i++)
                    mwrirm_produducts.images.push_back({i, "MWRIRM-" + std::to_string(i + 1), std::to_string(i + 1), mwrirm_reader.getChannel(i), 16});

                mwrirm_produducts.save(directory);
                dataset.products_list.push_back("MWRI-RM");

                mwrirm_status = DONE;
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

                auto img = gas_reader.getChannel();
                image::save_img(img, directory + "/GAS");

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

                satdump::products::ImageProduct erm_products;
                erm_products.instrument_name = "erm";
                erm_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_abc_erm.json")), satellite_tle, erm_reader.timestamps);

                erm_products.images.push_back({0, "ERM-1", "1", erm_reader.getChannel(), 16});

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

                satdump::products::ImageProduct mwts1_products;
                mwts1_products.instrument_name = "mwts1";

                for (int i = 0; i < 27; i++)
                    mwts1_products.images.push_back({i, "MWTS1-" + std::to_string(i + 1), std::to_string(i + 1), mwts1_reader.getChannel(i), 16});

                mwts1_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_ab_mwts1.json")), satellite_tle, mwts1_reader.timestamps);

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

                satdump::products::ImageProduct mwts2_products;
                mwts2_products.instrument_name = "mwts2";

                for (int i = 0; i < 18; i++)
                    mwts2_products.images.push_back({i, "MWTS2-" + std::to_string(i + 1), std::to_string(i + 1), mwts2_reader.getChannel(i), 16});

                mwts2_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_d_mwts2.json")), satellite_tle, mwts2_reader.timestamps);

                mwts2_products.save(directory);
                dataset.products_list.push_back("MWTS-2");

                mwts2_status = DONE;
            }

            if (d_satellite == FY_3E || d_satellite == FY_3F) // MWTS-3
            {
                mwts3_status = SAVING;
                std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWTS-3";

                if (!std::filesystem::exists(directory))
                    std::filesystem::create_directory(directory);

                logger->info("----------- MWTS-3");
                logger->info("Lines : " + std::to_string(mwhs2_reader.lines));

                satdump::products::ImageProduct mwts3_products;
                mwts3_products.instrument_name = "mwts3";
                mwts3_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/fengyun_e_mwts3.json")), satellite_tle, mwts3_reader.timestamps);

                for (int i = 0; i < 18; i++)
                    mwts3_products.images.push_back({i, "MWTS3-" + std::to_string(i + 1), std::to_string(i + 1), mwts3_reader.getChannel(i), 16});

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
                    ImGui::TextColored(style::theme.green, "%d", mersi1_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersi1_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mersi2_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersi2_status);
                }

                if (d_satellite == FY_3E)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-LL");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mersill_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersill_status);
                }

                if (d_satellite == FY_3F)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-3");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mersi3_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersi3_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWRI-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwri2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwri2_status);
                }

                if (d_satellite == FY_3G)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MERSI-RM");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mersirm_reader.segments);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mersirm_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWRI-RM");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwrirm_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwrirm_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("PMR-1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", pmr1_reader->images_count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(pmr1_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("PMR-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", pmr2_reader->images_count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(pmr2_status);
                }

                if ((d_satellite == FY_AB || d_satellite == FY_3C) && (d_downlink == AHRPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("VIRR");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", virr_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(virr_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("ERM");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", erm_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(erm_status);
                }

                if (d_satellite == FY_AB && (d_downlink == AHRPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWTS-1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwts1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwts1_status);
                }

                if (d_satellite == FY_AB && (d_downlink == AHRPT || d_downlink == DPT))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWHS-1");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwhs1_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwhs1_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWTS-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwts2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwts2_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWRI");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwri_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwri_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("WAI");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", wai_reader->images_count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(wai_status);
                }

                if (d_satellite == FY_3D)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("GAS");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", gas_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(gas_status);
                }

                if (d_satellite == FY_3E || d_satellite == FY_3F)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWTS-3");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwts3_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwts3_status);
                }

                if (d_satellite == FY_3F || d_satellite == FY_3E || d_satellite == FY_3D || d_satellite == FY_3C)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("MWHS-2");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", mwhs2_reader.lines);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(mwhs2_status);
                }

                if (d_satellite == FY_3E)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("XEUVI");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", xeuvi_reader->images_count);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(xeuvi_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("WindRAD C-Band");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", windrad_reader1->imgCount);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(windrad_C_status);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("WindRAD Ku-Band");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(style::theme.green, "%d", windrad_reader2->imgCount);
                    ImGui::TableSetColumnIndex(2);
                    drawStatus(windrad_Ku_status);
                }

                ImGui::EndTable();
            }

            drawProgressBar();

            ImGui::End();
        }

        std::string FY3InstrumentsDecoderModule::getID() { return "fy3_instruments"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> FY3InstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FY3InstrumentsDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace instruments
} // namespace fengyun3