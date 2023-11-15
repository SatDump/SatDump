#pragma once

#include "core/module.h"
#include <fstream>
#include "sd_imager_reader.h"

namespace goes
{
    namespace sd
    {
        class SDImageDecoderModule : public ProcessingModule
        {
        protected:
            uint8_t *frame;
            uint16_t *frame_words;

            std::ifstream data_in;
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            SDImagerReader img_reader;

        public:
            SDImageDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~SDImageDecoderModule();
            static std::string getGvarFilename(int sat_number, std::tm *timeReadable, std::string channel);
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