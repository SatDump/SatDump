#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"

namespace goes
{
    namespace grb
    {
        class GOESGRBCADUextractor : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            uint8_t *bb_buffer, *cadu_buffer;
            int cadu_cor = 0;
            bool cadu_sync = 0;
            int best_cor = 0;

            // UI Stuff
            float cor_history_ca[200];

        public:
            GOESGRBCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GOESGRBCADUextractor();
            void process();
            void drawUI(bool window);
            nlohmann::json getModuleStats();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace grb
} // namespace goes