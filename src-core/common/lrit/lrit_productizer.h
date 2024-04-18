#pragma once

#include <string>
#include "common/image/image.h"
#include "lrit_file.h"

namespace lrit
{
    template <typename T>
    class LRITProductizer
    {
    public:
        LRITProductizer();
        ~LRITProductizer();

        void saveImage(image::Image<T> img,
                       std::string directory,
                       std::string satellite,
                       std::string satshort,
                       std::string channel,
                       time_t timestamp,
                       std::string region = "",
                       lrit::ImageNavigationRecord *navigationHdr = nullptr);
    };
}