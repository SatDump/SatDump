#pragma once

#include "common/widgets/constellation.h"
#include "pipeline/module.h"
#include <fstream>

namespace meteor
{
    using namespace satdump::pipeline; // TODOREWORK

    class MeteorQPSKKmssDecoderModule : public ProcessingModule
    {
    protected:
        int8_t *soft_buffer;
        uint8_t *bit_buffer;
        uint8_t *bit_buffer_2;

        uint8_t *diff_buffer_1;
        uint8_t *diff_buffer_2;

        uint8_t *rfrm_buffer_1, *rfrm_buffer_2;
        uint8_t *rpkt_buffer_1, *rpkt_buffer_2;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        int frame_count = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        MeteorQPSKKmssDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~MeteorQPSKKmssDecoderModule();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace meteor