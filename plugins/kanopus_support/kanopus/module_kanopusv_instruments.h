#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"
#include "pmss/pmss_ccd_frame.h"
#include "pmss/pmss_proc.h"

namespace kanopus
{
    class KanopusVInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        // OCMReader ocm_reader;
        // instrument_status_t ocm_status = DECODING;

        int global_counter;
        bool counter_locked = false;

        pmss::CCDFrameProcessor frame_proc;
        pmss::PMSSProcessor pmss_proc;

    public:
        KanopusVInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; } // TODOREWORK
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace kanopus
