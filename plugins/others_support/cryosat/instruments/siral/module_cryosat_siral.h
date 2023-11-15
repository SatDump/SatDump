#pragma once

#include "core/module.h"

namespace cryosat
{
    namespace siral
    {
        class CryoSatSIRALDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

        public:
            CryoSatSIRALDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace avhrr
} // namespace noaa