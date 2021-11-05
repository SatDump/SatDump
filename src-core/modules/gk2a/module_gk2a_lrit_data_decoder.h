#pragma once

#include "module.h"
#include "data/lrit_data_decoder.h"

namespace gk2a
{
    namespace lrit
    {
        class GK2ALRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            std::map<int, std::shared_ptr<LRITDataDecoder>> decoders;

            bool write_images;
            bool write_additional;
            bool write_unknown;

        public:
            GK2ALRITDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GK2ALRITDataDecoderModule();
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
    } // namespace avhrr
} // namespace metop