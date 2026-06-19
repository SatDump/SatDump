#pragma once

#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "common/ccsds/ccsds_aos/demuxer.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "common/net/udp.h"
#include "core/exception.h"
#include "logger.h"
#include "pipeline/module.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pipeline/modules/instrument_utils.h"

namespace apid
{
    struct ApidConfig
    {
        int apid;
        int port;
        int packet_size; // -1 means use pkt.payload.size()
    };

    class APIDSenderModule : public satdump::pipeline::base::FileStreamToFileStreamModule
    {
    protected:
        std::string addr;
        bool use_fecf;
        int cadu_size;
        int mpdu_data_size;

        bool save_to_cadu;

        std::vector<ApidConfig> apid_configs;

    public:
        APIDSenderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        void process();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static nlohmann::json getParams() { return {}; };
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
} // namespace apid
