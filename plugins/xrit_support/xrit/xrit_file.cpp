#include "xrit_file.h"
#include "logger.h"
#include <algorithm>

namespace satdump
{
    namespace xrit
    {
        void XRITFile::parseHeaders(bool safe)
        {
            PrimaryHeader primary_header = getHeader<PrimaryHeader>();

            // Get all other headers
            all_headers.clear();
            for (uint32_t i = 0; i < primary_header.total_header_length;)
            {
                uint8_t type = lrit_data[i];
                uint16_t record_length = lrit_data[i + 1] << 8 | lrit_data[i + 2];

                if (record_length == 0)
                    break;

                if (safe)
                    if (i + record_length >= lrit_data.size())
                        break; //  TODOREWORK ? ? ?

                all_headers.emplace(std::pair<int, int>(type, i));

                i += record_length;
            }

            // Check if this has a filename
            if (all_headers.count(AnnotationRecord::TYPE) > 0)
            {
                AnnotationRecord annotation_record = getHeader<AnnotationRecord>();
                try
                {
                    filename = std::string(annotation_record.annotation_text.data());

                    std::replace(filename.begin(), filename.end(), '/', '_');  // Safety
                    std::replace(filename.begin(), filename.end(), '\\', '_'); // Safety

                    for (char &c : filename) // Strip invalid chars
                    {
                        if (c < 33)
                            c = '_';
                    }
                }
                catch (std::exception &e)
                {
                    filename.clear();
                }
            }

            total_header_length = primary_header.total_header_length;
        }
    } // namespace xrit
} // namespace satdump