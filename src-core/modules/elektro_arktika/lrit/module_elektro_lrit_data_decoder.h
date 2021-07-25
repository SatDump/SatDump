#pragma once

#include "module.h"
#include "data/lrit_data_decoder.h"

namespace elektro
{
    namespace lrit
    {
        class ELEKTROLRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            std::map<int, std::shared_ptr<LRITDataDecoder>> decoders;

            std::shared_ptr<ELEKTRO221Composer> elektro_221_composer_full_disk;
             std::shared_ptr<ELEKTRO321Composer> elektro_321_composer_full_disk;

        public:
            ELEKTROLRITDataDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            ~ELEKTROLRITDataDecoderModule();
            void process();
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace avhrr
} // namespace metop