/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include <fstream>

#define _USE_MATH_DEFINES
#include <math.h>
#include "common/codings/dvb-s2/bbframe_descramble.h"
#include "common/codings/dvb-s2/bbframe_bch.h"
#include "common/codings/dvb-s2/bbframe_ldpc.h"
#include "common/codings/dvb-s2/s2_deinterleaver.h"
#include "common/dsp/demod//constellation.h"
#include "common/codings/dvb-s2/s2_scrambling.h"
#include "modules/demod/dvbs2/s2_defs.h"
#include "common/codings/dvb-s2/modcod_to_cfg.h"

#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/filter/firdes.h"
#include "common/dsp/io/file_sink.h"

#include "common/cli_utils.h"

namespace
{
    unsigned char crc_tab[256];

#define CRC_POLY 0xAB
// Reversed
#define CRC_POLYR 0xD5

    void build_crc8_table(void)
    {
        int r, crc;

        for (int i = 0; i < 256; i++)
        {
            r = i;
            crc = 0;
            for (int j = 7; j >= 0; j--)
            {
                if ((r & (1 << j) ? 1 : 0) ^ ((crc & 0x80) ? 1 : 0))
                {
                    crc = (crc << 1) ^ CRC_POLYR;
                }
                else
                {
                    crc <<= 1;
                }
            }
            crc_tab[i] = crc;
        }
    }

    /*
     * MSB is sent first
     *
     * The polynomial has been reversed
     */
    int add_crc8_bits(unsigned char *in, int length)
    {
        int crc = 0;
        int b;

        for (int n = 0; n < length; n++)
        {
            b = ((in[n / 8] >> (7 - (n % 8))) & 1) ^ (crc & 0x01);
            crc >>= 1;
            if (b)
            {
                crc ^= CRC_POLY;
            }
        }

        uint8_t crc2 = 0;
        for (int i = 0; i < 8; i++)
            crc2 = crc2 << 1 | ((crc >> i) & 1);

        return crc2;
    }
}

#include "common/codings/dvb-s2/bbframe_ts_parser.h"

int main(int argc, char *argv[])
{
    initLogger();
    build_crc8_table();

    if (argc < 3)
    {
        logger->error(std::string(argv[0]) + " input.ts output.f32 --modcod 11 --baseband_format f32 --rrc_alpha 0.25 [--shortframes] [--pilots]");
        return 0;
    }

    ///////////////
    std::string ts_input_path = argv[1];
    std::string baseband_output_path = argv[2];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 3, &argv[3]);

    int modulator_modcod = parameters["modcod"].get<int>();
    bool modulator_shortframes = parameters.contains("shortframes") ? parameters["shortframes"].get<bool>() : false;
    bool modulator_pilots = parameters.contains("pilots") ? parameters["pilots"].get<bool>() : false;
    float modulator_rrc_alpha = parameters["rrc_alpha"].get<float>();
    std::string baseband_format = parameters["baseband_format"].get<std::string>();
    /////////////

    // File stuff
    // std::ofstream bbframes_test("bbframe_test.bin");
    std::ifstream ts_input(ts_input_path);

    // Get CFG
    auto cfg = dvbs2::get_dvbs2_cfg(modulator_modcod, modulator_shortframes, modulator_pilots);

    dvbs2::BBFrameDescrambler bbframe_scra(cfg.framesize, cfg.coderate);
    dvbs2::BBFrameBCH bbframe_bch(cfg.framesize, cfg.coderate);
    dvbs2::BBFrameLDPC bbframe_ldpc(cfg.framesize, cfg.coderate);
    dvbs2::S2Deinterleaver interleaver(cfg.constellation, cfg.framesize, cfg.coderate);
    dsp::constellation_t constellation_slots(cfg.constel_obj_type, cfg.g1, cfg.g2);
    dvbs2::S2Scrambling s2_scrambling;

    // Bunch of variables
    int plframe_slots = cfg.frame_slot_count;
    int final_frm_size = cfg.framesize == dvbs2::FECFRAME_NORMAL ? 64800 : 16200;
    int final_frm_size_bytes = final_frm_size / 8;
    int bb_size = bbframe_bch.dataSize();
    // int bb_bytes = bb_size / 8;
    int bb_data_size = bb_size - 10 * 8;
    int bb_data_size_bytes = bb_data_size / 8;

    logger->warn("PL Slots : {:d}", plframe_slots);

    // Main BB frame buffer
    uint8_t *bbframe_raw = new uint8_t[final_frm_size_bytes];

    // Interleaving stuff
    uint8_t *frame_bits_buf = new uint8_t[final_frm_size];
    uint8_t *frame_bits_buf_interleaved = new uint8_t[final_frm_size];

    // Modulator stuff
    int pls_code = modulator_modcod << 2 | modulator_shortframes << 1 | modulator_pilots;
    dvbs2::s2_sof sof;
    dvbs2::s2_plscodes pls;
    // complex_t *modulated_frame; //[(plframe_slots + 1) * 90];

    // TS Stuff
    int ts_bytes_remaining = 0;
    int current_ts_offset = 0;
    uint8_t current_ts[188];
    uint8_t ts_crc = 0;

    // DSP To do pulse-shaping and saving to disk
    std::shared_ptr<dsp::stream<complex_t>> input_dsp_stream = std::make_shared<dsp::stream<complex_t>>();
    // modulated_frame = input_dsp_stream->writeBuf;

    dsp::RationalResamplerBlock<complex_t> resampler_blk(input_dsp_stream, 2, 1, dsp::firdes::root_raised_cosine(1, 2, 1, 0.25, 31));
    std::shared_ptr<dsp::FileSinkBlock> file_sink_blk = std::make_shared<dsp::FileSinkBlock>(resampler_blk.output_stream);

    resampler_blk.start();
    file_sink_blk->start();
    file_sink_blk->set_output_sample_type(dsp::basebandTypeFromString(baseband_format));
    file_sink_blk->start_recording(baseband_output_path, 2e6, 0, true);

    while (!ts_input.eof())
    {
        // Reset
        memset(bbframe_raw, 0, final_frm_size_bytes);

        // Encoder BBFrame
        {
            // MAPTYPE
            uint8_t ts_gs = 0b11;
            uint8_t sis_mis = 0b1;
            uint8_t ccm_acm = 0b1;
            uint8_t issyi = 0b0;
            uint8_t npd = 0b0;
            uint8_t ro = 0b11;

            if (modulator_rrc_alpha == 0.35)
                ro = 0b00;
            else if (modulator_rrc_alpha == 0.25)
                ro = 0b01;
            else if (modulator_rrc_alpha == 0.20)
                ro = 0b10;

            bbframe_raw[0] = ts_gs << 6 | sis_mis << 5 | ccm_acm << 4 | issyi << 3 | npd << 2 | ro;
            bbframe_raw[1] = 0x00; // RESERVED since ISI not set

            // UPL
            uint16_t user_pkt_length = 188 * 8; // TS

            bbframe_raw[2] = user_pkt_length >> 8;
            bbframe_raw[3] = user_pkt_length & 0xFF;

            // DFL
            uint16_t bb_pkt_length = bb_data_size;

            bbframe_raw[4] = bb_pkt_length >> 8;
            bbframe_raw[5] = bb_pkt_length & 0xFF;

            // SYNC
            bbframe_raw[6] = 0x47; // TS

            // Now, we put the TS in and set SYNCD
            uint8_t b;
            for (int i = 0; i < bb_data_size_bytes; i++)
            {
                if (ts_bytes_remaining == 0) // Read a new TS if required
                {
                    ts_input.read((char *)current_ts, 188);
                    ts_bytes_remaining = 188;
                }

                if (ts_bytes_remaining == 188)
                {
                    if (current_ts[0] != 0x47)
                        logger->error("TS Input error, sync not 0x47!");

                    current_ts[0] = ts_crc;

                    b = ts_crc;
                    ts_crc = 0;
                }
                else
                {
                    b = current_ts[188 - ts_bytes_remaining];
                    ts_crc = crc_tab[b ^ ts_crc];
                }

                bbframe_raw[10 + i] = current_ts[188 - ts_bytes_remaining--];
            }

            uint16_t syncd = current_ts_offset * 8;
            bbframe_raw[7] = syncd >> 8;
            bbframe_raw[8] = syncd & 0xFF;

            if (ts_bytes_remaining == 0)
                current_ts_offset = 0;
            else
                current_ts_offset = ts_bytes_remaining;

            // Compute CRC
            bbframe_raw[9] = add_crc8_bits(bbframe_raw, 72);
        }

        // Scrambling
        bbframe_scra.work(bbframe_raw);

        // BCH Encoder
        bbframe_bch.encode(bbframe_raw);

        // LDPC Encoder
        bbframe_ldpc.encode(bbframe_raw);

        // Convert the frame to bits
        for (int i = 0; i < final_frm_size; i++)
            frame_bits_buf[i] = (bbframe_raw[i / 8] >> (7 - (i % 8))) & 1;

        // Interleave
        interleaver.interleave(frame_bits_buf, frame_bits_buf_interleaved);

        // Insert PL Header stuff
        for (int i = 0; i < 26; i++)
            input_dsp_stream->writeBuf[i] = sof.symbols[i];

        for (int i = 26; i < 90; i++)
            input_dsp_stream->writeBuf[i] = pls.symbols[pls_code][i - 26];

        // Now, time to modulate slots
        int sym_pos = 0;
        s2_scrambling.reset();
        for (int slot = 0; slot < plframe_slots; slot++)
        {
            complex_t *slotp = &input_dsp_stream->writeBuf[(slot + 1) * 90];

            for (int i = 0; i < 90; i++)
            {
                uint8_t const_bits = 0;
                for (int i = 0; i < constellation_slots.getBitsCnt(); i++)
                    const_bits = const_bits << 1 | !frame_bits_buf_interleaved[sym_pos++];

                complex_t symbol = constellation_slots.mod(const_bits);

                if (cfg.constellation == dvbs2::MOD_QPSK)
                    slotp[i] = s2_scrambling.scramble(symbol) * 1.35;
                else
                    slotp[i] = s2_scrambling.scramble(symbol);
            }
        }

        // Scale samples down, so they fit in "1"
        for (int i = 0; i < (plframe_slots + 1) * 90; i++)
            input_dsp_stream->writeBuf[i] *= 0.7;

        // bbframes_test.write((char *)input_dsp_stream->writeBuf, (plframe_slots + 1) * 90 * sizeof(complex_t));
        input_dsp_stream->swap((plframe_slots + 1) * 90);
    }

    resampler_blk.stop();
    file_sink_blk->stop_recording();
    file_sink_blk->stop();
}
