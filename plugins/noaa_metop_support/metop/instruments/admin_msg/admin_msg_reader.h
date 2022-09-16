#pragma once

#include <string>
#include "common/ccsds/ccsds.h"
#include "common/tracking/tle.h"
#include "libs/rapidxml.hpp"

#define MAX_MSG_SIZE 1000000

namespace metop
{
    namespace admin_msg
    {
        class AdminMsgReader
        {
        private:
            uint8_t *message_out;

            // XML Parser
            rapidxml::xml_document<> doc;
            rapidxml::xml_node<> *root_node = NULL;

        public:
            int count;
            std::string directory = "";
            satdump::TLERegistry tles;

        public:
            AdminMsgReader();
            ~AdminMsgReader();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace modis
} // namespace eos