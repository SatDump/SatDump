#pragma once

#include <string>
#include "common/ccsds/ccsds.h"

#define MAX_MSG_SIZE 1000000

namespace metop
{
    namespace admin_msg
    {
        class AdminMsgReader
        {
        private:
            uint8_t *message_out;

        public:
            int count;
            std::string directory = "";

        public:
            AdminMsgReader();
            ~AdminMsgReader();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace modis
} // namespace eos