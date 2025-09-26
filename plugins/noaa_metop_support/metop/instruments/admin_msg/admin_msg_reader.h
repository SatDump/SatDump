#pragma once

#include "common/ccsds/ccsds.h"
#include "common/tracking/tle.h"
#include "libs/rapidxml.hpp"
#include <string>

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
            std::vector<satdump::TLE> tles;

        public:
            AdminMsgReader();
            ~AdminMsgReader();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace admin_msg
} // namespace metop