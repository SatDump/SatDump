#include "lrit_data.h"
#include <cstring>

namespace goes
{
    namespace hrit
    {
        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder()
        {
            seg_count = 0;
            seg_size = 0;
            image_id = -1;
        }

        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder(int max_seg, int max_width, int max_height, uint16_t id) : seg_count(max_seg), image_id(id)
        {
            segments_done = std::shared_ptr<bool>(new bool[seg_count], [](bool *p)
                                                  { delete[] p; });
            std::fill(segments_done.get(), &segments_done.get()[seg_count], false);
            image = std::make_shared<image2::Image>(8, max_width, max_height, 1);
            seg_size = int(max_height / max_seg) * max_width;
        }

        SegmentedLRITImageDecoder::~SegmentedLRITImageDecoder()
        {
        }

        void SegmentedLRITImageDecoder::pushSegment(uint8_t *data, size_t this_size, int segc)
        {
            if (segc >= seg_count || segc < 0)
                return;
            std::memcpy((uint8_t*)image->raw_data() + seg_size * segc, data, this_size); // IMGTODO, maybe check 8-bits?
            segments_done.get()[segc] = true;
        }

        bool SegmentedLRITImageDecoder::isComplete()
        {
            bool complete = true;
            for (int i = 0; i < seg_count; i++)
                complete = complete && segments_done.get()[i];
            return complete;
        }
    }
}