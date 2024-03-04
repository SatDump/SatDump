#include "module_s2_ts_extractor.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "common/utils.h"

#if !(defined(__APPLE__) || defined(_WIN32))
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#endif

#include "codings/dvb-s2/bbframe_bch.h"
#include "codings/dvb-s2/modcod_to_cfg.h"

namespace dvbs2
{
    S2TStoTCPModule::S2TStoTCPModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        if (d_parameters.contains("bb_size"))
        {
            bbframe_size = d_parameters["bb_size"].get<int>();
        }
        else
        {
            int d_modcod = -1;
            bool d_shortframes = false;
            bool d_pilots = false;

            if (parameters.count("modcod") > 0)
                d_modcod = parameters["modcod"].get<int>();
            else
                throw std::runtime_error("MODCOD parameter must be present!");

            if (parameters.count("shortframes") > 0)
                d_shortframes = parameters["shortframes"].get<bool>();

            if (parameters.count("pilots") > 0)
                d_pilots = parameters["pilots"].get<bool>();

            // Parse modcod number
            auto cfg = dvbs2::get_dvbs2_cfg(d_modcod, d_shortframes, d_pilots);

            dvbs2::BBFrameBCH bch_decoder(cfg.framesize, cfg.coderate);
            bbframe_size = bch_decoder.dataSize();
        }
    }

    std::vector<ModuleDataType> S2TStoTCPModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> S2TStoTCPModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    S2TStoTCPModule::~S2TStoTCPModule()
    {
    }

    void S2TStoTCPModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        logger->info("Using input bbframes " + d_input_file);

        time_t lastTime = 0;

        uint8_t *bb_buffer = new uint8_t[bbframe_size];
        uint8_t ts_frames[188 * 1000];

        dvbs2::BBFrameTSParser ts_extractor(bbframe_size);

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)bb_buffer, bbframe_size / 8);
            else
                input_fifo->read((uint8_t *)bb_buffer, bbframe_size / 8);

            int ts_cnt = ts_extractor.work(bb_buffer, 1, ts_frames, 188 * 1000);

            for (int i = 0; i < ts_cnt; i++)
            {
                if (output_data_type == DATA_FILE)
                    data_out.write((char *)&ts_frames[i * 188], 188);
                else
                    output_fifo->write((uint8_t *)&ts_frames[i * 188], 188);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        delete[] bb_buffer;

        data_out.close();
        if (output_data_type == DATA_FILE)
            data_in.close();
    }

    void S2TStoTCPModule::drawUI(bool window)
    {
        ImGui::Begin("DVB-S2 TS Extractor", NULL, window ? 0 : NOWINDOW_FLAGS);
        ImGui::BeginGroup();
        {
            // ImGui::Button("TS Status", {200 * ui_scale, 20 * ui_scale});
            //{
            //     ImGui::Text("PID  : ");
            //     ImGui::SameLine();
            //     ImGui::TextColored(pid_to_decode == current_pid ? style::theme.green : style::theme.orange, UITO_C_STR(current_pid));
            // }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string S2TStoTCPModule::getID()
    {
        return "dvbs2_ts_extractor";
    }

    std::vector<std::string> S2TStoTCPModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> S2TStoTCPModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<S2TStoTCPModule>(input_file, output_file_hint, parameters);
    }
}