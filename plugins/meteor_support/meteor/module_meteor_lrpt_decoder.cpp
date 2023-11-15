#include "module_meteor_lrpt_decoder.h"
#include "logger.h"
#include "common/codings/rotation.h"
#include "common/codings/randomization.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"
#include "common/codings/viterbi/viterbi27.h"
#include "common/codings/correlator.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "deint.h"

#define BUFFER_SIZE 8192
#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace meteor
{
    METEORLRPTDecoderModule::METEORLRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                        diff_decode(parameters["diff_decode"].get<bool>())
    {
        m2x_mode = d_parameters.contains("m2x_mode") ? d_parameters["m2x_mode"].get<bool>() : false;
        _buffer = new int8_t[ENCODED_FRAME_SIZE + INTER_MARKER_STRIDE];
        buffer = &_buffer[INTER_MARKER_STRIDE];

        if (m2x_mode)
        {
            int d_viterbi_outsync_after = parameters["viterbi_outsync_after"].get<int>();
            float d_viterbi_ber_threasold = parameters["viterbi_ber_thresold"].get<float>();
            std::vector<phase_t> phases = {PHASE_0, PHASE_90};
            viterbin = std::make_shared<viterbi::Viterbi1_2>(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, phases, true);
            deframer = std::make_shared<deframing::BPSK_CCSDS_Deframer>(8192);

            if (d_parameters.contains("interleaved") ? d_parameters["interleaved"].get<bool>() : false)
            {
                _buffer2 = new int8_t[ENCODED_FRAME_SIZE + INTER_MARKER_STRIDE];
                buffer2 = &_buffer2[INTER_MARKER_STRIDE];
                viterbin2 = std::make_shared<viterbi::Viterbi1_2>(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, phases, true);
            }
        }
        else
        {
            viterbi = std::make_shared<viterbi::Viterbi27>(ENCODED_FRAME_SIZE / 2, viterbi::CCSDS_R2_K7_POLYS);
        }
    }

    std::vector<ModuleDataType> METEORLRPTDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> METEORLRPTDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    METEORLRPTDecoderModule::~METEORLRPTDecoderModule()
    {
        delete[] _buffer;
        if (d_parameters.contains("interleaved") ? d_parameters["interleaved"].get<bool>() : false)
            delete[] _buffer2;
    }

    class DintSampleReader
    {
    private:
        bool iserror = false;
        std::vector<int8_t> buffer1, buffer2;

        void read_more()
        {
            buffer1.resize(buffer1.size() + 8192);
            iserror = iserror ||
                      !input_function(&buffer1[buffer1.size() - 8192], 8192);

            buffer2.resize(buffer2.size() + 8192);
            memcpy(&buffer2[buffer2.size() - 8192], &buffer1[buffer1.size() - 8192], 8192);
            rotate_soft(&buffer2[buffer2.size() - 8192], 8192, PHASE_90, false);
        }

    public:
        std::function<int(int8_t *, size_t)> input_function;

        int read1(int8_t *buf, size_t len)
        {
            while (buffer1.size() < len && !iserror)
                read_more();
            if (iserror)
                return 0;
            memcpy(buf, buffer1.data(), len);
            buffer1.erase(buffer1.begin(), buffer1.begin() + len);
            return len;
        }

        int read2(int8_t *buf, size_t len)
        {
            while (buffer2.size() < len && !iserror)
                read_more();
            if (iserror)
                return 0;
            memcpy(buf, buffer2.data(), len);
            buffer2.erase(buffer2.begin(), buffer2.begin() + len);
            return len;
        }
    };

    void METEORLRPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        if (m2x_mode)
        {
            bool interleaved = d_parameters.contains("interleaved") ? d_parameters["interleaved"].get<bool>() : false;

            std::shared_ptr<DeinterleaverReader> deint1, deint2;

            if (interleaved)
            {
                deint1 = std::make_shared<DeinterleaverReader>();
                deint2 = std::make_shared<DeinterleaverReader>();
            }

            uint8_t *viterbi_out = new uint8_t[BUFFER_SIZE * 2];
            uint8_t *viterbi_out2 = new uint8_t[BUFFER_SIZE * 2];
            uint8_t *frame_buffer = new uint8_t[BUFFER_SIZE * 2];

            reedsolomon::ReedSolomon reed_solomon(reedsolomon::RS223);
            diff::NRZMDiff diff;

            DintSampleReader file_reader;
            if (input_data_type == DATA_FILE)
                file_reader.input_function =
                    [this](int8_t *buf, size_t len) -> int
                { return !(!data_in.read((char *)buf, len)); };
            else
                file_reader.input_function =
                    [this](int8_t *buf, size_t len) -> int
                { return !(!input_fifo->read((uint8_t *)buf, len)); };

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (interleaved)
                {
                    deint1->read_samples([&file_reader](int8_t *buf, size_t len) -> int
                                         { return (bool)file_reader.read1(buf, len); },
                                         buffer, 8192);
                    deint2->read_samples([&file_reader](int8_t *buf, size_t len) -> int
                                         { return (bool)file_reader.read2(buf, len); },
                                         buffer2, 8192);
                }
                else
                {
                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)buffer, BUFFER_SIZE);
                    else
                        input_fifo->read((uint8_t *)buffer, BUFFER_SIZE);
                }

                // Perform Viterbi decoding
                int vitout = 0, vitout1 = 0, vitout2 = 0;

                if (interleaved)
                {
                    vitout1 = viterbin->work((int8_t *)buffer, BUFFER_SIZE, viterbi_out);
                    vitout2 = viterbin2->work((int8_t *)buffer2, BUFFER_SIZE, viterbi_out2);
                    if (viterbin2->getState() > viterbin->getState())
                    {
                        vitout = vitout2;
                        viterbi_ber = viterbin2->ber();
                        viterbi_lock = viterbin2->getState();
                        memcpy(viterbi_out, viterbi_out2, vitout2);
                    }
                    else
                    {
                        vitout = vitout1;
                        viterbi_ber = viterbin->ber();
                        viterbi_lock = viterbin->getState();
                    }
                }
                else
                {
                    vitout = viterbin->work((int8_t *)buffer, BUFFER_SIZE, viterbi_out);
                    viterbi_ber = viterbin->ber();
                    viterbi_lock = viterbin->getState();
                }

                if (diff_decode) // Diff decoding if required
                    diff.decode_bits(viterbi_out, vitout);

                // Run deframer
                int frames = deframer->work(viterbi_out, vitout, frame_buffer);

                for (int i = 0; i < frames; i++)
                {
                    uint8_t *cadu = &frame_buffer[i * 1024];

                    derand_ccsds(&cadu[4], 1020);

                    // RS Correction
                    reed_solomon.decode_interlaved(&cadu[4], false, 4, errors);

                    if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                    {
                        // Write it out
                        if (output_data_type == DATA_STREAM)
                            output_fifo->write((uint8_t *)cadu, 1024);
                        else
                            data_out.write((char *)cadu, 1024);
                    }
                }

                // Update module stats
                module_stats["deframer_lock"] = deframer->getState() == deframer->STATE_SYNCED;
                module_stats["viterbi_ber"] = viterbi_ber;
                module_stats["viterbi_lock"] = viterbi_lock;
                module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string viterbi_state = viterbi_lock == 0 ? "NOSYNC" : "SYNCED";
                    std::string deframer_state = deframer->getState() == deframer->STATE_NOSYNC ? "NOSYNC" : (deframer->getState() == deframer->STATE_SYNCING ? "SYNCING" : "SYNCED");
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi_ber) + ", Deframer : " + deframer_state);
                }
            }

            delete[] viterbi_out;
            delete[] viterbi_out2;
            delete[] frame_buffer;
        }
        else
        {
            // Correlator
            Correlator correlator(QPSK, diff_decode ? 0xfc4ef4fd0cc2df89 : 0xfca2b63db00d9794);

            // Viterbi, rs, etc
            reedsolomon::ReedSolomon rs(reedsolomon::RS223);

            // Other buffers
            uint8_t frameBuffer[FRAME_SIZE];

            phase_t phase;
            bool swap;

            // Vectors are inverted
            diff::NRZMDiff diff;

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)buffer, ENCODED_FRAME_SIZE);
                else
                    input_fifo->read((uint8_t *)buffer, ENCODED_FRAME_SIZE);

                int pos = correlator.correlate((int8_t *)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

                locked = pos == 0; // Update locking state

                if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
                {
                    std::memmove(buffer, &buffer[pos], ENCODED_FRAME_SIZE - pos);

                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
                    else
                        input_fifo->read((uint8_t *)&buffer[ENCODED_FRAME_SIZE - pos], pos);
                }

                // Correct phase ambiguity
                rotate_soft(buffer, ENCODED_FRAME_SIZE, phase, swap);

                // Viterbi
                viterbi->work((int8_t *)buffer, frameBuffer);

                if (diff_decode)
                    diff.decode(frameBuffer, FRAME_SIZE);

                // Derandomize that frame
                derand_ccsds(&frameBuffer[4], FRAME_SIZE - 4);

                // There is a VERY rare edge case where CADUs end up inverted... Better cover it to be safe
                if (frameBuffer[9] == 0xFF)
                {
                    for (int i = 0; i < FRAME_SIZE; i++)
                        frameBuffer[i] ^= 0xFF;
                }

                // RS Correction
                rs.decode_interlaved(&frameBuffer[4], false, 4, errors);

                // Write it out if it's not garbage
                if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                {
                    data_out.put(0x1a);
                    data_out.put(0xcf);
                    data_out.put(0xfc);
                    data_out.put(0x1d);
                    data_out.write((char *)&frameBuffer[4], FRAME_SIZE - 4);
                }

                // Update module stats
                module_stats["correlator_lock"] = locked;
                module_stats["viterbi_ber"] = viterbi->ber();
                module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Viterbi BER : " + std::to_string(viterbin->ber() * 100) + "%%, Lock : " + lock_state);
                }
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void METEORLRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR LRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            if (m2x_mode)
            {
                float ber = viterbi_ber;

                ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (viterbi_lock == 0)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(viterbi_lock == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber));

                    std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                    ber_history[200 - 1] = ber;

                    ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (deframer->getState() == deframer->STATE_NOSYNC)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                    else if (deframer->getState() == deframer->STATE_SYNCING)
                        ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
                }

                ImGui::Spacing();

                ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("RS    : ");
                    for (int i = 0; i < 4; i++)
                    {
                        ImGui::SameLine();

                        if (errors[i] == -1)
                            ImGui::TextColored(IMCOLOR_NOSYNC, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(IMCOLOR_SYNCING, "%i ", i);
                        else
                            ImGui::TextColored(IMCOLOR_SYNCED, "%i ", i);
                    }
                }
            }
            else
            {
                float ber = viterbi->ber();

                ImGui::Button("Correlator", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(locked ? IMCOLOR_SYNCED : IMCOLOR_SYNCING, UITO_C_STR(cor));

                    std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                    cor_history[200 - 1] = cor;

                    ImGui::PlotLines("", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 40.0f, 64.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(ber < 0.22 ? IMCOLOR_SYNCED : IMCOLOR_NOSYNC, UITO_C_STR(ber));

                    std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                    ber_history[200 - 1] = ber;

                    ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("RS    : ");
                    for (int i = 0; i < 4; i++)
                    {
                        ImGui::SameLine();

                        if (errors[i] == -1)
                            ImGui::TextColored(IMCOLOR_NOSYNC, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(IMCOLOR_SYNCING, "%i ", i);
                        else
                            ImGui::TextColored(IMCOLOR_SYNCED, "%i ", i);
                    }
                }
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string METEORLRPTDecoderModule::getID()
    {
        return "meteor_lrpt_decoder";
    }

    std::vector<std::string> METEORLRPTDecoderModule::getParameters()
    {
        return {"diff_decode"};
    }

    std::shared_ptr<ProcessingModule> METEORLRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<METEORLRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor