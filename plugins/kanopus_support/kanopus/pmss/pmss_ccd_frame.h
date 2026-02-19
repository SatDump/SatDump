#pragma once

#include "image/image.h"
#include "pmss_segment.h"
#include "utils/majority_law.h"
#include <cstdint>
#include <cstring>
#include <memory>

namespace kanopus
{
    namespace pmss
    {
        class CCDFrameProcessor
        {
        private:
            uint8_t wip_img[1928 * 495];
            std::vector<std::vector<uint8_t>> ccd_ids;

        public:
            std::shared_ptr<PMSSSegment> work(uint8_t *frm)
            {
                uint16_t line_cnt = ((frm[1946] & 0xF) << 8 | frm[1945]) >> 1;

                if (line_cnt >= 495)
                    return nullptr;

                uint8_t ccd_id = frm[1944] & 0xF;

                memcpy(&wip_img[1928 * line_cnt], frm + 16, 1928);
                ccd_ids.push_back({ccd_id});

                if (line_cnt == 494)
                {
                    satdump::image::Image img(wip_img, 8, 1928, 495, 1);

                    auto ccd_id = satdump::majority_law_vec(ccd_ids);

                    ccd_ids.clear();
                    memset(wip_img, 0, 1928 * 495);
                    return std::make_shared<PMSSSegment>(img, ccd_id[0]);
                }
                else
                {
                    return nullptr;
                }
            }
        };
    } // namespace pmss
} // namespace kanopus