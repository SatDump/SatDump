#include "module_wsfm_instruments.h"
#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/tracking/tle.h"
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

namespace wsfm
{
    WSFMInstrumentsDecoderModule::WSFMInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        fsfsm_enable_output = false;
    }

    void WSFMInstrumentsDecoderModule::process()
    {
        uint8_t cadu[1024];

        logger->info("Demultiplexing and deframing...");

        ccsds::ccsds_aos::Demuxer demuxer_vcid35(1010, true, 0);

        std::vector<uint8_t> wsfm_scids;

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)cadu, 1024);

            // Parse this transport frame
            ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

            // if (vcdu.spacecraft_id == METOP_A_SCID ||
            //     vcdu.spacecraft_id == METOP_B_SCID ||
            //     vcdu.spacecraft_id == METOP_C_SCID)
            wsfm_scids.push_back(vcdu.spacecraft_id);

            if (vcdu.vcid == 35) // MWI
            {
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid35.work(cadu);
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                    mwi_reader.work(pkt);
            }
        }

        cleanup();

        int scid = satdump::most_common(wsfm_scids.begin(), wsfm_scids.end(), 0);
        wsfm_scids.clear();

#define WSFM_1_SCID 120
#define WSFM_1_NORAD 59481

        std::string sat_name = "Unknown WSF-M";
        if (scid == WSFM_1_SCID)
            sat_name = "WSF-M1";

        int norad = 0;
        if (scid == WSFM_1_SCID)
            norad = WSFM_1_NORAD;

        // Products dataset
        satdump::products::DataSet dataset;
        dataset.satellite_name = sat_name;
        dataset.timestamp = satdump::get_median(mwi_reader.timestamps);

        std::optional<satdump::TLE> satellite_tle = satdump::general_tle_registry->get_from_norad_time(norad, dataset.timestamp);

        // Satellite ID
        {
            logger->info("----------- Satellite");
            logger->info("NORAD : " + std::to_string(norad));
            logger->info("Name  : " + sat_name);
        }

        // MWI
        {
            mwi_status = SAVING;
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MWI";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("----------- MWI");
            logger->info("Lines : " + std::to_string(mwi_reader.lines));

            satdump::products::ImageProduct mwi_products;
            mwi_products.instrument_name = "wsfm_mwi";
            mwi_products.set_proj_cfg_tle_timestamps(loadJsonFile(resources::getResourcePath("projections_settings/wsfm_mwi.json")), satellite_tle, mwi_reader.timestamps);

            for (int i = 0; i < 17; i++)
                mwi_products.images.push_back({i, "MWI-" + std::to_string(i + 1), std::to_string(i + 1), mwi_reader.getChannel(i), 16});

            mwi_products.save(directory);
            dataset.products_list.push_back("MWI");

            mwi_status = DONE;
        }

        dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
    }

    void WSFMInstrumentsDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("WSF-M Instruments Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (ImGui::BeginTable("##wsfminstrumentstable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
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
            ImGui::Text("MWI");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(style::theme.green, "%d", mwi_reader.lines);
            ImGui::TableSetColumnIndex(2);
            drawStatus(mwi_status);

            ImGui::EndTable();
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string WSFMInstrumentsDecoderModule::getID() { return "wsfm_instruments"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> WSFMInstrumentsDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<WSFMInstrumentsDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace wsfm