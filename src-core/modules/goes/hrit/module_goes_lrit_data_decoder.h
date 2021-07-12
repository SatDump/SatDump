#pragma once

#include "module.h"
#include "data/lrit_data_decoder.h"

namespace goes
{
    namespace hrit
    {
        class GOESLRITDataDecoderModule : public ProcessingModule
        {
        protected:
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            bool write_images;
            bool write_emwin;
            bool write_messages;
            bool write_dcs;
            bool write_unknown;

            std::map<int, std::shared_ptr<LRITDataDecoder>> decoders;

        private:
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_full_disk;
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_meso1;
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_meso2;

        public:
            GOESLRITDataDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
            ~GOESLRITDataDecoderModule();
            void process();
            void drawUI(bool window);
            std::vector<ModuleDataType> getInputTypes();
            std::vector<ModuleDataType> getOutputTypes();

        public:
            static std::string getID();
            static std::vector<std::string> getParameters();
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        };
    } // namespace avhrr
} // namespace metop