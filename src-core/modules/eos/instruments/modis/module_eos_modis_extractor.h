#pragma once

#include "module.h"

namespace eos
{
    namespace modis
    {
        class EOSMODISExtractorModule : public ProcessingModule
        {
        protected:
            bool terra;

            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

        public:
            EOSMODISExtractorModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace modis
} // namespace eos