#include "module_ccsds_analyzer.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_1024/demuxer.h"
#include "common/ccsds/ccsds_1_0_1024/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

/*
This is a generic "CCSDS Analyzer", meant to
quickly get an insight of what a downlink contains
for reverse-engineering etc.
*/

namespace ccsds
{
    namespace analyzer
    {
        CCSDSAnalyzerModule::CCSDSAnalyzerModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void CCSDSAnalyzerModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            logger->info("Using input frames " + d_input_file);

            time_t lastTime = 0;

            ccsds::ccsds_1_0_1024::Demuxer ccsdsDemuxer(882, true);
            uint64_t argos_cadu = 0, ccsds = 0, argos_ccsds = 0;
            uint8_t cadu[1024];

            logger->info("Demultiplexing and deframing...");
            logger->warn("This currently only supports 8192-bits CADU!");

            // We need 1 demux per VCID
            std::map<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>> demuxers;

            nlohmann::json analysis;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)&cadu, 1024);

                // Parse this transport frame
                ccsds::ccsds_1_0_1024::VCDU vcdu = ccsds::ccsds_1_0_1024::parseVCDU(cadu);

                std::string vcstring = "VCID " + std::to_string(vcdu.vcid);

                // New VCID?
                if (demuxers.count(vcdu.vcid) <= 0)
                {
                    logger->info("New VCID found! VC " + std::to_string(vcdu.vcid));
                    demuxers.emplace(std::pair<int, std::shared_ptr<ccsds::ccsds_1_0_1024::Demuxer>>(vcdu.vcid, std::make_shared<ccsds::ccsds_1_0_1024::Demuxer>(882, true)));

                    analysis[vcstring]["count"] = 0;
                }

                // Update analysis
                analysis[vcstring]["count"] = analysis[vcstring]["count"].get<int>() + 1;

                // Demux this VCID
                std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxers[vcdu.vcid]->work(cadu);

                // Process
                for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
                {
                    // APIDs
                    std::string apstring = "APID " + std::to_string(pkt.header.apid);

                    if (analysis[vcstring].count("apids") <= 0)
                        analysis[vcstring]["apids"] = {};

                    if (analysis[vcstring]["apids"].count(apstring) <= 0)
                    {
                        logger->info("New APID found! VCID " + std::to_string(vcdu.vcid) + " APID " + std::to_string(pkt.header.apid));
                        analysis[vcstring]["apids"][apstring]["count"] = 0;
                    }

                    analysis[vcstring]["apids"][apstring]["count"] = analysis[vcstring]["apids"][apstring]["count"].get<int>() + 1;

                    // Sizes
                    nlohmann::json &apids_json = analysis[vcstring]["apids"][apstring];

                    if (apids_json.count("sizes") <= 0)
                        apids_json["sizes"] = {};

                    std::string pkt_size = std::to_string(pkt.payload.size() + 6);

                    if (apids_json["sizes"].count(pkt_size) <= 0)
                    {
                        apids_json["sizes"][pkt_size] = 0;
                    }

                    apids_json["sizes"][pkt_size] = apids_json["sizes"][pkt_size].get<int>() + 1;
                }

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            std::ofstream file_out = std::ofstream(d_output_file_hint + "_ccsds_analysis.json", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + "_ccsds_analysis.json");
            file_out << analysis.dump(4);
            file_out.close();

            data_in.close();
        }

        void CCSDSAnalyzerModule::drawUI(bool window)
        {
            ImGui::Begin("CCSDS Downlink Analyzer", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string CCSDSAnalyzerModule::getID()
        {
            return "ccsds_analyzer";
        }

        std::vector<std::string> CCSDSAnalyzerModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> CCSDSAnalyzerModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<CCSDSAnalyzerModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop