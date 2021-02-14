#include "module_fengyun_ahrpt_decoder.h"
#include <fstream>
#include "logger.h"
#include "modules/common/deframer.h"
#include "modules/common/reedsolomon.h"
#include "ahrpt_viterbi.h"
#include "diff.h"
#include "modules/metop/instruments/iasi/utils.h"
#include "modules/common/ctpl/ctpl_stl.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace fengyun
{
    FengyunAHRPTDecoderModule::FengyunAHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                                d_viterbi_outsync_after(std::stoi(parameters["viterbi_outsync_after"])),
                                                                                                                                                                d_viterbi_ber_threasold(std::stof(parameters["viterbi_ber_thresold"])),
                                                                                                                                                                d_soft_symbols(std::stoi(parameters["soft_symbols"])),
                                                                                                                                                                d_invert_second_viterbi(std::stoi(parameters["invert_second_viterbi"]))
    {
        viterbi_out = new uint8_t[BUFFER_SIZE];
        sym_buffer = new std::complex<float>[BUFFER_SIZE];
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
        viterbi1_out = new uint8_t[BUFFER_SIZE];
        viterbi2_out = new uint8_t[BUFFER_SIZE];
        iSamples = new std::complex<float>[BUFFER_SIZE];
        qSamples = new std::complex<float>[BUFFER_SIZE];
        diff_in = new uint8_t[BUFFER_SIZE];
        diff_out = new uint8_t[BUFFER_SIZE];
    }

    void FengyunAHRPTDecoderModule::process()
    {
        size_t filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);
        std::ofstream data_out(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Our 2 Viterbi decoders and differential decoder
        FengyunAHRPTViterbi viterbi1(true, d_viterbi_ber_threasold, 1, d_viterbi_outsync_after, 50), viterbi2(true, d_viterbi_ber_threasold, 1, d_viterbi_outsync_after, 50);
        FengyunDiff diff;

        SatHelper::ReedSolomon reedSolomon;
        CADUDeframer deframer;

        uint8_t frameBuffer[BUFFER_SIZE];
        int inFrameBuffer = 0;

        // Multithreading stuff
        ctpl::thread_pool viterbi_pool(2);

        while (!data_in.eof())
        {

            // Read a buffer
            if (d_soft_symbols)
            {
                data_in.read((char *)soft_buffer, BUFFER_SIZE * 2);

                // Convert to hard symbols from soft symbols. We may want to work with soft only later?
                for (int i = 0; i < BUFFER_SIZE; i++)
                {
                    using namespace std::complex_literals;
                    sym_buffer[i] = ((float)soft_buffer[i * 2 + 1] / 127.0f) + ((float)soft_buffer[i * 2] / 127.0f) * 1if;
                }
            }
            else
            {
                data_in.read((char *)sym_buffer, sizeof(std::complex<float>) * BUFFER_SIZE);
            }

            inI = 0;
            inQ = 0;

            // Deinterleave I & Q for the 2 Viterbis
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                using namespace std::complex_literals;
                std::complex<float> iS = sym_buffer[i * 2 + shift].imag() + sym_buffer[i * 2 + 1 + shift].imag() * 1if;
                std::complex<float> qS = sym_buffer[i * 2 + shift].real() + sym_buffer[i * 2 + 1 + shift].real() * 1if;
                iSamples[inI++] = iS;
                if (d_invert_second_viterbi)
                {
                    qSamples[inQ++] = -qS; //FY3C
                }
                else
                {
                    qSamples[inQ++] = qS; // FY3B
                }
            }

            // Run Viterbi!
            v1_fut = viterbi_pool.push([&](int) { v1 = viterbi1.work(qSamples, inQ, viterbi1_out); });
            v2_fut = viterbi_pool.push([&](int) { v2 = viterbi2.work(iSamples, inI, viterbi2_out); });
            v1_fut.get();
            v2_fut.get();

            diffin = 0;

            // Interleave and pack output into 2 bits chunks
            if (v1 > 0 || v2 > 0)
            {
                if (v1 == v2)
                {
                    uint8_t bit1, bit2, bitCb;
                    for (int y = 0; y < v1; y++)
                    {
                        for (int i = 7; i >= 0; i--)
                        {
                            bit1 = getBit<uint8_t>(viterbi1_out[y], i);
                            bit2 = getBit<uint8_t>(viterbi2_out[y], i);
                            diff_in[diffin++] = bit2 << 1 | bit1;
                        }
                    }
                }
            }
            else
            {
                if (shift)
                {
                    shift = 0;
                }
                else
                {
                    shift = 1;
                }

                inI = 0;
                inQ = 0;

                // Deinterleave I & Q for the 2 Viterbis
                for (int i = 0; i < BUFFER_SIZE / 2; i++)
                {
                    using namespace std::complex_literals;
                    std::complex<float> iS = sym_buffer[i * 2 + shift].imag() + sym_buffer[i * 2 + 1 + shift].imag() * 1if;
                    std::complex<float> qS = sym_buffer[i * 2 + shift].real() + sym_buffer[i * 2 + 1 + shift].real() * 1if;
                    iSamples[inI++] = iS;
                    if (d_invert_second_viterbi)
                    {
                        qSamples[inQ++] = -qS; //FY3C
                    }
                    else
                    {
                        qSamples[inQ++] = qS; // FY3B
                    }
                }
                // Run Viterbi!
                v1_fut = viterbi_pool.push([&](int) { v1 = viterbi1.work(qSamples, inQ, viterbi1_out); });
                v2_fut = viterbi_pool.push([&](int) { v2 = viterbi2.work(iSamples, inI, viterbi2_out); });
                v1_fut.get();
                v2_fut.get();

                diffin = 0;

                // Interleave and pack output into 2 bits chunks
                if (v1 > 0 || v2 > 0)
                {
                    if (v1 == v2)
                    {
                        uint8_t bit1, bit2, bitCb;
                        for (int y = 0; y < v1; y++)
                        {
                            for (int i = 7; i >= 0; i--)
                            {
                                bit1 = getBit<uint8_t>(viterbi1_out[y], i);
                                bit2 = getBit<uint8_t>(viterbi2_out[y], i);
                                diff_in[diffin++] = bit2 << 1 | bit1;
                            }
                        }
                    }
                }
                else
                {
                    if (shift)
                    {
                        shift = 0;
                    }
                    else
                    {
                        shift = 1;
                    }
                }
            }

            // Perform differential decoding
            diff.work(diff_in, diffin, diff_out);

            // Reconstruct into bytes and write to output file
            if (diffin > 0)
            {

                inFrameBuffer = 0;
                // Reconstruct into bytes and write to output file
                for (int i = 0; i < diffin / 4; i++)
                {
                    frameBuffer[inFrameBuffer++] = diff_out[i * 4] << 6 | diff_out[i * 4 + 1] << 4 | diff_out[i * 4 + 2] << 2 | diff_out[i * 4 + 3];
                }

                // Deframe / derand
                std::vector<std::array<uint8_t, CADU_SIZE>> frames = deframer.work(frameBuffer, inFrameBuffer);

                if (frames.size() > 0)
                {
                    for (std::array<uint8_t, CADU_SIZE> cadu : frames)
                    {
                        // RS Decoding
                        int errors = 0;
                        for (int i = 0; i < 4; i++)
                        {
                            reedSolomon.deinterleave(&cadu[4], rsWorkBuffer, i, 4);
                            errors = reedSolomon.decode_ccsds(rsWorkBuffer);
                            reedSolomon.interleave(rsWorkBuffer, &cadu[4], i, 4);
                        }

                        // Write it out
                        //data_out_total += CADU_SIZE;
                        data_out.write((char *)&cadu, CADU_SIZE);
                    }
                }
            }
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string viterbi1_state = viterbi1.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string viterbi2_state = viterbi2.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi 1 : " + viterbi1_state + ", Viterbi 2 : " + viterbi2_state + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    std::string FengyunAHRPTDecoderModule::getID()
    {
        return "fengyun_ahrpt_decoder";
    }

    std::vector<std::string> FengyunAHRPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols", "invert_second_viterbi"};
    }

    std::shared_ptr<ProcessingModule> FengyunAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<FengyunAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun