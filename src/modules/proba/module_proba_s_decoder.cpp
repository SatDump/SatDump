#include "module_proba_s_decoder.h"
#include <fstream>
#include "logger.h"
#include "modules/common/reedsolomon.h"
#include "modules/common/correlator.h"
#include "modules/common/packetfixer.h"
#include "modules/common/viterbi27.h"
#include "modules/common/derandomizer.h"

#define FRAME_SIZE 1279
#define ENCODED_FRAME_SIZE 1279 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace proba
{
    ProbaSDecoderModule::ProbaSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                    derandomize(std::stoi(parameters["derandomize"]))
    {
    }

    void ProbaSDecoderModule::process()
    {
        size_t filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);
        std::ofstream data_out(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        // Correlator
        SatHelper::Correlator correlator;

        // All encoded sync words. Though PROBAs are BPSK. So we only have 2!
        // Weirdly enough... IQ inversed and always 90/270 deg phase shift weirdness? Well it works so whatever I guess!
        correlator.addWord((uint64_t)0xa9f7e368e558c2c1);
        correlator.addWord((uint64_t)0x56081c971aa73d3e);

        // Viterbi, rs, etc
        SatHelper::PacketFixer packetFixer;
        SatHelper::Viterbi27 viterbi(ENCODED_FRAME_SIZE / 2);
        SatHelper::DeRandomizer derand;
        SatHelper::ReedSolomon reedSolomon;

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];
        uint8_t buffer[ENCODED_FRAME_SIZE];

        // Lock mechanism stuff
        bool locked = false;
        int errors[5] = {-1, -1, -1, -1, -1};

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, ENCODED_FRAME_SIZE);

            // Correlate less if we're locked to go faster
            if (!locked)
                correlator.correlate(buffer, ENCODED_FRAME_SIZE);
            {
                correlator.correlate(buffer, ENCODED_FRAME_SIZE / 64);
                if (correlator.getHighestCorrelationPosition() != 0)
                {
                    correlator.correlate(buffer, ENCODED_FRAME_SIZE);
                    if (correlator.getHighestCorrelationPosition() > 30)
                        locked = false;
                }
            }

            // Correlator statistics
            uint32_t cor = correlator.getHighestCorrelation();
            uint32_t word = correlator.getCorrelationWordNumber();
            uint32_t pos = correlator.getHighestCorrelationPosition();

            if (cor > 10)
            {
                if (pos != 0)
                {
                    shiftWithConstantSize(buffer, pos, ENCODED_FRAME_SIZE);
                    uint32_t offset = ENCODED_FRAME_SIZE - pos;
                    uint8_t buffer_2[pos];

                    data_in.read((char *)buffer_2, pos);

                    for (int i = offset; i < ENCODED_FRAME_SIZE; i++)
                    {
                        buffer[i] = buffer_2[i - offset];
                    }
                }

                // Correct phase ambiguity
                packetFixer.fixPacket(buffer, ENCODED_FRAME_SIZE, word == 0 ? SatHelper::PhaseShift::DEG_90 : SatHelper::PhaseShift::DEG_270, true);

                // Viterbi
                viterbi.decode(buffer, frameBuffer);

                if (derandomize)
                {
                    // Derandomize that frame
                    derand.DeRandomize(&frameBuffer[4], FRAME_SIZE - 4);
                }

                // RS Correction
                for (int i = 0; i < 5; i++)
                {
                    reedSolomon.deinterleave(&frameBuffer[4], rsWorkBuffer, i, 5);
                    errors[i] = reedSolomon.decode_ccsds(rsWorkBuffer);
                    reedSolomon.interleave(rsWorkBuffer, &frameBuffer[4], i, 5);
                }

                // Write it out if it's not garbage
                if (cor > 50)
                    locked = true;

                if (locked)
                {
                    //data_out_total += FRAME_SIZE;
                    data_out.put(0x1a);
                    data_out.put(0xcf);
                    data_out.put(0xfc);
                    data_out.put(0x1d);
                    data_out.write((char *)&frameBuffer[4], FRAME_SIZE - 4);
                }
            }
            else
            {
                locked = false;
            }

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%, Viterbi BER : " + std::to_string(viterbi.GetPercentBER()) + "%, Lock : " + lock_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    void ProbaSDecoderModule::shiftWithConstantSize(uint8_t *arr, int pos, int length)
    {
        for (int i = 0; i < length - pos; i++)
        {
            arr[i] = arr[pos + i];
        }
    }

    std::string ProbaSDecoderModule::getID()
    {
        return "proba_s_decoder";
    }

    std::vector<std::string> ProbaSDecoderModule::getParameters()
    {
        return {"viterbi_outsync_after", "viterbi_ber_thresold", "soft_symbols"};
    }

    std::shared_ptr<ProcessingModule> ProbaSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<ProbaSDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace proba