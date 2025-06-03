#pragma once

#include "pipeline/module.h"
#include "utils/file.h"
#include <cstdint>
#include <fstream>

namespace satdump
{
    namespace pipeline
    {
        namespace base
        {
            /*
            This class is meant to handle all common basics shared between demodulators,
            such as file input, DC Blocking, resampling if the symbolrate is out of bounds
            and so on.

            This is not meant to be used by itself, and would serve no purpose anyway
            */
            class FileStreamToFileStreamModule : public ProcessingModule
            {
            public:
                FileStreamToFileStreamModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~FileStreamToFileStreamModule() {}

                std::vector<ModuleDataType> getInputTypes() { return {DATA_FILE, DATA_STREAM}; }
                std::vector<ModuleDataType> getOutputTypes()
                {
                    if (fsfsm_enable_output)
                        return {DATA_FILE, DATA_STREAM};
                    else
                        return {DATA_FILE};
                }

            private:
                std::ifstream data_in;
                std::ofstream data_out;

                uint64_t filesize;
                uint64_t progress;

            private: // TODOREWORK REMOVE!
                time_t time_ = 0;

            protected:
                std::string fsfsm_file_ext = ".bin";
                bool fsfsm_enable_output = true;

            protected:
                void init();

                nlohmann::json getModuleStats();

            protected:
                void read_data(uint8_t *buffer, size_t size);

                void write_data(uint8_t *buffer, size_t size);

                bool should_run();

                void cleanup();

                void drawProgressBar();
            };
        } // namespace base
    } // namespace pipeline
} // namespace satdump