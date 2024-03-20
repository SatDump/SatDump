#include "module_meteor_xband_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/codings/differential/nrzm.h"
#include "meteor_rec_deframer.h"
#include "common/simple_deframer.h"
#include "common/repack_bits_byte.h"
#include "common/codings/soft_reader.h"

#define BUFFER_SIZE 8192

namespace meteor
{
    MeteorXBandDecoderModule::MeteorXBandDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters),
          constellation(1.0, 0.15, demod_constellation_size)
    {
        d_instrument_mode = parseDumpType(parameters);

        soft_buffer = new int8_t[BUFFER_SIZE];
        bit_buffer = new uint8_t[BUFFER_SIZE];

        if (d_instrument_mode == DUMP_TYPE_KMSS_BPSK)
            rfrm_buffer = new uint8_t[12288 * 4];
        else
            rfrm_buffer = new uint8_t[BUFFER_SIZE];

        if (d_instrument_mode == DUMP_TYPE_KMSS_BPSK)
            rpkt_buffer = new uint8_t[12288 * 4];
        else
            rpkt_buffer = new uint8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> MeteorXBandDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> MeteorXBandDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    MeteorXBandDecoderModule::~MeteorXBandDecoderModule()
    {
        delete[] soft_buffer;
        delete[] bit_buffer;
        delete[] rfrm_buffer;
        delete[] rpkt_buffer;
    }

    void MeteorXBandDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        logger->info("Using input data " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        SoftSymbolReader soft_reader(data_in, input_fifo, input_data_type, d_parameters);

        if (d_instrument_mode == DUMP_TYPE_MTVZA || d_instrument_mode == DUMP_TYPE_KMSS_BPSK)
        {
            diff::NRZMDiff nrzm;
            RepackBitsByte repack;

            MTVZA_ExtDeframer mtvza_recdef;
            KMSS_BPSK_ExtDeframer kmssbpsk_def;

            def::SimpleDeframer mtvza_deframer(0x38fb456a, 32, 3040, 0, false);
            def::SimpleDeframer kmssbpsk_deframer(0xaf5fff50aa5aa0, 56, 481280, 0, false);

            time_t lastTime = 0;
            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                soft_reader.readSoftSymbols(soft_buffer, BUFFER_SIZE);

                for (int i = 0; i < BUFFER_SIZE; i++)
                    bit_buffer[i] = soft_buffer[i] > 0;

                nrzm.decode_bits(bit_buffer, BUFFER_SIZE);

                if (d_instrument_mode == DUMP_TYPE_MTVZA)
                {
                    int nfrm = mtvza_recdef.work(bit_buffer, BUFFER_SIZE, rfrm_buffer);

                    int rpktpos = 0;
                    for (int i = 0; i < nfrm; i++)
                    {
                        uint8_t *bytes = &rfrm_buffer[i * 384];
                        memcpy(&rpkt_buffer[rpktpos], bytes + 2, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 50, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 98, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 146, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 194, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 242, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 290, 46);
                        rpktpos += 46;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 338, 46);
                        rpktpos += 46;
                    }

                    for (int i = 0; i < rpktpos; i++)
                        rpkt_buffer[i] ^= 0xAA;

                    auto deframed_dat = mtvza_deframer.work(rpkt_buffer, rpktpos);
                    frame_count += deframed_dat.size();
                    for (auto &frm : deframed_dat)
                        data_out.write((char *)frm.data(), 380);
                }
                else if (d_instrument_mode == DUMP_TYPE_KMSS_BPSK)
                {
                    int nbytes = repack.work(bit_buffer, BUFFER_SIZE, bit_buffer);
                    int nfrm = kmssbpsk_def.work(bit_buffer, nbytes, rfrm_buffer);

                    int rpktpos = 0;
                    for (int i = 0; i < nfrm; i++)
                    {
                        uint8_t *bytes = &rfrm_buffer[i * (KMSS_BPSK_REC_FRM_SIZE / 8)];
                        memcpy(&rpkt_buffer[rpktpos], bytes + 2, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 194, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 386, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 578, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 770, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 962, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 1154, 188);
                        rpktpos += 188;
                        memcpy(&rpkt_buffer[rpktpos], bytes + 1346, 188);
                        rpktpos += 188;
                    }

                    auto deframed_dat = kmssbpsk_deframer.work(rpkt_buffer, rpktpos);
                    frame_count += deframed_dat.size();
                    for (auto &kfrm : deframed_dat)
                    {
                        for (int u = 0; u < kfrm.size(); u++)
                            kfrm[u] ^= 0xF0;

                        uint8_t final_frames[4][15040];
                        for (int o = 0; o < 4; o++)
                            for (int c = 0; c < 15040; c++)
                                final_frames[o][c] =
                                    ((kfrm[c * 4 + 0] >> (7 - o)) & 1) << 7 |
                                    ((kfrm[c * 4 + 0] >> (3 - o)) & 1) << 6 |
                                    ((kfrm[c * 4 + 1] >> (7 - o)) & 1) << 5 |
                                    ((kfrm[c * 4 + 1] >> (3 - o)) & 1) << 4 |
                                    ((kfrm[c * 4 + 2] >> (7 - o)) & 1) << 3 |
                                    ((kfrm[c * 4 + 2] >> (3 - o)) & 1) << 2 |
                                    ((kfrm[c * 4 + 3] >> (7 - o)) & 1) << 1 |
                                    ((kfrm[c * 4 + 3] >> (3 - o)) & 1) << 0;

                        // 120320. x4 = 481280 !!
                        data_out.write((char *)final_frames[0], 15040);
                        data_out.write((char *)final_frames[1], 15040);
                        data_out.write((char *)final_frames[2], 15040);
                        data_out.write((char *)final_frames[3], 15040);
                    }
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                // module_stats["deframer_lock"] = def->getState() == 12;

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string deframer_state = ""; // def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Deframer : " + deframer_state + ", Frames : " + std::to_string(frame_count));
                }
            }
        }

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void MeteorXBandDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR X-Band Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // Constellation
        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFFER_SIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
#if 0
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (def->getState() == 0)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else if (def->getState() == 2 || def->getState() == 6)
                    ImGui::TextColored(style::theme.orange, "SYNCING");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");
            }
#endif

            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string MeteorXBandDecoderModule::getID()
    {
        return "meteor_xband_decoder";
    }

    std::vector<std::string> MeteorXBandDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> MeteorXBandDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MeteorXBandDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor