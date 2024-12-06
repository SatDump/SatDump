#include "module_aws_instruments.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "products/image_products.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "products/dataset.h"
#include "resources.h"
#include "nlohmann/json_utils.h"

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
                        sterna_dump_reader.work(pkt);
                    else if (pkt.header.apid == 51)
                        navatt_reader.work(pkt);
            }
            else if (vcdu.vcid == 3) //  DB
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid3.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    if (pkt.header.apid == 100)
                        sterna_reader.work(pkt);
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
        dataset.timestamp = get_median(sterna_reader.timestamps);

        std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry.get_from_norad_time(norad, dataset.timestamp);

        // Satellite ID
        {
            logger->info("----------- Satellite");
            logger->info("NORAD : " + std::to_string(norad));
            logger->info("Name  : " + sat_name);
        }

        // Sterna DB
        {
            sterna_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/STERNA";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- STERNA");
            logger->info("Lines : " + std::to_string(sterna_reader.lines));

            satdump::ImageProducts sterna_products;
            sterna_products.instrument_name = "sterna";
            sterna_products.has_timestamps = true;
            sterna_products.set_tle(satellite_tle);
            sterna_products.bit_depth = 16;
            sterna_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            sterna_products.set_timestamps(sterna_reader.timestamps);

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_sterna.json"));
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            sterna_products.set_proj_cfg(proj_cfg);

            for (int i = 0; i < 19; i++)
                sterna_products.images.push_back({"STERNA-" + std::to_string(i + 1), std::to_string(i + 1), sterna_reader.getChannel(i)});

            sterna_products.save(directory);
            dataset.products_list.push_back("STERNA");

            sterna_status = DONE;
        }

        // Sterna Dump
        {
            sterna_dump_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/STERNA_Dump";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- STERNA Dump");
            logger->info("Lines : " + std::to_string(sterna_dump_reader.lines));

            satdump::ImageProducts sterna_dump_products;
            sterna_dump_products.instrument_name = "sterna";
            sterna_dump_products.has_timestamps = true;
            sterna_dump_products.set_tle(satellite_tle);
            sterna_dump_products.bit_depth = 16;
            sterna_dump_products.timestamp_type = satdump::ImageProducts::TIMESTAMP_LINE;
            sterna_dump_products.set_timestamps(sterna_dump_reader.timestamps);

            nlohmann::json proj_cfg = loadJsonFile(resources::getResourcePath("projections_settings/aws_sterna.json"));
            if (d_parameters["use_ephemeris"].get<bool>())
                proj_cfg["ephemeris"] = navatt_reader.getEphem();
            sterna_dump_products.set_proj_cfg(proj_cfg);

            for (int i = 0; i < 19; i++)
                sterna_dump_products.images.push_back({"STERNA-" + std::to_string(i + 1), std::to_string(i + 1), sterna_dump_reader.getChannel(i)});

            sterna_dump_products.save(directory);
            dataset.products_list.push_back("STERNA_Dump");

            sterna_dump_status = DONE;
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
            ImGui::Text("Sterna");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", sterna_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(sterna_status);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Sterna Dump");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", sterna_dump_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(sterna_dump_status);

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