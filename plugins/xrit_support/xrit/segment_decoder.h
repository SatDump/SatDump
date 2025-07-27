#pragma once

#include "common/lrit/lrit_file.h"
#include "image/image.h"
#include "xrit/identify.h"
#include <cstring>

namespace satdump
{
    namespace xrit
    {
        class SegmentedImageDecoder
        {
        public:
            virtual void pushSegment(lrit::LRITFile &file) = 0;
            virtual bool isComplete() = 0;
            virtual void reset() = 0;
            virtual bool hasData() = 0;

        public:
            image::Image image;
            XRITFileInfo info;
        };
    } // namespace xrit
} // namespace satdump