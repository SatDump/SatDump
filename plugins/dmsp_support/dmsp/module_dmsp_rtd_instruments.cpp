#include "module_dmsp_rtd_instruments.h"
#include "common/repack.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "products2/dataset.h"
#include "products2/image_product.h"
#include <cstdint>
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

        if (sat_num == 17)
            ols_reader.set_offsets(9, -8);
        else if (sat_num == 18)
            ols_reader.set_offsets(0, 14);

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

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)rtd_frame, 19);

            shift_array_left(&rtd_frame[0], 19, 6, rtd_words);

            ols_reader.work(rtd_frame, rtd_words);
        }

        cleanup();

        // Products dataset
        satdump::products::DataSet dataset;
        dataset.satellite_name = "DMSP-F1x";
        dataset.timestamp = time(0);

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
