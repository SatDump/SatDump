#include "module_s2udp_xrit_cadu_extractor.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
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
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input bbframes " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        const int udp_cadu_size = 1104;

        uint8_t *bb_buffer = new uint8_t[bbframe_size];
        uint8_t ts_frames[188 * 100];
        int cadu_demuxed_cnt = 0;
        uint8_t cadu_demuxed[1104];

        dvbs2::BBFrameTSParser ts_extractor(bbframe_size);

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)bb_buffer, bbframe_size / 8);
            else
                input_fifo->read((uint8_t *)bb_buffer, bbframe_size / 8);

            int ts_cnt = ts_extractor.work(bb_buffer, 1, ts_frames);

            for (int i = 0; i < ts_cnt; i++)
            {
                uint8_t *ts_frame = &ts_frames[i * 188];

                current_pid = (ts_frames[1] & 0b11111) << 8 | ts_frame[2]; // Extract PID
                bool is_new_payload = (ts_frame[1] >> 6) & 1;              // Is this a new payload?

                if (current_pid == pid_to_decode) // Select the right PID
                {
                    if (is_new_payload && cadu_demuxed_cnt > 0) // Write payload, if we have one
                    {
                        if (cadu_demuxed[41] == 0x1a && cadu_demuxed[42] == 0xcf && cadu_demuxed[43] == 0xfc && cadu_demuxed[44] == 0x1d) // Check this is a CADU and not other IP data
                            data_out.write((char *)&cadu_demuxed[41], 1024);
                        cadu_demuxed_cnt = 0;
                        memset(cadu_demuxed, 0, udp_cadu_size);
                    }

                    if (cadu_demuxed_cnt >= udp_cadu_size) // Don't go over frame size
                        continue;

                    memcpy(&cadu_demuxed[cadu_demuxed_cnt], &cadu_demuxed[4], 184); // Feed it in
                    cadu_demuxed_cnt += 184;
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        delete[] bb_buffer;

        data_out.close();
        if (output_data_type == DATA_FILE)
            data_in.close();
    }

    void S2UDPxRITCADUextractor::drawUI(bool window)
    {
        ImGui::Begin("DVB-S2 UDP xRIT CADU Extractor", NULL, window ? NULL : NOWINDOW_FLAGS);
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
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

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