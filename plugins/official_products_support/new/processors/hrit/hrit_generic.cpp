#include "hrit_generic.h"
#include "common/lrit/lrit_file.h"

#include "logger.h"
#include "xrit/gk2a/decomp.h"
#include "xrit/goes/goes_headers.h"
#include "xrit/identify.h"
#include "xrit/msg/decomp.h"
#include "xrit/msg/msg_headers.h"

#include "image/io.h"
#include <memory>
#include <string>

namespace satdump
{
    namespace official
    {
        void HRITGenericProcessor::ingestFile(std::vector<uint8_t> vec)
        {
            ::lrit::LRITFile file;
            file.lrit_data = vec;
            file.parseHeaders();

            xrit::XRITFileInfo finfo = xrit::identifyXRITFIle(file);

            // Check if this is image data
            if (file.hasHeader<::lrit::ImageStructureRecord>())
            {
                if (finfo.type == xrit::XRIT_ELEKTRO_MSUGS || finfo.type == xrit::XRIT_MSG_SEVIRI)
                    xrit::msg::decompressMsgHritFileIfRequired(file);
                if (finfo.type == xrit::XRIT_GK2A_AMI)
                    xrit::gk2a::decompressGK2AHritFileIfRequired(file);
            }

            processor_test.push(finfo, file);
        }

        std::shared_ptr<satdump::products::Product> HRITGenericProcessor::getProduct()
        {
            processor_test.flush();

            if (processor_test.in_memory_products.size())
                return processor_test.in_memory_products.begin()->second;
            else
                return nullptr;
        }
    } // namespace official
} // namespace satdump