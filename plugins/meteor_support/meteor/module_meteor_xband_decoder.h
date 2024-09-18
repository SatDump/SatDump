#pragma once

#include "core/module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "deframer.h"
#include "common/widgets/constellation.h"
#include "meteor_xband_types.h"

namespace meteor
{
    class MeteorXBandDecoderModule : public ProcessingModule
    {
    protected:
        dump_instrument_type_t d_instrument_mode;

        int8_t *soft_buffer;
        uint8_t *bit_buffer;
        uint8_t *rfrm_buffer;
        uint8_t *rpkt_buffer;

        std::ifstream data_in;
        std::ofstream data_out;

        std::atomic<uint64_t> filesize;
        std::atomic<uint64_t> progress;

        int frame_count = 0;

        // UI Stuff
        widgets::ConstellationViewer constellation;

    public:
        MeteorXBandDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~MeteorXBandDecoderModule();
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
} // namespace meteor