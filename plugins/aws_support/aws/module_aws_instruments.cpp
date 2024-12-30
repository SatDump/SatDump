#include "module_aws_instruments.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "products2/image_product.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "products/dataset.h"
#include "resources.h"
#include "nlohmann/json_utils.h"

#include "common/tracking/tle.h"

#define AWS_SCID 104
#define AWS_NORAD 60543

namespace aws
{
    AWSInstrumentsDecoderModule::AWSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    void AWSInstrumentsDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        logger->info("Using input frames " + d_input_file);

        time_t lastTime = 0;
        uint8_t cadu[1279];

        logger->info("Demultiplexing and deframing...");

        ccsds::ccsds_tm::Demuxer demuxer_vcid2(1109, false);
        ccsds::ccsds_tm::Demuxer demuxer_vcid3(1109, false);

        std::vector<uint8_t> aws_scids;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)cadu, 1279);

            // Parse this transport frame
            ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

            if (vcdu.spacecraft_id == AWS_SCID)
                aws_scids.push_back(vcdu.spacecraft_id);

            if (vcdu.vcid == 2) // Dump
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 100)
                        mws_dump_reader.work(pkt);
                    else if (pkt.header.apid == 51)
                        navatt_reader.work(pkt);
            }
            else if (vcdu.vcid == 3) //  DB
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 100)
                        mws_reader.work(pkt);
                    else if (pkt.header.apid == 51)
                        navatt_reader.work(pkt);
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        data_in.close();

        int scid = most_common(aws_scids.begin(), aws_scids.end(), 0);
        aws_scids.clear();

        std::string sat_name = "Unknown AWS";
        if (scid == AWS_SCID)
            sat_name = "AWS";

        int norad = 0;
        if (scid == AWS_SCID)
            norad = AWS_NORAD;

        // Products dataset
        satdump::ProductDataSet dataset;
        dataset.satellite_name = sat_name;
        dataset.timestamp = get_median(mws_reader.timestamps);

        std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry.get_from_norad_time(norad, dataset.timestamp);

        // Satellite ID
        {
            logger->info("----------- Satellite");
            logger->info("NORAD : " + std::to_string(norad));
            logger->info("Name  : " + sat_name);
        }

#if 0
        // Sterna DB
        {
            mws_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/STERNA";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- STERNA");
            logger->info("Lines : " + std::to_string(mws_reader.lines));

            satdump::ImageProducts mws_products;
            mws_products.instrument_name = "sterna";
            mws_products.has_timestamps = true;
            mws_products.set_tle(satellite_tle);
            mws_products.bit_depth = 16;
            mws_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            mws_products.set_timestamps(mws_reader.timestamps);

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_sterna.json"));
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            mws_products.set_proj_cfg(proj_cfg);

            for (int i = 0; i < 19; i++)
                mws_products.images.push_back({"STERNA-" + std::to_string(i + 1), std::to_string(i + 1), mws_reader.getChannel(i)});

            mws_products.save(directory);
            dataset.products_list.push_back("STERNA");

            mws_status = DONE;
        }

        // Sterna Dump
        {
            mws_dump_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/mws_Dump";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- STERNA Dump");
            logger->info("Lines : " + std::to_string(mws_dump_reader.lines));

            satdump::ImageProducts mws_dump_products;
            mws_dump_products.instrument_name = "sterna";
            mws_dump_products.has_timestamps = true;
            mws_dump_products.set_tle(satellite_tle);
            mws_dump_products.bit_depth = 16;
            mws_dump_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            mws_dump_products.set_timestamps(mws_dump_reader.timestamps);

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_sterna.json"));
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            mws_dump_products.set_proj_cfg(proj_cfg);

            for (int i = 0; i < 19; i++)
                mws_dump_products.images.push_back({"STERNA-" + std::to_string(i + 1), std::to_string(i + 1), mws_dump_reader.getChannel(i)});

            mws_dump_products.save(directory);
            dataset.products_list.push_back("mws_Dump");

            mws_dump_status = DONE;
        }
#else
        // Sterna Dump
        {
            mws_dump_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWS_Dump";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- MWS Dump");
            logger->info("Lines : " + std::to_string(mws_dump_reader.lines));

            satdump::products::ImageProduct mws_dump_product;
            mws_dump_product.instrument_name = "aws_mws";
            //  mws_dump_products.has_timestamps = true;
            //  mws_dump_products.set_tle(satellite_tle);
            //  mws_dump_products.bit_depth = 16;
            //  mws_dump_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            //  mws_dump_products.set_timestamps(mws_dump_reader.timestamps);

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_sterna.json")); // TODOREWORK RENAME
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            proj_cfg["timestamps"] = mws_dump_reader.timestamps;
            mws_dump_product.set_proj_cfg(proj_cfg);

            // Channels are aligned by groups. 1 to 8 / 9 / 10 to 15 / 16 to 19
            satdump::ChannelTransform tran[19];
            double halfscan = 145.0 / 2.0;
            for (auto &v : tran)
                v.init_none();
            tran[8].init_affine_slantx(1, 1, -6.5, 0, halfscan, 11.0 / halfscan);
            tran[10] = tran[11] = tran[12] = tran[13] = tran[14] = tran[9].init_affine_slantx(0.92, 1, -0.5, 5, halfscan, 11.0 / halfscan);
            tran[15] = tran[16] = tran[17] = tran[18].init_affine_slantx(0.93, 1, 2.8, 4, halfscan, 6.0 / halfscan);

            // TODOREWORK
            double mws_freqs[19] = {
                50.3e9,
                52.8e9,
                53.246e9,
                53.596e9,
                54.4e9,
                54.94e9,
                55.5e9,
                57.290344e9,
                89e9,
                165.5e9,
                176.311e9,
                178.811e9,
                180.311e9,
                181.511e9,
                182.311e9,
                325.15e9,
                325.15e9,
                325.15e9,
                325.15e9,
            };

            for (int i = 0; i < 19; i++)
            {
                mws_dump_product.images.push_back({i,
                                                   "STERNA-" + std::to_string(i + 1),
                                                   std::to_string(i + 1),
                                                   mws_dump_reader.getChannel(i),
                                                   16,
                                                   tran[i]});
                mws_dump_product.set_channel_frequency(i, mws_freqs[i]);
            }

            mws_dump_product.save(directory);
            dataset.products_list.push_back("MWS_Dump");

            mws_dump_status = DONE;
        }
#endif

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
    }

    void AWSInstrumentsDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("AWS Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTable("##sawsinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Instrument");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Lines / Frames");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Status");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("MWS");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", mws_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(mws_status);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("MWS Dump");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", mws_dump_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(mws_dump_status);

            ImGui::EndTable();
        }

        ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string AWSInstrumentsDecoderModule::getID()
    {
        return "aws_instruments";
    }

    std::vector<std::string> AWSInstrumentsDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> AWSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<AWSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
}