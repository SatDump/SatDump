#include "module_aws_instruments.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/utils.h"
#include "core/resources.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include "utils/stats.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "common/tracking/tle.h"

#define AWS_SCID 104
#define AWS_NORAD 60543

namespace aws
{
    AWSInstrumentsDecoderModule::AWSInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
    }

    void AWSInstrumentsDecoderModule::process()
    {
        uint8_t cadu[1279];

        logger->info("Demultiplexing and deframing...");

        ccsds::ccsds_tm::Demuxer demuxer_vcid2(1109, false);
        ccsds::ccsds_tm::Demuxer demuxer_vcid3(1109, false);

        std::vector<uint8_t> aws_scids;

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)cadu, 1279);

            // Parse this transport frame
            ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

            if (vcdu.spacecraft_id == AWS_SCID)
                aws_scids.push_back(vcdu.spacecraft_id);

            if (vcdu.vcid == 2) // Dump
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 100)
                        mwr_dump_reader.work(pkt);
                    else if (pkt.header.apid == 51)
                        navatt_reader.work(pkt);
            }
            else if (vcdu.vcid == 3) //  DB
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 100)
                        mwr_reader.work(pkt);
                    else if (pkt.header.apid == 51)
                        navatt_reader.work(pkt);
            }
        }

        cleanup();

        int scid = satdump::most_common(aws_scids.begin(), aws_scids.end(), 0);
        aws_scids.clear();

        std::string sat_name = "Unknown AWS";
        if (scid == AWS_SCID)
            sat_name = "AWS";

        int norad = 0;
        if (scid == AWS_SCID)
            norad = AWS_NORAD;

        // Products dataset
        satdump::products::DataSet dataset;
        dataset.satellite_name = sat_name;
        dataset.timestamp = satdump::get_median(mwr_reader.timestamps);
        if (dataset.timestamp == -1)
            dataset.timestamp = satdump::get_median(mwr_dump_reader.timestamps);

        std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

        // Satellite ID
        {
            logger->info("----------- Satellite");
            logger->info("NORAD : " + std::to_string(norad));
            logger->info("Name  : " + sat_name);
        }

        // TODOREWORK
        double mwr_freqs[19] = {
            50.3e9, 52.8e9, 53.246e9, 53.596e9, 54.4e9, 54.94e9, 55.5e9, 57.290344e9, 89e9, 165.5e9, 176.311e9, 178.811e9, 180.311e9, 181.511e9, 182.311e9, 325.15e9, 325.15e9, 325.15e9, 325.15e9,
        };

        // Channels are aligned by groups. 1 to 8 / 9 / 10 to 15 / 16 to 19
        satdump::ChannelTransform tran[19];
        double halfscan = 145.0 / 2.0;
        for (auto &v : tran)
            v.init_none();
        tran[8].init_affine_slantx(1, 1, -6.5, 0, halfscan, 11.0 / halfscan);
        tran[10] = tran[11] = tran[12] = tran[13] = tran[14] = tran[9].init_affine_slantx(0.92, 1, -0.5, 5, halfscan, 11.0 / halfscan);
        tran[15] = tran[16] = tran[17] = tran[18].init_affine_slantx(0.93, 1, 2.8, 4, halfscan, 6.0 / halfscan);

        // Sterna DB
        {
            mwr_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWR";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- MWR");
            logger->info("Lines : " + std::to_string(mwr_reader.lines));

            satdump::products::ImageProduct mwr_products;
            mwr_products.instrument_name = "aws_mwr";

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_mwr.json"));
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            mwr_products.set_proj_cfg_tle_timestamps(proj_cfg, satellite_tle, mwr_reader.timestamps);

            for (int i = 0; i < 19; i++)
            {
                mwr_products.images.push_back({i, "MWR-" + std::to_string(i + 1), std::to_string(i + 1), mwr_reader.getChannel(i), 16, tran[i]});
                mwr_products.set_channel_frequency(i, mwr_freqs[i]);
            }

            mwr_products.save(directory);
            dataset.products_list.push_back("MWR");

            mwr_status = DONE;
        }

        // Sterna Dump
        {
            mwr_dump_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWR_Dump";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- MWR Dump");
            logger->info("Lines : " + std::to_string(mwr_dump_reader.lines));

            satdump::products::ImageProduct mwr_dump_product;
            mwr_dump_product.instrument_name = "aws_mwr";

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_mwr.json"));
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            mwr_dump_product.set_proj_cfg_tle_timestamps(proj_cfg, satellite_tle, mwr_dump_reader.timestamps);

            for (int i = 0; i < 19; i++)
            {
                mwr_dump_product.images.push_back({i, "MWR-" + std::to_string(i + 1), std::to_string(i + 1), mwr_dump_reader.getChannel(i), 16, tran[i]});
                mwr_dump_product.set_channel_frequency(i, mwr_freqs[i]);
            }

            mwr_dump_product.save(directory);
            dataset.products_list.push_back("MWR_Dump");

            mwr_dump_status = DONE;
        }

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
            ImGui::Text("MWR");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", mwr_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(mwr_status);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("MWR Dump");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", mwr_dump_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(mwr_dump_status);

            ImGui::EndTable();
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string AWSInstrumentsDecoderModule::getID() { return "aws_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> AWSInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<AWSInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aws