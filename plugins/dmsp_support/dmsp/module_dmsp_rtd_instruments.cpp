#include "module_dmsp_rtd_instruments.h"
#include "common/repack.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "core/resources.h"
#include "dmsp/instruments/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"
#include "products/image_product.h"
#include <cstdint>
#include <exception>
#include <filesystem>
#include <volk/volk.h>

namespace dmsp
{
    DMSPInstrumentsModule::DMSPInstrumentsModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    DMSPInstrumentsModule::~DMSPInstrumentsModule() {}

    void DMSPInstrumentsModule::process()
    {
        int sat_num = 0;

        try
        {
            sat_num = d_parameters["satellite_number"];
        }
        catch (std::exception &)
        {
            sat_num = std::stoi(d_parameters["satellite_number"].get<std::string>());
        }

        // So the syncword on SSMIS seems to change between satellites? Sure...
        if (sat_num == 17)
        {
            ssmis_deframer = std::make_shared<def::SimpleDeframer>(0x2fffffffff, 40, 27000, 0);
            ols_reader.set_offsets(9, -8);
        }
        else if (sat_num == 18)
        {
            ssmis_deframer = std::make_shared<def::SimpleDeframer>(0x7ffffffff, 40, 27000, 0);
            ols_reader.set_offsets(0, 14);
        }

        std::string fs_config = d_parameters["tag_bit_override"].get<std::string>();

        if (fs_config == "VIS/IR")
        {
            ols_reader.set_tag_bit(1);
        }
        else if (fs_config == "IR/VIS")
        {
            ols_reader.set_tag_bit(2);
        }
        else
        {
            ols_reader.set_tag_bit(0);
        }

        uint8_t rtd_words[19];

        // Timestamps
        std::ifstream timestamps_in;
        bool has_timestamps = false;
        if (has_timestamps = std::filesystem::exists(d_output_file_hint + "_timestamps.txt"))
            timestamps_in = std::ifstream(d_output_file_hint + "_timestamps.txt", std::ios::binary);
        double last_timestamp = -1;

        std::ofstream data_ou = std::ofstream("/tmp/test.bin");

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)rtd_frame, 19);

            if (has_timestamps)
            {
                try
                {
                    std::string tt;
                    std::getline(timestamps_in, tt);
                    last_timestamp = std::stod(tt);
                }
                catch (std::exception &)
                {
                }
            }

            shift_array_left(&rtd_frame[0], 19, 6, rtd_words);

            // OLS
            ols_reader.work(rtd_frame, rtd_words);

            // The rest
            rtd_frame[18] <<= 2;
            RTDFrame parsed_frm(rtd_frame);

            auto dats = terdats.work(parsed_frm);

            for (auto &d : dats)
            {
                if (d.first == SENSORID_SSMIS)
                {
                    data_ou.write((char *)d.second.data(), d.second.size());

                    auto fs = ssmis_deframer->work(d.second.data(), d.second.size());
                    for (auto &f : fs)
                    {
                        logger->critical("%d %d", d.first, fs.size());
                        //  data_ou.write((char *)f.data(), 3375);
                        ssmis_reader.work(f.data(), last_timestamp);
                    }
                }
            }
        }

        cleanup();

        std::string sat_name = "Unknown DMSP-F1x";
        if (sat_num == 16)
            sat_name = "DMSP F-16";
        else if (sat_num == 17)
            sat_name = "DMSP F-17";
        else if (sat_num == 18)
            sat_name = "DMSP F-18";

        int norad = 0;
        if (sat_num == 16)
            norad = 28054;
        else if (sat_num == 17)
            norad = 29522;
        else if (sat_num == 18)
            norad = 35951;

        // Products dataset
        satdump::products::DataSet dataset;
        dataset.satellite_name = "DMSP-F1x";
        dataset.timestamp = last_timestamp == -1 ? time(0) : last_timestamp;

        std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

        // OLS
        {
            ols_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/OLS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- OLS");
            logger->info("Lines : " + std::to_string(ols_reader.lines));

            satdump::products::ImageProduct ols_products;
            ols_products.instrument_name = "ols";

            ols_products.images.push_back({0, "OLS-VIS", "vis", ols_reader.getChannelVIS(), 8});
            ols_products.images.push_back({1, "OLS-IR", "ir", ols_reader.getChannelIR(), 8});

            ols_products.save(directory);
            dataset.products_list.push_back("OLS");

            ols_status = DONE;
        }

        // SSMIS
        {
            ssmis_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SSMIS";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- SSMIS");
            logger->info("Lines : " + std::to_string(ssmis_reader.lines));

            satdump::products::ImageProduct ssmis_products;
            ssmis_products.instrument_name = "ssmis";
            ssmis_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/dmsp_ssmis.json")), satellite_tle, ssmis_reader.timestamps);

            for (int i = 0; i < 24; i++)
            {
                satdump::image::Image img = ssmis_reader.getChannel(i);

                int scale = 1;
                if (i < 6)
                    scale = 1;
                else if (i < 11)
                    scale = 2;
                else
                    scale = 2; // TODOREWORK?

                ssmis_products.images.push_back({0, "SSMIS-" + std::to_string(i + 1), std::to_string(i + 1), img, 16, satdump::ChannelTransform().init_affine(scale, 1, 0, 0)});
            }

            ssmis_products.save(directory);
            dataset.products_list.push_back("SSMIS");

            ssmis_status = DONE;
        }

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
    }

    void DMSPInstrumentsModule::drawUI(bool window)
    {
        ImGui::Begin("DMSP RTD Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTable("##dmsprtdinstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
            ImGui::Text("OLS");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", ols_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(ols_status);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("SSMIS");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", ssmis_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(ssmis_status);

            ImGui::EndTable();
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string DMSPInstrumentsModule::getID() { return "dmsp_rtd_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> DMSPInstrumentsModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<DMSPInstrumentsModule>(input_file, output_file_hint, parameters);
    }
} // namespace dmsp
