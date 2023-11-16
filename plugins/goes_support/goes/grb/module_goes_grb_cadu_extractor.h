#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>
//#include "deframer.h"

namespace goes
{
    namespace grb
    {
        class GOESGRBCADUextractor : public ProcessingModule
        {
        protected:
            uint8_t *bb_buffer, *cadu_buffer;
            int cadu_cor = 0;
            bool cadu_sync = 0;

            std::ifstream data_in;
            std::ofstream data_out;
            std::atomic<uint64_t> filesize;
            std::atomic<uint64_t> progress;

            // UI Stuff
            float cor_history_ca[200];

        public:
            GOESGRBCADUextractor(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GOESGRBCADUextractor();
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
} // namespace aqua