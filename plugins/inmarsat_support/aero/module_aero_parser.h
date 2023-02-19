#pragma once

#include "core/module.h"
#include "decode_utils.h"
#include <fstream>
#include "common/net/udp.h"
#include "pkt_structs.h"
#include "acars_parser.h"

namespace inmarsat
{
    namespace aero
    {
        class AeroParserModule : public ProcessingModule
        {
        protected:
            uint8_t *buffer;

            bool is_c_channel = false;

            std::ifstream data_in;
            std::atomic<size_t> filesize;
            std::atomic<size_t> progress;

            std::mutex pkt_history_mtx;
            std::vector<nlohmann::json> pkt_history;
            std::vector<nlohmann::json> pkt_history_acars;

            void process_final_pkt(nlohmann::json &msg);

            bool do_save_files;
            std::vector<std::shared_ptr<net::UDPClient>> udp_clients;

            bool is_gui = false;

            bool has_wip_user_data = false;
            pkts::MessageUserDataFinal wip_user_data;
            acars::ACARSParser acars_parser;
            void process_pkt();

            bool enable_audio = false;

        public:
            AeroParserModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~AeroParserModule();
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
    }
}