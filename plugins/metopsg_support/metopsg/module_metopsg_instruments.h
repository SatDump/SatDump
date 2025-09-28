#pragma once

#include "instruments/mws/mws_reader.h"
#include "metopsg/instruments/3mi/3mi_reader.h"
#include "metopsg/instruments/admin_msg/admin_msg_reader.h"
#include "metopsg/instruments/metimage/metimage_reader.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace metopsg
{
    namespace instruments
    {
        class MetOpSGInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            bool ignore_integrated_tle = false;

            // Readers
            mws::MWSReader mws_reader;
            threemi::ThreeMIReader threemi_reader;
            admin_msg::AdminMsgReader admin_msg_reader;
            metimage::METImageReader metimage_reader;

            // Statuses
            instrument_status_t mws_status = DECODING;
            instrument_status_t threemi_status = DECODING;
            instrument_status_t admin_msg_status = DECODING;
            instrument_status_t metimage_status = DECODING;

        public:
            MetOpSGInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams()
            {
                nlohmann::json v; // TODOREWORk
                v["ignore_integrated_tle"] = false;
                return v;
            }
            static std::shared_ptr<satdump::pipeline::ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace instruments
} // namespace metopsg