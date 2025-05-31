#pragma once

#include "common/codings/viterbi/viterbi27.h"
#include "decode_utils.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include <fstream>

namespace inmarsat
{
    namespace stdc
    {
        class STDCDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            int8_t *buffer;
            int8_t *buffer_shifter;
            int8_t *buffer_synchronized;
            int8_t *buffer_depermuted;
            uint8_t *buffer_vitdecoded;

            viterbi::Viterbi27 viterbi;

            // UI Stuff
            float ber_history[200];
            float cor_history[200];

            int cor = 0;
            bool gotFrame = false;

            uint16_t frm_num = 0;

        public:
            STDCDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~STDCDecoderModule();
            void process();
            void drawUI(bool window);
            nlohmann::json getModuleStats();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace stdc
} // namespace inmarsat