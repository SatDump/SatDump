#include "module_meteor_qpsk_kmss_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/codings/differential/qpsk_diff.h"
#include "meteor_rec_deframer.h"
#include "common/simple_deframer.h"
#include "common/repack_bits_byte.h"
#include "common/codings/soft_reader.h"

#define BUFFER_SIZE (1024 * 8)

namespace meteor
{
    MeteorQPSKKmssDecoderModule::MeteorQPSKKmssDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters),
          constellation(1.0, 0.15, demod_constellation_size)
    {
        soft_buffer = new int8_t[BUFFER_SIZE];
        bit_buffer = new uint8_t[BUFFER_SIZE / 2];
        bit_buffer_2 = new uint8_t[BUFFER_SIZE];

        diff_buffer_1 = new uint8_t[BUFFER_SIZE];
        diff_buffer_2 = new uint8_t[BUFFER_SIZE];

        rfrm_buffer_1 = new uint8_t[12288 * 160];
        rfrm_buffer_2 = new uint8_t[12288 * 160];
        rpkt_buffer_1 = new uint8_t[12288 * 160];
        rpkt_buffer_2 = new uint8_t[12288 * 160];
    }

    std::vector<ModuleDataType> MeteorQPSKKmssDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> MeteorQPSKKmssDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    MeteorQPSKKmssDecoderModule::~MeteorQPSKKmssDecoderModule()
    {
        delete[] soft_buffer;
        delete[] bit_buffer;
        delete[] bit_buffer_2;

        delete[] diff_buffer_1;
        delete[] diff_buffer_2;

        delete[] rfrm_buffer_1;
        delete[] rfrm_buffer_2;
        delete[] rpkt_buffer_1;
        delete[] rpkt_buffer_2;
    }

    inline void reorgFrames(uint8_t *rfrm_buffer, int nfrm)
    {
        for (int i = 0; i < nfrm; i++)
        {
            //  logger->info("FRAME!");

            uint8_t *bytes = &rfrm_buffer[i * 3072];
            uint8_t buffer_tmp_1[3072 / 2];
            uint8_t buffer_tmp_2[3072 / 2];

            for (int x = 0; x < 3072 / 2; x++)
            {
                buffer_tmp_1[x] = (rfrm_buffer[x * 2 + 0] >> 4) << 4 | (rfrm_buffer[x * 2 + 1] >> 4);
                buffer_tmp_2[x] = (rfrm_buffer[x * 2 + 0] & 0xF) << 4 | (rfrm_buffer[x * 2 + 1] & 0xF);
            }

            for (int c = 0, b = 0; c < (3072 / 2) / 4; c++)
            {
                memcpy(&bytes[b], &buffer_tmp_1[c * 4], 4);
                b += 4;
                memcpy(&bytes[b], &buffer_tmp_2[c * 4], 4);
                b += 4;
            }
        }
    }

    inline int unpackPayload(uint8_t *rfrm_buffer, int nfrm, uint8_t *rpkt_buffer)
    {
        int rpktpos = 0;
        for (int i = 0; i < nfrm; i++)
        {
            uint8_t *bytes = &rfrm_buffer[i * 3072]; // 384];
            for (int b = 0; b < 16; b++)
            {
                memcpy(&rpkt_buffer[rpktpos], bytes + (192 * b) + 4, 188);
                rpktpos += 188;
            }
        }
        for (int i = 0; i < rpktpos; i++)
            rpkt_buffer[i] ^= 0xF0;
        return rpktpos;
    }

    inline void handleFrames(std::vector<std::vector<uint8_t>> &vf1, std::ofstream &data_out, std::mutex &writeMtx, int type)
    {
        for (auto &kfrm : vf1)
        {
            memmove(&kfrm[4], &kfrm[0], (15040 * 4) - 4);

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

            if (type == 1)
            {
                memmove(&final_frames[1][0], &final_frames[1][1], 15040 - 1);
                memmove(&final_frames[3][0], &final_frames[3][1], 15040 - 1);
            }
            else if (type == 2)
            {
                memmove(&final_frames[0][0], &final_frames[0][1], 15040 - 1);
                memmove(&final_frames[2][0], &final_frames[2][1], 15040 - 1);

                memmove(&final_frames[1][0], &final_frames[1][1], 15040 - 1);
                memmove(&final_frames[3][0], &final_frames[3][1], 15040 - 1);
            }
            else if (type == 3)
            {
                memmove(&final_frames[0][0], &final_frames[0][1], 15040 - 1);
                memmove(&final_frames[2][0], &final_frames[2][1], 15040 - 1);
            }

            writeMtx.lock();
            // 120320. x4 = 481280 !!
            data_out.write((char *)final_frames[0], 15040);
            data_out.write((char *)final_frames[2], 15040);
            data_out.write((char *)final_frames[1], 15040);
            data_out.write((char *)final_frames[3], 15040);
            writeMtx.unlock();
        }
    }

    void MeteorQPSKKmssDecoderModule::process()
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

        diff::QPSKDiff diff;
        RepackBitsByte repack_1;
        RepackBitsByte repack_2;

        KMSS_QPSK_ExtDeframer recorder_deframer_1;
        KMSS_QPSK_ExtDeframer recorder_deframer_2;

        // 20230901_191544_METEOR-M2 3.hard
        def::SimpleDeframer kmssbpsk_deframer1_1(0x3dcd25621e46b8, 56, 481280, 0, false);
        def::SimpleDeframer kmssbpsk_deframer1_2(0x3dcd25621e46b8, 56, 481280, 0, false);

        // 20230901_191942_METEOR-M2 3.hard
        def::SimpleDeframer kmssbpsk_deframer2_1(0x3fcf0fc03ccc30, 56, 481280, 0, false);
        def::SimpleDeframer kmssbpsk_deframer2_2(0x3fcf0fc03ccc30, 56, 481280, 0, false);

        // 20241030_002029_METEOR-M2 4.dat
        def::SimpleDeframer kmssbpsk_deframer3_1(0x3ece1a912d8974, 56, 481280, 0, false);
        def::SimpleDeframer kmssbpsk_deframer3_2(0x3ece1a912d8974, 56, 481280, 0, false);

        std::mutex writeMtx;

        bool convert_from_hard = false;
        if (d_parameters.contains("hard_symbols"))
            convert_from_hard = d_parameters["hard_symbols"];

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (convert_from_hard)
            {
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)soft_buffer, BUFFER_SIZE / 8);
                else
                    input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE / 8);

                int bit2pos = 0;
                for (int i = 0; i < BUFFER_SIZE / 8; i++)
                {
                    for (int x = 6; x >= 0; x -= 2)
                    {
                        bit_buffer[bit2pos++] = ((((uint8_t *)soft_buffer)[i] >> (x + 1)) & 1) << 1 |
                                                ((((uint8_t *)soft_buffer)[i] >> (x + 0)) & 1);
                    }
                }
            }
            else
            {
                soft_reader.readSoftSymbols(soft_buffer, BUFFER_SIZE);

                for (int i = 0; i < BUFFER_SIZE / 2; i++)
                    bit_buffer[i] = (soft_buffer[i * 2 + 0] >= 0) << 1 | (soft_buffer[i * 2 + 1] >= 0);
            }

            diff.work(bit_buffer, BUFFER_SIZE / 2, bit_buffer_2);

            // #pragma omp parallel sections num_threads(2)
            {
                // #pragma omp section
                {
                    for (int i = 0; i < BUFFER_SIZE / 2; i++)
                    {
                        diff_buffer_1[i * 2 + 0] = bit_buffer_2[i * 2 + 0];
                        diff_buffer_1[i * 2 + 1] = !bit_buffer_2[i * 2 + 1];
                    }

                    int nbytes_1 = repack_1.work(diff_buffer_1, BUFFER_SIZE, diff_buffer_1);
                    int nfrm_1 = recorder_deframer_1.work(diff_buffer_1, nbytes_1, rfrm_buffer_1);
                    reorgFrames(rfrm_buffer_1, nfrm_1);
                    int rpktpos_1 = unpackPayload(rfrm_buffer_1, nfrm_1, rpkt_buffer_1);

                    std::vector<std::vector<uint8_t>> vf1_1 = kmssbpsk_deframer1_1.work(rpkt_buffer_1, rpktpos_1);
                    std::vector<std::vector<uint8_t>> vf2_1 = kmssbpsk_deframer2_1.work(rpkt_buffer_1, rpktpos_1);
                    std::vector<std::vector<uint8_t>> vf3_1 = kmssbpsk_deframer3_1.work(rpkt_buffer_1, rpktpos_1);

                    handleFrames(vf1_1, data_out, writeMtx, 1);
                    handleFrames(vf2_1, data_out, writeMtx, 2);
                    handleFrames(vf3_1, data_out, writeMtx, 3);

                    writeMtx.lock();
                    frame_count += vf1_1.size();
                    frame_count += vf2_1.size();
                    frame_count += vf3_1.size();
                    writeMtx.unlock();
                }
                // #pragma omp section
                {
                    for (int i = 0; i < BUFFER_SIZE / 2; i++)
                    {
                        diff_buffer_2[i * 2 + 1] = !bit_buffer_2[i * 2 + 0];
                        diff_buffer_2[i * 2 + 0] = bit_buffer_2[i * 2 + 1];
                    }

                    int nbytes_2 = repack_2.work(diff_buffer_2, BUFFER_SIZE, diff_buffer_2);
                    int nfrm_2 = recorder_deframer_2.work(diff_buffer_2, nbytes_2, rfrm_buffer_2);
                    reorgFrames(rfrm_buffer_2, nfrm_2);
                    int rpktpos_2 = unpackPayload(rfrm_buffer_2, nfrm_2, rpkt_buffer_2);

                    std::vector<std::vector<uint8_t>> vf1_2 = kmssbpsk_deframer1_2.work(rpkt_buffer_2, rpktpos_2);
                    std::vector<std::vector<uint8_t>> vf2_2 = kmssbpsk_deframer2_2.work(rpkt_buffer_2, rpktpos_2);
                    std::vector<std::vector<uint8_t>> vf3_2 = kmssbpsk_deframer3_2.work(rpkt_buffer_2, rpktpos_2);

                    handleFrames(vf1_2, data_out, writeMtx, 1);
                    handleFrames(vf2_2, data_out, writeMtx, 2);
                    handleFrames(vf3_2, data_out, writeMtx, 3);

                    writeMtx.lock();
                    frame_count += vf1_2.size();
                    frame_count += vf2_2.size();
                    frame_count += vf3_2.size();
                    writeMtx.unlock();
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

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void MeteorQPSKKmssDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR QPSK KMSS Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

    std::string MeteorQPSKKmssDecoderModule::getID()
    {
        return "meteor_qpsk_kmss_decoder";
    }

    std::vector<std::string> MeteorQPSKKmssDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> MeteorQPSKKmssDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MeteorQPSKKmssDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor
