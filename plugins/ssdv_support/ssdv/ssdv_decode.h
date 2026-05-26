#pragma once

#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/net/udp.h"
#include "core/exception.h"
#include "instruments/ssdv-ng/ssdv.h"
#include "logger.h"
#include "pipeline/module.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace ssdv
{
    // namespace instruments
    // {
    class SSDVInstrumentsDecoderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        ssdvng::SSDVNGReader ssdv_ng_reader;
        instrument_status_t ssdv_ng_status = DECODING;

    protected:
        std::string addr;
        bool use_fecf;
        int port_apid_10;
        int port_apid_20;
        int port_apid_100;

    public:
        SSDVInstrumentsDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; };
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
    // } // namespace instruments
} // namespace ssdv
