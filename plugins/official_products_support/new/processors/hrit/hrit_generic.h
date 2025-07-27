#pragma once

#include "../processor.h"
#include "common/lrit/lrit_productizer.h"
#include "products/image_product.h"
#include "xrit/goes/segment_decoder.h"
#include "xrit/msg/segment_decoder.h"
#include "xrit/processor/xrit_channel_processor.h"

namespace satdump
{
    namespace official
    {
        class HRITGenericProcessor : public OfficialProductProcessor
        {
        private:
            xrit::XRITChannelProcessor processor_test;

        public:
            HRITGenericProcessor() { processor_test.in_memory_mode = true; }
            void ingestFile(std::vector<uint8_t> file);
            std::shared_ptr<satdump::products::Product> getProduct();
        };
    } // namespace official
} // namespace satdump