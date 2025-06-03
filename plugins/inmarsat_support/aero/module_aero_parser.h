#pragma once

#include "common/net/udp.h"

#include "acars_parser.h"
#include "decode_utils.h"
#include "pipeline/modules/base/filestream_to_filestream.h"
#include "pkt_structs.h"

namespace inmarsat
{
    namespace aero
    {
        class AeroParserModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            uint8_t *buffer;

            bool is_c_channel = false;

            std::mutex pkt_history_mtx;
            std::vector<nlohmann::json> pkt_history;
            std::vector<nlohmann::json> pkt_history_acars;

            void process_final_pkt(nlohmann::json &msg);

            bool do_save_files;
            std::vector<std::shared_ptr<net::UDPClient>> udp_clients;
            std::string d_station_id = "";

            bool is_gui = false;

            bool has_wip_user_data = false;
            pkts::MessageUserDataFinal wip_user_data;
            acars::ACARSParser acars_parser;
            void process_pkt();

            bool enable_audio = false;
            bool play_audio;

        public:
            AeroParserModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~AeroParserModule();
            void process();
            void drawUI(bool window);

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace aero
} // namespace inmarsat