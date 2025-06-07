#include "module_s2udp_xrit_cadu_extractor.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/mpeg_ts/ts_header.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"

namespace satdump
{
    namespace pipeline
    {
        namespace xrit
        {
            S2UDPxRITCADUextractor::S2UDPxRITCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
            {
                pid_to_decode = d_parameters["pid"].get<int>();
                bbframe_size = d_parameters["bb_size"].get<int>();
                fsfsm_file_ext = ".cadu";
            }

            std::vector<ModuleDataType> S2UDPxRITCADUextractor::getInputTypes() { return {DATA_FILE, DATA_STREAM}; }

            std::vector<ModuleDataType> S2UDPxRITCADUextractor::getOutputTypes() { return {DATA_FILE}; }

            S2UDPxRITCADUextractor::~S2UDPxRITCADUextractor() {}

            void S2UDPxRITCADUextractor::process()
            {
                bool ts_input = d_parameters.contains("ts_input") ? d_parameters["ts_input"].get<bool>() : false;

                uint8_t *bb_buffer = new uint8_t[bbframe_size];
                uint8_t ts_frames[188 * 100];

                dvbs2::BBFrameTSParser ts_extractor(bbframe_size);

                // TS Stuff
                int ts_cnt = 0;
                mpeg_ts::TSHeader ts_header;
                mpeg_ts::TSDemux ts_demux;

                while (should_run())
                {
                    if (ts_input)
                    {
                        // Read buffer
                        read_data(ts_frames, 188);
                        ts_cnt = 1;
                    }
                    else
                    {
                        // Read buffer
                        read_data(bb_buffer, bbframe_size / 8);
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
                                write_data(&payload[40], 1024);
                            }
                        }
                    }
                }

                delete[] bb_buffer;

                cleanup();
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
                        ImGui::TextColored(pid_to_decode == current_pid ? style::theme.green : style::theme.orange, UITO_C_STR(current_pid));
                    }
                }
                ImGui::EndGroup();

                drawProgressBar();

                ImGui::End();
            }

            std::string S2UDPxRITCADUextractor::getID() { return "s2_udp_cadu_extractor"; }

            std::shared_ptr<ProcessingModule> S2UDPxRITCADUextractor::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            {
                return std::make_shared<S2UDPxRITCADUextractor>(input_file, output_file_hint, parameters);
            }
        } // namespace xrit
    } // namespace pipeline
} // namespace satdump