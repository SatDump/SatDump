#include "module_s2udp_xrit_cadu_extractor.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "common/mpeg_ts/ts_header.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/utils.h"

namespace xrit
{
    S2UDPxRITCADUextractor::S2UDPxRITCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        pid_to_decode = d_parameters["pid"].get<int>();
        bbframe_size = d_parameters["bb_size"].get<int>();
    }

    std::vector<ModuleDataType> S2UDPxRITCADUextractor::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> S2UDPxRITCADUextractor::getOutputTypes()
    {
        return {DATA_FILE};
    }

    S2UDPxRITCADUextractor::~S2UDPxRITCADUextractor()
    {
    }

    void S2UDPxRITCADUextractor::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".cadu");
        }

        logger->info("Using input bbframes " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        bool ts_input = d_parameters.contains("ts_input") ? d_parameters["ts_input"].get<bool>() : false;

        uint8_t *bb_buffer = new uint8_t[bbframe_size];
        uint8_t ts_frames[188 * 100];

        dvbs2::BBFrameTSParser ts_extractor(bbframe_size);

        // TS Stuff
        int ts_cnt = 0;
        mpeg_ts::TSHeader ts_header;
        mpeg_ts::TSDemux ts_demux;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            if (ts_input)
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)ts_frames, 188);
                else
                    input_fifo->read((uint8_t *)ts_frames, 188);
                ts_cnt = 1;
            }
            else
            {
                // Read buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)bb_buffer, bbframe_size / 8);
                else
                    input_fifo->read((uint8_t *)bb_buffer, bbframe_size / 8);
                ts_cnt = ts_extractor.work(bb_buffer, 1, ts_frames, 188 * 100);
            }

            for (int i = 0; i < ts_cnt; i++)
            {
                uint8_t *ts_frame = &ts_frames[i * 188];

                ts_header.parse(ts_frame);
                current_pid = ts_header.pid;

                std::vector<std::vector<uint8_t>> frames = ts_demux.demux(ts_frame, pid_to_decode); // Extract PID

                for (std::vector<uint8_t> payload : frames)
                {
                    if (payload[40] == 0x1a && payload[41] == 0xcf && payload[42] == 0xfc && payload[43] == 0x1d && payload.size() >= 1024) // Check this is a CADU and not other IP data
                    {
                        if (output_data_type == DATA_FILE)
                            data_out.write((char *)&payload[40], 1024);
                        else
                            output_fifo->write((uint8_t *)&payload[40], 1024);
                    }
                }
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

        if (output_data_type == DATA_FILE)
            data_out.close();
        if (output_data_type == DATA_FILE)
            data_in.close();
    }

    void S2UDPxRITCADUextractor::drawUI(bool window)
    {
        ImGui::Begin("DVB-S2 UDP xRIT CADU Extractor", NULL, window ? 0 : NOWINDOW_FLAGS);
        ImGui::BeginGroup();
        {
            ImGui::Button("TS Status", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("PID  : ");
                ImGui::SameLine();
                ImGui::TextColored(pid_to_decode == current_pid ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(current_pid));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string S2UDPxRITCADUextractor::getID()
    {
        return "s2_udp_cadu_extractor";
    }

    std::vector<std::string> S2UDPxRITCADUextractor::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> S2UDPxRITCADUextractor::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<S2UDPxRITCADUextractor>(input_file, output_file_hint, parameters);
    }
}