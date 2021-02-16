#include "module_elektro_rdas_decoder.h"
#include <fstream>
#include "logger.h"
#include "modules/common/deframer.h"
#include "modules/common/reedsolomon.h"
#include "modules/common/differentialencoding.h"

#define BUFFER_SIZE 8192

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace elektro
{
    ElektroRDASDecoderModule::ElektroRDASDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    void ElektroRDASDecoderModule::process()
    {
        size_t filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);
        std::ofstream data_out(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        NRZMDiff diff;
        CADUDeframer deframer;

        // Read buffer
        int8_t buffer[BUFFER_SIZE];

        // Final buffer after decoding
        uint8_t finalBuffer[BUFFER_SIZE];

        // Bits => Bytes stuff
        uint8_t byteShifter;
        int inByteShifter = 0;
        int byteShifted = 0;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            // Group symbols into bytes now, I channel
            inByteShifter = 0;
            byteShifted = 0;

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                byteShifter = byteShifter << 1 | (buffer[i] > 0);
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    finalBuffer[byteShifted++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            // Differential decoding for both of them
            diff.decode(finalBuffer, BUFFER_SIZE / 8);

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, CADU_SIZE>> frameBuffer = deframer.work(finalBuffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, CADU_SIZE> cadu : frameBuffer)
                {
                    data_out.write((char *)&cadu, CADU_SIZE);
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

    std::string ElektroRDASDecoderModule::getID()
    {
        return "elektro_rdas_decoder";
    }

    std::vector<std::string> ElektroRDASDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> ElektroRDASDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<ElektroRDASDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace elektro