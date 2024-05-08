#include "mersi_offset_interleaved.h"
#include "logger.h"
#include <cstring>

namespace fengyun3
{
    namespace mersi
    {
        void mersi_offset_interleaved(image::Image &img, int /*ndet*/, int shift)
        {
            std::vector<int> line_buffer(img.width());
            for (int y = 0; y < (int)img.height(); y += 2)
            {
                for (int x = 0; x < (int)img.width(); x++)
                    line_buffer[x] = img.get(y * img.width() + x);

                for (int x = 0; x < (int)img.width(); x++)
                    if (x + shift >= 0 && x + shift < (int)img.width())
                        img.set(y * img.width() + x, line_buffer[x + shift]);
            }
        }
    }
}