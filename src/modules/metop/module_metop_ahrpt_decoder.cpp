#include "module_metop_ahrpt_decoder.h"
#include <fstream>
#include "logger.h"
#include "modules/common/deframer.h"
#include "modules/common/reedsolomon.h"
#include "viterbi.h"

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

namespace metop
{
    MetOpAHRPTDecoderModule::MetOpAHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            d_viterbi_outsync_after(std::stoi(parameters["viterbi_outsync_after"])),
                                                                                                                                                            d_viterbi_ber_threasold(std::stof(parameters["viterbi_ber_thresold"])),
                                                                                                                                                            d_soft_symbols(std::stoi(parameters["soft_symbols"])),
                                                                                                                                                            sw(0)
    {
        viterbi_out = new uint8_t[BUFFER_SIZE];
        sym_buffer = new std::complex<float>[BUFFER_SIZE];
        soft_buffer = new int8_t[BUFFER_SIZE * 2];
    }

    void MetOpAHRPTDecoderModule::process()
    {
        size_t filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);
        std::ofstream data_out(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        MetopViterbi viterbi(true, d_viterbi_ber_threasold, 1, d_viterbi_outsync_after, 50);
        ;
        SatHelper::ReedSolomon reedSolomon;
        CADUDeframer deframer;

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

            // Push into Viterbi
            int num_samp = viterbi.work(sym_buffer, BUFFER_SIZE, viterbi_out);

            // Reconstruct into bytes and write to output file
            if (num_samp > 0)
            {
                // Deframe / derand
                std::vector<std::array<uint8_t, CADU_SIZE>> frames = deframer.work(viterbi_out, num_samp);

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
                std::string viterbi_state = viterbi.getState() == 0 ? "NOSYNC" : "SYNCED";
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi : " + viterbi_state + ", Deframer : " + deframer_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    std::string MetOpAHRPTDecoderModule::getID()
    {
        return "metop_ahrpt_decoder";
    }

    std::vector<std::string> MetOpAHRPTDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols"};
    }

    std::shared_ptr<ProcessingModule> MetOpAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<MetOpAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace metop