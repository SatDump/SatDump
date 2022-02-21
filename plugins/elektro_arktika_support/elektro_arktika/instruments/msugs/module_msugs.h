#pragma once

#include "module.h"
#include <fstream>

namespace elektro_arktika
{
    namespace msugs
    {
        class MSUGSDecoderModule : public ProcessingModule
        {
        protected:
            std::ifstream data_in;
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            MSUGSDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace msugs
} // namespace elektro_arktika