#include "module_kanopusv_decoder.h"
#include "common/codings/differential/qpsk_diff.h"
#include "common/codings/soft_reader.h"
#include "common/repack_bits_byte.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "utils/binary.h"
#include <cstdint>

#define BUFFER_SIZE (1024 * 8)

using namespace satdump;

namespace kanopus
{
    KanopusVDecoderModule::KanopusVDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        soft_buffer = new int8_t[BUFFER_SIZE];
        prediff_bits_buffer = new uint8_t[BUFFER_SIZE];
        bits_buffer = new uint8_t[BUFFER_SIZE];
        frameshift_buffer = new uint64_t[384];

        hard_symbs = getValueOrDefault(d_parameters["hard_symbols"], false);

        dump_diff = getValueOrDefault(d_parameters["dump_diff"], false);
        dump_stream = getValueOrDefault(d_parameters["dump_stream"], false);

        if (dump_diff || dump_stream)
            fsfsm_file_ext = ".bin";
        else
            fsfsm_file_ext = ".frm";
    }

    std::vector<ModuleDataType> KanopusVDecoderModule::getInputTypes() { return {DATA_FILE, DATA_STREAM}; }

    std::vector<ModuleDataType> KanopusVDecoderModule::getOutputTypes() { return {DATA_FILE}; }

    KanopusVDecoderModule::~KanopusVDecoderModule()
    {
        delete[] soft_buffer;
        delete[] prediff_bits_buffer;
        delete[] bits_buffer;
        delete[] frameshift_buffer;
    }

    void KanopusVDecoderModule::process()
    {
        diff::QPSKDiff diff;
        RepackBitsByte repack;

        while (should_run())
        {
            if (!hard_symbs)
            {
                // Read a buffer
                read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

                // Repack to bits fo the diff
                for (int i = 0; i < BUFFER_SIZE / 2; i++)
                    prediff_bits_buffer[i] = (soft_buffer[i * 2 + 0] >= 0) << 1 | (soft_buffer[i * 2 + 1] >= 0);
            }
            else
            {
                read_data((uint8_t *)soft_buffer, BUFFER_SIZE / 8);

                // We need only half...
                for (int i = 0; i < BUFFER_SIZE / 8; i++)
                {
                    prediff_bits_buffer[i * 4 + 0] = (((uint8_t *)soft_buffer)[i] >> 6) & 0b11;
                    prediff_bits_buffer[i * 4 + 1] = (((uint8_t *)soft_buffer)[i] >> 4) & 0b11;
                    prediff_bits_buffer[i * 4 + 2] = (((uint8_t *)soft_buffer)[i] >> 2) & 0b11;
                    prediff_bits_buffer[i * 4 + 3] = (((uint8_t *)soft_buffer)[i] >> 0) & 0b11;
                }
            }

            // Perform
            diff.work(prediff_bits_buffer, BUFFER_SIZE / 2, bits_buffer);

            // Invert 2nd branch
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                bits_buffer[i * 2 + 0] = bits_buffer[i * 2 + 0];
                bits_buffer[i * 2 + 1] = (~bits_buffer[i * 2 + 1]) & 1;
            }

            // Optionally, dump data already
            if (dump_diff)
            {
                int o = repack.work(bits_buffer, BUFFER_SIZE, bits_buffer);
                write_data(bits_buffer, o);
                continue;
            }

            // Next, start synchronizing
            process_bits(bits_buffer, BUFFER_SIZE);
        }

        logger->info("Decoding finished");

        cleanup();
    }

    inline uint8_t get_bit_in_buffer(uint64_t *buf, uint32_t pos) { return (buf[pos / 64] >> (63 - (pos % 64))) & 1; } // TODOREWORK put into utils

    void KanopusVDecoderModule::write_frame()
    {
        // PN Sequence
        uint8_t derand[48] = {0x55, 0xE2, 0xA4, 0x6A, 0x30, 0xA7, 0xDA, 0x16, 0x24, 0x86, 0x39, 0x07, 0x0D, 0x1D, 0xEC, 0x64, 0xF0, 0x3D, 0xD7, 0x66, 0x98, 0x08, 0x15, 0x94,
                              0xA0, 0xBA, 0x5B, 0x22, 0x3E, 0x67, 0x40, 0x1B, 0x54, 0x3A, 0xB7, 0x2B, 0x9E, 0xB0, 0x4B, 0xD3, 0xB6, 0xF3, 0x8D, 0xF1, 0xE5, 0xC4, 0x27, 0x36};

        // Deinterleave
        uint8_t deint_buffer[8][384];
        for (int i = 0; i < 3072 * 8; i++)
            deint_buffer[i % 8][i / 64] = deint_buffer[i % 8][i / 64] << 1 | get_bit_in_buffer(frameshift_buffer, i);

        // write_data(deint_buffer[0], 384);

        // Derand everything
        for (int f = 0; f < 8; f++)
            for (int i = 0; i < 8; i++)
                for (int ii = 0; ii < 48; ii++)
                    deint_buffer[f][i * 48 + ii] ^= derand[ii];

        // Write, re-interleaving into a full stream
        uint8_t final[384 * 8 - 16];
        for (int i = 0, y = 0; i < 8; i++)
        {
            for (int ii = 0; ii < 48; ii++)
            {
                for (int f : {1, 0, 3, 2, 5, 4, 7, 6})
                    if (f < 2 && ii == 0)
                        ;
                    else
                        final[y++] = deint_buffer[f][i * 48 + ii];
            }
        }

        frame_count++;

        if (dump_stream)
        {
            write_data(final, 3056);
            return;
        }

        // Finally sync PMSS Frames & write
        auto pmss_frames = pmss_deframer.work(final, 3056);

        for (auto &f : pmss_frames)
        {
            write_data(f.data(), 15680 / 8);
            pmss_frame_count++;
        }
    }

    void KanopusVDecoderModule::process_bits(uint8_t *bits, int cnt)
    {
        const uint8_t SYNC_0 = 0b00000100;
        const uint8_t SYNC_1 = 0b10110011;
        const uint8_t SYNC_2 = 0b11100011;
        const uint8_t SYNC_3 = 0b01110101;
        // SYNC_4 Counter
        // SYNC_5 No idea, some marker?
        // SYNC_6 Changes
        // SYNC_7 Counter

        for (int c = 0; c < cnt; c++)
        {
            // Shift into sync buffer. uint64_t to optimize things
            for (int i = 0; i < (384 - 1); i++)
                frameshift_buffer[i] = (frameshift_buffer[i] << 1) | (frameshift_buffer[i + 1] >> 63);
            frameshift_buffer[(384 - 1)] = (frameshift_buffer[(384 - 1)] << 1) | bits[c];

            // Skip to next frame as needed
            if (skip > 0)
            {
                skip--;
                continue;
            }

            // Extract what should be sync bytes
            uint8_t sync_bytes[2][4];

            for (int f = 0; f < 2; f++)
            {
                for (int s = 0; s < 4; s++)
                {
                    sync_bytes[f][s] = get_bit_in_buffer(frameshift_buffer, ((s * 384) + 0) * 8 + f) << 7 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 1) * 8 + f) << 6 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 2) * 8 + f) << 5 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 3) * 8 + f) << 4 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 4) * 8 + f) << 3 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 5) * 8 + f) << 2 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 6) * 8 + f) << 1 | //
                                       get_bit_in_buffer(frameshift_buffer, ((s * 384) + 7) * 8 + f);
                }
            }

            // Diff them & count
            uint8_t final_diff = count_ones(sync_bytes[0][0] ^ SYNC_0) + count_ones(sync_bytes[1][0] ^ SYNC_0) + //
                                 count_ones(sync_bytes[0][1] ^ SYNC_1) + count_ones(sync_bytes[1][1] ^ SYNC_1) + //
                                 count_ones(sync_bytes[0][2] ^ SYNC_2) + count_ones(sync_bytes[1][2] ^ SYNC_2) + //
                                 count_ones(sync_bytes[0][3] ^ SYNC_3) + count_ones(sync_bytes[1][3] ^ SYNC_3);

            if (!synced)
            {
                for (int i = 0; i < 8; i++)
                {
                    if (final_diff == 0)
                    {
                        synced = true;
                        write_frame();
                        skip = 3072 * 8 - 1;
                        bad_sync = 0;
                        break;
                    }
                }
            }
            else
            {
                if (final_diff < 10)
                {
                    write_frame();
                    skip = 3072 * 8 - 1;
                    bad_sync = 0;
                }
                else
                    bad_sync++;

                if (bad_sync > 10)
                    synced = false;
            }
        }
    }

    nlohmann::json KanopusVDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["synced"] = synced;
        return v;
    }

    void KanopusVDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Kanopus-V Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("State : ");

            ImGui::SameLine();

            if (!synced)
                ImGui::TextColored(style::theme.red, "NOSYNC");
            else
                ImGui::TextColored(style::theme.green, "SYNCED");

            ImGui::Text("Frames : ");

            ImGui::SameLine();

            ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count));
        }

        ImGui::Button("Payload", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("PMSS : ");

            ImGui::SameLine();

            ImGui::TextColored(style::theme.green, UITO_C_STR(pmss_frame_count));
        }

        drawProgressBar();

        ImGui::End();
    }

    std::string KanopusVDecoderModule::getID() { return "kanopusv_decoder"; }

    std::shared_ptr<ProcessingModule> KanopusVDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<KanopusVDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace kanopus
