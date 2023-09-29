#include "mersi_histmatch.h"
#include "common/image/histogram_utils.h"
#include "logger.h"
#include <cstring>

namespace fengyun3
{
    namespace mersi
    {
        void mersi_offset_interleaved(image::Image<uint16_t> &img, int /*ndet*/, int shift)
        {
            std::vector<uint16_t> line_buffer(img.width());
            for (int y = 0; y < (int)img.height(); y += 2)
            {
                memcpy(line_buffer.data(), &img[y * img.width()], img.width() * sizeof(uint16_t));
                for (int x = 0; x < (int)img.width(); x++)
                    if (x + shift >= 0 && x + shift < (int)img.width())
                        img[y * img.width() + x] = line_buffer[x + shift];
            }
        }
    }
}