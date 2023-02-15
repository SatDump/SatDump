#pragma once

#include "core/module.h"
#include "common/codings/viterbi/viterbi27.h"
#include "decode_utils.h"
#include <fstream>

namespace inmarsat
{
    namespace stdc
    {
        class STDCDecoderModule : public ProcessingModule
        {
        protected:
            int8_t *buffer;
            int8_t *buffer_shifter;
            int8_t *buffer_synchronized;
            int8_t *buffer_depermuted;
            uint8_t *buffer_vitdecoded;

            std::ifstream data_in;
            std::ofstream data_out;
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            viterbi::Viterbi27 viterbi;

            // UI Stuff
            float ber_history[200];
            float cor_history[200];

            int cor = 0;
            bool gotFrame = false;

        public:
            STDCDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~STDCDecoderModule();
            void process();
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    }
}