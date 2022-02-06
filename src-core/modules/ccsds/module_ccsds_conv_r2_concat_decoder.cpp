#include "module_ccsds_conv_r2_concat_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"
#include "common/codings/randomization.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace ccsds
{
    CCSDSConvR2ConcatDecoderModule::CCSDSConvR2ConcatDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters),
          is_ccsds(parameters.count("ccsds") > 0 ? parameters["ccsds"].get<bool>() : true),

          d_constellation_str(parameters["constellation"].get<std::string>()),

          d_cadu_size(parameters["cadu_size"].get<int>()),
          d_cadu_bytes(ceil(d_cadu_size / 8.0)), // If we can't use complete bytes, add one and padding
          d_buffer_size(d_cadu_size),

          d_viterbi_outsync_after(parameters["viterbi_outsync_after"].get<int>()),
          d_viterbi_ber_threasold(parameters["viterbi_ber_thresold"].get<float>()),

          d_diff_decode(parameters.count("nrzm") > 0 ? parameters["nrzm"].get<bool>() : false),

          d_derand(parameters.count("derandomize") > 0 ? parameters["derandomize"].get<bool>() : true),
          d_derand_after_rs(parameters.count("derand_after_rs") > 0 ? parameters["derand_after_rs"].get<bool>() : false),
          d_derand_from(parameters.count("derand_start") > 0 ? parameters["derand_start"].get<int>() : 4),

          d_rs_interleaving_depth(parameters["rs_i"].get<int>()),
          d_rs_dualbasis(parameters.count("rs_dualbasis") > 0 ? parameters["rs_dualbasis"].get<bool>() : true),
          d_rs_type(parameters.count("rs_type") > 0 ? parameters["rs_type"].get<std::string>() : "none")
    {
        viterbi_out = new uint8_t[d_buffer_size * 2];
        soft_buffer = new int8_t[d_buffer_size];
        frame_buffer = new uint8_t[d_cadu_size * 2]; // Larger by safety

        // Get constellation
        if (d_constellation_str == "bpsk")
        {
            d_constellation = dsp::BPSK;
            d_bpsk_90 = false;
        }
        else if (d_constellation_str == "bpsk_90")
        {
            d_constellation = dsp::BPSK;
            d_bpsk_90 = true;
        }
        else if (d_constellation_str == "qpsk")
            d_constellation = dsp::QPSK;
        else
            logger->critical("CCSDS Concatenated 1/2 Decoder : invalid constellation type!");

        std::vector<phase_t> d_phases;

        // Get phases for the viterbi decoder to check
        if (d_constellation == dsp::BPSK && !d_bpsk_90)
            d_phases = {PHASE_0};
        else if (d_constellation == dsp::BPSK && d_bpsk_90)
            d_phases = {PHASE_90};
        else if (d_constellation == dsp::QPSK)
            d_phases = {PHASE_0, PHASE_90};

        // Parse RS
        reedsolomon::RS_TYPE rstype;
        if (d_rs_interleaving_depth != 0)
        {
            if (d_rs_type == "rs223")
                rstype = reedsolomon::RS223;
            else if (d_rs_type == "rs239")
                rstype = reedsolomon::RS239;
            else
                logger->critical("CCSDS Concatenated 1/2 Decoder : invalid Reed-Solomon type!");
        }

        // Parse sync marker if set
        uint32_t asm_sync = 0x1acffc1d;
        if (parameters.count("asm") > 0)
            asm_sync = std::stoul(parameters["asm"].get<std::string>(), nullptr, 16);

        viterbi = std::make_shared<viterbi::Viterbi1_2>(d_viterbi_ber_threasold, d_viterbi_outsync_after, d_buffer_size, d_phases);
        deframer = std::make_shared<deframing::BPSK_CCSDS_Deframer>(d_cadu_size, asm_sync);
        if (d_rs_interleaving_depth != 0)
            reed_solomon = std::make_shared<reedsolomon::ReedSolomon>(rstype);

        if (d_cadu_size % 8 != 0) // If this is not a perfect byte length match, pad the frames
        {
            deframer->CADU_PADDING = d_cadu_size % 8;
            logger->info("Frames will be padded!");
        }
    }

    std::vector<ModuleDataType> CCSDSConvR2ConcatDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> CCSDSConvR2ConcatDecoderModule::getOutputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    CCSDSConvR2ConcatDecoderModule::~CCSDSConvR2ConcatDecoderModule()
    {
        delete[] viterbi_out;
        delete[] soft_buffer;
        delete[] frame_buffer;
    }

    void CCSDSConvR2ConcatDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        std::string extension = is_ccsds ? ".cadu" : ".frm";

        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + extension, std::ios::binary);
        d_output_files.push_back(d_output_file_hint + extension);

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + extension);

        time_t lastTime = 0;

        diff::NRZMDiff diff;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, d_buffer_size);
            else
                input_fifo->read((uint8_t *)soft_buffer, d_buffer_size);

            if (d_bpsk_90) // Symbols are swapped for some BPSK sats
                rotate_soft((int8_t *)soft_buffer, d_buffer_size, PHASE_0, true);

            // Perform Viterbi decoding
            int vitout = viterbi->work((int8_t *)soft_buffer, d_buffer_size, viterbi_out);

            if (d_diff_decode) // Diff decoding if required
                diff.decode_bits(viterbi_out, vitout);

            // Run deframer
            int frames = deframer->work(viterbi_out, vitout, frame_buffer);

            for (int i = 0; i < frames; i++)
            {
                uint8_t *cadu = &frame_buffer[i * d_cadu_bytes];

                if (d_derand && !d_derand_after_rs) // Derand if required, before RS
                    derand_ccsds(&cadu[d_derand_from], d_cadu_bytes - d_derand_from);

                if (d_rs_interleaving_depth != 0) // RS Correction
                    reed_solomon->decode_interlaved(&cadu[4], d_rs_dualbasis, d_rs_interleaving_depth, errors);

                if (d_derand && d_derand_after_rs) // Derand if required, after RS
                    derand_ccsds(&cadu[d_derand_from], d_cadu_bytes - d_derand_from);

                // Write it out
                data_out.write((char *)cadu, d_cadu_bytes);
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi_state = viterbi->getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer->getState() == deframer->STATE_NOSYNC ? "NOSYNC" : (deframer->getState() == deframer->STATE_SYNCING ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi->ber()) + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void CCSDSConvR2ConcatDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("CCSDS r=1/2 Concatenated Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);
        float ber = viterbi->ber();

        ImGui::BeginGroup();
        {
            // Constellation
            if (d_constellation == dsp::BPSK)
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
            else
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)soft_buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
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
            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (viterbi->getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi->getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber));

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

            if (d_rs_interleaving_depth != 0)
            {
                ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("RS    : ");
                    for (int i = 0; i < d_rs_interleaving_depth; i++)
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
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string CCSDSConvR2ConcatDecoderModule::getID()
    {
        return "ccsds_conv_r2_concat_decoder";
    }

    std::vector<std::string> CCSDSConvR2ConcatDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold"};
    }

    std::shared_ptr<ProcessingModule> CCSDSConvR2ConcatDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<CCSDSConvR2ConcatDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace npp