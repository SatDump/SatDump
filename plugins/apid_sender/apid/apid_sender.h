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

#include <atomic>
#include <chrono>

namespace apid
{
    struct ApidConfig
    {
        int apid;
        int port;
        int packet_size; // -1 means use pkt.payload.size()

        std::atomic<uint64_t> total_pkts{0};
        uint64_t history_pkts[10] = {0};
        int history_idx = 0;
        std::chrono::steady_clock::time_point last_history_update = std::chrono::steady_clock::now();

        ApidConfig() = default;
        ApidConfig(const ApidConfig& other) {
            apid = other.apid;
            port = other.port;
            packet_size = other.packet_size;
            total_pkts.store(other.total_pkts.load());
            last_history_update = std::chrono::steady_clock::now();
        }
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
