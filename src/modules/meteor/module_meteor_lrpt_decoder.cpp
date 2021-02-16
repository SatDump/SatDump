#include "module_meteor_lrpt_decoder.h"
#include <fstream>
#include "logger.h"
#include "modules/common/reedsolomon.h"
#include "modules/common/correlator.h"
#include "modules/common/packetfixer.h"
#include "modules/common/viterbi27.h"
#include "modules/common/derandomizer.h"
#include "modules/aqua/differentialencoding.h"

#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    METEORLRPTDecoderModule::METEORLRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                            diff_decode(std::stoi(parameters["diff_decode"]))
    {
    }

    void METEORLRPTDecoderModule::process()
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

        if (diff_decode)
        {
            // All differentially encoded sync words
            correlator.addWord((uint64_t)0xfc4ef4fd0cc2df89);
            correlator.addWord((uint64_t)0x56275254a66b45ec);
            correlator.addWord((uint64_t)0x03b10b02f33d2076);
            correlator.addWord((uint64_t)0xa9d8adab5994ba89);

            correlator.addWord((uint64_t)0xfc8df8fe0cc1ef46);
            correlator.addWord((uint64_t)0xa91ba1a859978adc);
            correlator.addWord((uint64_t)0x03720701f33e1089);
            correlator.addWord((uint64_t)0x56e45e57a6687546);
        }
        else
        {
            // All  encoded sync words
            correlator.addWord((uint64_t)0xfca2b63db00d9794);
            correlator.addWord((uint64_t)0x56fbd394daa4c1c2);
            correlator.addWord((uint64_t)0x035d49c24ff2686b);
            correlator.addWord((uint64_t)0xa9042c6b255b3e3d);

            correlator.addWord((uint64_t)0xfc51793e700e6b68);
            correlator.addWord((uint64_t)0xa9f7e368e558c2c1);
            correlator.addWord((uint64_t)0x03ae86c18ff19497);
            correlator.addWord((uint64_t)0x56081c971aa73d3e);
        }

        // Viterbi, rs, etc
        SatHelper::PacketFixer packetFixer;
        SatHelper::PhaseShift phaseShift;
        SatHelper::Viterbi27 viterbi(ENCODED_FRAME_SIZE / 2);
        SatHelper::DeRandomizer derand;
        SatHelper::ReedSolomon reedSolomon;
        SatHelper::DifferentialEncoding diff;
        bool iqinv;

        // Other buffers
        uint8_t frameBuffer[FRAME_SIZE];
        uint8_t buffer[ENCODED_FRAME_SIZE];

        // Lock mechanism stuff
        bool locked = false;
        int errors[4] = {-1, -1, -1, -1};

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
                iqinv = (word / 4) > 0;
                switch (word % 4)
                {
                case 0:
                    phaseShift = SatHelper::PhaseShift::DEG_0;
                    break;

                case 1:
                    phaseShift = SatHelper::PhaseShift::DEG_90;
                    break;

                case 2:
                    phaseShift = SatHelper::PhaseShift::DEG_180;
                    break;

                case 3:
                    phaseShift = SatHelper::PhaseShift::DEG_270;
                    break;

                default:
                    break;
                }

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
                else
                {
                }

                // Correct phase ambiguity
                packetFixer.fixPacket(buffer, ENCODED_FRAME_SIZE, phaseShift, iqinv);

                // Viterbi
                viterbi.decode(buffer, frameBuffer);

                if (diff_decode)
                    diff.nrzmDecode(frameBuffer, FRAME_SIZE);

                // Derandomize that frame
                derand.DeRandomize(&frameBuffer[4], FRAME_SIZE - 4);

                // RS Correction
                for (int i = 0; i < 4; i++)
                {
                    reedSolomon.deinterleave(&frameBuffer[4], rsWorkBuffer, i, 5);
                    errors[i] = reedSolomon.decode_rs8(rsWorkBuffer);
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

    void METEORLRPTDecoderModule::shiftWithConstantSize(uint8_t *arr, int pos, int length)
    {
        for (int i = 0; i < length - pos; i++)
        {
            arr[i] = arr[pos + i];
        }
    }

    std::string METEORLRPTDecoderModule::getID()
    {
        return "meteor_lrpt_decoder";
    }

    std::vector<std::string> METEORLRPTDecoderModule::getParameters()
    {
        return {"diff_decode"};
    }

    std::shared_ptr<ProcessingModule> METEORLRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<METEORLRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor