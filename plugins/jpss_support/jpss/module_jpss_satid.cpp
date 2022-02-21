#include "module_jpss_satid.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"
#include "jpss.h"
#include "nlohmann/json_utils.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace jpss
{
    namespace satid
    {
        JPSSSatIDModule::JPSSSatIDModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                            scid_hint(parameters.count("scid_hint") > 0 ? parameters["scid_hint"].get<int>() : -1)
        {
        }

        void JPSSSatIDModule::process()
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

                if (vcdu.spacecraft_id == SNPP_SCID ||
                    vcdu.spacecraft_id == JPSS1_SCID)
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

            std::string sat_name = "Unknown JPSS";

            if (scid == SNPP_SCID)
                sat_name = "Suomi NPP";
            else if (scid == JPSS1_SCID)
                sat_name = "NOAA 20 (JPSS-1)";

            int norad = 0;

            if (scid == SNPP_SCID)
                norad = SNPP_NORAD;
            else if (scid == JPSS1_SCID)
                norad = JPSS1_NORAD;

            logger->info("This is satellite is " + sat_name + ", NORAD " + std::to_string(norad));

            nlohmann::json jData;

            jData["scid"] = scid;
            jData["name"] = sat_name;
            jData["norad"] = norad;

            saveJsonFile(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/sat_info.json", jData);
        }

        void JPSSSatIDModule::drawUI(bool window)
        {
            ImGui::Begin("JPSS Satellite Identifier", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string JPSSSatIDModule::getID()
        {
            return "jpss_satid";
        }

        std::vector<std::string> JPSSSatIDModule::getParameters()
        {
            return {"scid_hint"};
        }

        std::shared_ptr<ProcessingModule> JPSSSatIDModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<JPSSSatIDModule>(input_file, output_file_hint, parameters);
        }
    } // namespace amsu
} // namespace metop