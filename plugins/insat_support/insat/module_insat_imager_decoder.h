#pragma once

#include "common/codings/viterbi/viterbi_1_2.h"
#include "common/simple_deframer.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace insat
{
    class INSATImagerDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        viterbi::Viterbi1_2 viterbi;
        def::SimpleDeframer def;
        uint32_t frame_cnt = 0;

    protected:
        float ber_history[200];

    public:
        INSATImagerDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~INSATImagerDecoderModule();
        void process();
        void drawUI(bool window);
        nlohmann::json getModuleStats();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace insat