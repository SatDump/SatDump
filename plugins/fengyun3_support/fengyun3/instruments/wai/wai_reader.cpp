#include "wai_reader.h"
#include "logger.h"
#include "common/image2/io/io.h"

namespace fengyun3
{
    namespace wai
    {
        WAIReader::WAIReader(std::string directory) : directory(directory)
        {
            lines = 0;
            image = image2::Image(16, 832, 832, 1);
        }

        WAIReader::~WAIReader()
        {
        }

        void WAIReader::writeCurrent()
        {
            image2::save_img(image, std::string(directory + "/WAI_" + std::to_string(images_count++ + 1)).c_str());
            image = image2::Image(16, 832, 832, 1);

            lines = 0;
        }

        void WAIReader::work(std::vector<uint8_t> &packet)
        {
            uint8_t marker = packet[10] >> 6;

            if (marker == 2)
            {
                if (lines + 3603 < 832 * 832)
                    for (int i = 0; i < 3603; i++)
                        image.set(lines + i, packet[68 + i * 2 + 0] << 8 | packet[68 + i * 2 + 1]);
                lines += 3603;
            }
            else if (marker == 1) // Start
            {
                // Save old
                if (lines > 0)
                    writeCurrent();

                if (lines + 32591 < 832 * 832)
                    for (int i = 0; i < 32591; i++)
                        image.set(lines + i, packet[320 + i * 2 + 0] << 8 | packet[320 + i * 2 + 1]);
                lines += 32591;
            }
            else
            {
                if (lines + 32737 < 832 * 832)
                    for (int i = 0; i < 32737; i++)
                        image.set(lines + i, packet[68 + i * 2 + 0] << 8 | packet[68 + i * 2 + 1]);
                lines += 32737;
            }
        }
    } // namespace virr
} // namespace fengyun