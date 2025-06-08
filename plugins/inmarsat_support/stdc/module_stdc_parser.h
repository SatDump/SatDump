#pragma once

#include "common/net/udp.h"
#include "decode_utils.h"
#include "pipeline/modules/base/filestream_to_filestream.h"

namespace inmarsat
{
    namespace stdc
    {
        class STDCParserModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            uint8_t *buffer;

            std::mutex pkt_history_mtx;
            std::vector<nlohmann::json> pkt_history;
            std::vector<nlohmann::json> pkt_history_msg;
            std::vector<nlohmann::json> pkt_history_egc;

            void process_final_pkt(nlohmann::json &msg);

            int last_pkt_count = 0;

            bool do_save_files;
            std::vector<std::shared_ptr<net::UDPClient>> udp_clients;
            std::string d_station_id = "";

            bool is_gui = false;

        public:
            STDCParserModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~STDCParserModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace stdc
} // namespace inmarsat