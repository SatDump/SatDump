#include "module_metop_satid.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "metop.h"
#include "nlohmann/json_utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    namespace satid
    {
        MetOpSatIDModule::MetOpSatIDModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void MetOpSatIDModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            logger->info("Scanning through CADUs...");

            std::vector<int> scids;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                if (vcdu.spacecraft_id == METOP_A_SCID ||
                    vcdu.spacecraft_id == METOP_B_SCID ||
                    vcdu.spacecraft_id == METOP_C_SCID)
                    scids.push_back(vcdu.spacecraft_id);

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            int scid = most_common(scids.begin(), scids.end());

            logger->info("SCID " + std::to_string(scid) + " was most common.");

            std::string sat_name = "Unknown MetOp";

            if (scid == METOP_A_SCID)
                sat_name = "MetOp-A";
            else if (scid == METOP_B_SCID)
                sat_name = "MetOp-B";
            else if (scid == METOP_C_SCID)
                sat_name = "MetOp-C";

            int norad = 0;

            if (scid == METOP_A_SCID)
                norad = METOP_A_NORAD;
            else if (scid == METOP_B_SCID)
                norad = METOP_B_NORAD;
            else if (scid == METOP_C_SCID)
                norad = METOP_C_NORAD;

            logger->info("This is satellite is " + sat_name + ", NORAD " + std::to_string(norad));

            nlohmann::json jData;

            jData["scid"] = scid;
            jData["name"] = sat_name;
            jData["norad"] = norad;

            saveJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json", jData);
        }

        void MetOpSatIDModule::drawUI(bool window)
        {
            ImGui::Begin("MetOp Satellite Identifier", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string MetOpSatIDModule::getID()
        {
            return "metop_satid";
        }

        std::vector<std::string> MetOpSatIDModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> MetOpSatIDModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<MetOpSatIDModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop