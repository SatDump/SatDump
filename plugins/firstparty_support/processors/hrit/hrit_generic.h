#pragma once

#include "../processor.h"
#include "xrit/processor/xrit_channel_processor.h"

namespace satdump
{
    namespace firstparty
    {
        class HRITGenericProcessor : public FirstPartyProductProcessor
        {
        private:
            xrit::XRITChannelProcessor processor_test;

        public:
            HRITGenericProcessor() { processor_test.in_memory_mode = true; }
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace firstparty
} // namespace satdump