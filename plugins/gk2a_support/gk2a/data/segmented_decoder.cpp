#include "lrit_data_decoder.h"
#include <cstring>

namespace gk2a
{
    namespace lrit
    {
        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder()
        {
            seg_count = 0;
            seg_height = 0;
            seg_width = 0;
            image_id = "";
        }

        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder(int max_seg, int segment_width, int segment_height, std::string id) : seg_count(max_seg), image_id(id)
        {
            segments_done = std::shared_ptr<bool>(new bool[seg_count], [](bool *p)
                                                  { delete[] p; });
            std::fill(segments_done.get(), &segments_done.get()[seg_count], false);

            image = image::Image<uint8_t>(segment_width, segment_height * max_seg, 1);
            seg_height = segment_height;
            seg_width = segment_width;

            image.fill(0);
        }

        SegmentedLRITImageDecoder::~SegmentedLRITImageDecoder()
        {
        }

        void SegmentedLRITImageDecoder::pushSegment(uint8_t *data, int segc)
        {
            if (segc >= seg_count)
                return;
            std::memcpy(&image[(seg_height * seg_width) * segc], data, seg_height * seg_width);
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