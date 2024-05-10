#include "lrit_data.h"
#include <cstring>

namespace elektro
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

        SegmentedLRITImageDecoder::SegmentedLRITImageDecoder(int bit_depth, int max_seg, int segment_width, int segment_height, std::string id) : seg_count(max_seg), image_id(id)
        {
            segments_done = std::shared_ptr<bool>(new bool[seg_count], [](bool *p)
                                                  { delete[] p; });
            std::fill(segments_done.get(), &segments_done.get()[seg_count], false);

            image = image::Image(bit_depth, segment_width, segment_height * max_seg, 1);
            seg_height = segment_height;
            seg_width = segment_width;

            image.fill(0);
        }

        SegmentedLRITImageDecoder::~SegmentedLRITImageDecoder()
        {
        }

        void SegmentedLRITImageDecoder::pushSegment(image::Image &data, int segc)
        {
            if (segc >= seg_count || segc < 0)
                return;
            image::imemcpy(image, (seg_height * seg_width) * segc, data, 0, seg_height * seg_width);
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