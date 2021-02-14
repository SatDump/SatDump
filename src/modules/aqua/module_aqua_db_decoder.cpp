#include "module_aqua_db_decoder.h"
#include <fstream>
#include "logger.h"
#include "modules/common/deframer.h"
#include "modules/common/reedsolomon.h"
#include "differentialencoding.h"

#define BUFFER_SIZE (1024 * 8 * 8)

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

AquaDBDecoderModule::AquaDBDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
{
}

void AquaDBDecoderModule::process()
{
    size_t filesize = getFilesize(d_input_file);
    std::ifstream data_in(d_input_file, std::ios::binary);
    std::ofstream data_out(d_output_file_hint + ".cadu", std::ios::binary);
    d_output_files.push_back(d_output_file_hint + ".cadu");

    logger->info("Using input symbols " + d_input_file);
    logger->info("Decoding to " + d_output_file_hint + ".cadu");

    time_t lastTime = 0;

    SatHelper::DifferentialEncoding diff;
    SatHelper::ReedSolomon reedSolomon;
    CADUDeframer deframer;

    // Read buffer
    uint8_t buffer[BUFFER_SIZE];

    // I/Q Buffers
    uint8_t decodedBufI[BUFFER_SIZE / 2];
    uint8_t decodedBufQ[BUFFER_SIZE / 2];
    uint8_t bufI[(BUFFER_SIZE / 8) / 2 + 1];
    uint8_t bufQ[(BUFFER_SIZE / 8) / 2 + 1];

    // Final buffer after decoding
    uint8_t finalBuffer[(BUFFER_SIZE / 8)];

    // Bits => Bytes stuff
    uint8_t byteShifter;
    int inByteShifter = 0;
    int inFinalByteBuffer;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)buffer, BUFFER_SIZE);

        // Demodulate QPSK... This is the crappy way but it works
        for (int i = 0; i < BUFFER_SIZE / 2; i++)
        {
            int8_t sample_i = clamp(*((int8_t *)&buffer[i * 2]));
            int8_t sample_q = clamp(*((int8_t *)&buffer[i * 2 + 1]));

            if (sample_i == -1 && sample_q == -1)
            {
                decodedBufQ[i] = 0;
                decodedBufI[i] = 0;
            }
            else if (sample_i == 1 && sample_q == -1)
            {
                decodedBufQ[i] = 0;
                decodedBufI[i] = 1;
            }
            else if (sample_i == 1 && sample_q == 1)
            {
                decodedBufQ[i] = 1;
                decodedBufI[i] = 1;
            }
            else if (sample_i == -1 && sample_q == 1)
            {
                decodedBufQ[i] = 1;
                decodedBufI[i] = 0;
            }
        }

        // Group symbols into bytes now, I channel
        inFinalByteBuffer = 1;
        inByteShifter = 0;
        bufI[0] = bufI[512]; // Reuse some of last run's data
        for (int i = 0; i < BUFFER_SIZE / 2; i++)
        {
            byteShifter = byteShifter << 1 | decodedBufI[i];
            inByteShifter++;

            if (inByteShifter == 8)
            {
                bufI[inFinalByteBuffer++] = byteShifter;
                inByteShifter = 0;
            }
        }

        // Group symbols into bytes now, Q channel
        inFinalByteBuffer = 1;
        inByteShifter = 0;
        bufQ[0] = bufQ[512]; // Reuse some of last run's data
        for (int i = 0; i < BUFFER_SIZE / 2; i++)
        {
            byteShifter = byteShifter << 1 | decodedBufQ[i];
            inByteShifter++;

            if (inByteShifter == 8)
            {
                bufQ[inFinalByteBuffer++] = byteShifter;
                inByteShifter = 0;
            }
        }

        // Differential decoding for both of them
        diff.nrzmDecode(bufI, (BUFFER_SIZE / 8) / 2 + 1);
        diff.nrzmDecode(bufQ, (BUFFER_SIZE / 8) / 2 + 1);

        // Interleave them back
        for (int i = 0; i < (BUFFER_SIZE / 8) / 2; i++)
        {
            finalBuffer[i * 2] = getBit<uint8_t>(bufI[i + 1], 7) << 7 |
                                 getBit<uint8_t>(bufQ[i + 1], 7) << 6 |
                                 getBit<uint8_t>(bufI[i + 1], 6) << 5 |
                                 getBit<uint8_t>(bufQ[i + 1], 6) << 4 |
                                 getBit<uint8_t>(bufI[i + 1], 5) << 3 |
                                 getBit<uint8_t>(bufQ[i + 1], 5) << 2 |
                                 getBit<uint8_t>(bufI[i + 1], 4) << 1 |
                                 getBit<uint8_t>(bufQ[i + 1], 4) << 0;
            finalBuffer[i * 2 + 1] = getBit<uint8_t>(bufI[i + 1], 3) << 7 |
                                     getBit<uint8_t>(bufQ[i + 1], 3) << 6 |
                                     getBit<uint8_t>(bufI[i + 1], 2) << 5 |
                                     getBit<uint8_t>(bufQ[i + 1], 2) << 4 |
                                     getBit<uint8_t>(bufI[i + 1], 1) << 3 |
                                     getBit<uint8_t>(bufQ[i + 1], 1) << 2 |
                                     getBit<uint8_t>(bufI[i + 1], 0) << 1 |
                                     getBit<uint8_t>(bufQ[i + 1], 0) << 0;
        }

        // Deframe that! (Integrated derand)
        std::vector<std::array<uint8_t, CADU_SIZE>> frameBuffer = deframer.work(finalBuffer, (BUFFER_SIZE / 8));

        // If we found frames, write them out
        if (frameBuffer.size() > 0)
        {
            for (std::array<uint8_t, CADU_SIZE> cadu : frameBuffer)
            {
                // RS Correction
                int errors = 0;
                for (int i = 0; i < 4; i++)
                {
                    reedSolomon.deinterleave(&cadu[4], rsWorkBuffer, i, 4);
                    errors += reedSolomon.decode_ccsds(rsWorkBuffer);
                    reedSolomon.interleave(rsWorkBuffer, &cadu[4], i, 4);
                }

                // Write it to our output file! But only corrected frames...
                //if (errors > -4)
                //{
                //data_out_total += CADU_SIZE;
                data_out.write((char *)&cadu, CADU_SIZE);
                //}
            }
        }

        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
            logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state);
        }
    }

    data_out.close();
    data_in.close();
}

std::string AquaDBDecoderModule::getID()
{
    return "aqua_db_decoder";
}

std::vector<std::string> AquaDBDecoderModule::getParameters()
{
    return {};
}

std::shared_ptr<ProcessingModule> AquaDBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<AquaDBDecoderModule>(input_file, output_file_hint, parameters);
}