#include "pmr_reader.h"
#include "logger.h"
#include "common/image2/io.h"

namespace fengyun3
{
    namespace pmr
    {
        PMRReader::PMRReader(std::string directory) : directory(directory)
        {
            image.init(16, 645, 58 + 1, 1);
        }

        PMRReader::~PMRReader()
        {
        }

        void PMRReader::writeCurrent()
        {
            // image.equalize();
            // image.normalize();
            image2::save_img(image, std::string(directory + "/PMR_" + std::to_string(images_count++ + 1)).c_str());
            image.fill(0);
        }

        void PMRReader::work(std::vector<uint8_t> &packet)
        {
            int cnt = packet[11];
            // int cnt2 = packet[12] << 8 | packet[13];
            uint8_t marker = packet[5];

            if (cnt > 58 || !(marker == 90 || marker == 238))
                return; // Make sure we don't go out of bounds due to an invalid counter

            // logger->critical("CNT %d CNT2 %d MARKER %d", cnt, cnt2, marker);

            int off = 0;
            if (marker == 238) // Start
            {
                // Save old
                writeCurrent();

                off = 1278 + offset;
                cnt = 0;
            }
            else // Middle
            {
                off = 74 + offset;
            }

            for (int i = 0; i < 645; i++)
            {
                uint32_t v = packet[off + i * 3 + 0] << 16 | packet[off + i * 3 + 1] << 8 | packet[off + i * 3 + 2];

                v <<= 4;

                if (v > UINT16_MAX)
                    v = UINT16_MAX;

                image.set(cnt * 645 + i, v);
            }
        }
    } // namespace virr
} // namespace fengyun