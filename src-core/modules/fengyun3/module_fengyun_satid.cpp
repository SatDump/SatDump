#include "module_fengyun_satid.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "fengyun3.h"
#include "nlohmann/json_utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun3
{
    namespace satid
    {
        FengYunSatIDModule::FengYunSatIDModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  scid_hint(parameters.count("scid_hint") > 0 ? parameters["scid_hint"].get<int>() : -1)
        {
        }

        void FengYunSatIDModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;

            uint8_t cadu[1024];

            logger->info("Scanning through CADUs...");

            std::vector<uint8_t> scids;

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

            if (scid_hint > 0)
            {
                if (scid == scid_hint)
                {
                    logger->debug("That matches with the provided SCID.");
                }
                else
                {
                    logger->error("Detected SCID did not match expectations! Defaulting to provided value...");
                    scid = scid_hint;
                }
            }

            std::string sat_name = "Unknown FengYun";

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

            logger->info("This is satellite is " + sat_name + ", NORAD " + std::to_string(norad));

            nlohmann::json jData;

            jData["scid"] = scid;
            jData["name"] = sat_name;
            jData["norad"] = norad;

            saveJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json", jData);
        }

        void FengYunSatIDModule::drawUI(bool window)
        {
            ImGui::Begin("FengYun Satellite Identifier", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string FengYunSatIDModule::getID()
        {
            return "fengyun_satid";
        }

        std::vector<std::string> FengYunSatIDModule::getParameters()
        {
            return {"scid_hint"};
        }

        std::shared_ptr<ProcessingModule> FengYunSatIDModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<FengYunSatIDModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop